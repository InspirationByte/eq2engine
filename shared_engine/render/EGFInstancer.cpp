//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/IMaterialSystem.h"
#include "EGFInstancer.h"

// this is the truth about templates and inline...


CEGFInstancerBase::CEGFInstancerBase()
{
	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		memset(m_numInstances[i], 0, sizeof(m_numInstances[i]));
		memset(m_instances[i], 0, sizeof(m_instances[i]));
	}
}

CEGFInstancerBase::~CEGFInstancerBase()
{
	Cleanup();
}

void CEGFInstancerBase::ValidateAssert()
{
	ASSERT_MSG(m_vertFormat != nullptr && m_instanceBuf != nullptr, "Instancer is not valid - did you forgot to initialize it???");
}

void CEGFInstancerBase::InitEx( const VertexFormatDesc_t* instVertexFormat, int numAttrib, int sizeOfInstance )
{
	Cleanup();

	m_vertFormat = g_pShaderAPI->CreateVertexFormat("instancerFmt", instVertexFormat, numAttrib);
	m_instanceBuf = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_EGF_INSTANCES, sizeOfInstance);
	m_instanceBuf->SetFlags( VERTBUFFER_FLAG_INSTANCEDATA );

	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		memset(m_numInstances[i], 0, sizeof(m_numInstances[i]));
		memset(m_instances[i], 0, sizeof(m_instances[i]));
	}
}

void CEGFInstancerBase::Init( IVertexFormat* instVertexFormat, IVertexBuffer* instBuffer )
{
	Cleanup();

	m_preallocatedHWBuffer = true;

	m_vertFormat = instVertexFormat;
	m_instanceBuf = instBuffer;

	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		memset(m_numInstances[i], 0, sizeof(m_numInstances[i]));
		memset(m_instances[i], 0, sizeof(m_instances[i]));
	}
}

void CEGFInstancerBase::Cleanup()
{
	if((m_instanceBuf || m_vertFormat) && !m_preallocatedHWBuffer)
	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);
		g_pShaderAPI->ApplyBuffers();

		if(m_instanceBuf) g_pShaderAPI->DestroyVertexBuffer(m_instanceBuf);
		if(m_vertFormat) g_pShaderAPI->DestroyVertexFormat(m_vertFormat);
	}

	m_instanceBuf = nullptr;
	m_vertFormat = nullptr;

	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		for(int j = 0; j < MAX_INSTANCE_LODS; j++)
		{
			PPFree(m_instances[i][j]);
			m_instances[i][j] = nullptr;
			m_numInstances[i][j] = 0;
		}
	}

	m_hasInstances = false;
	m_preallocatedHWBuffer = false;
}

bool CEGFInstancerBase::HasInstances() const
{
	return m_hasInstances;
}

void CEGFInstancerBase::Draw( int renderFlags, IEqModel* model )
{
	if(!model)
		return;

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	studiohdr_t* pHdr = model->GetHWData()->studio;

	ASSERT_MSG(pHdr->numBodyGroups <= MAX_INSTANCE_BODYGROUPS, "Model got too many body groups! Tell it to programmer or reduce body group count.");

	// proceed to render
	materials->SetInstancingEnabled(true);

	IVertexBuffer* instBuffer = m_instanceBuf;
	g_pShaderAPI->SetVertexFormat(m_vertFormat);

	for(int lod = 0; lod < MAX_INSTANCE_LODS; lod++)
	{
		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			int numInst = m_numInstances[i][lod];

			// don't do empty instances
			if(numInst == 0)
				continue;

			m_numInstances[i][lod] = 0;

			int bodyGroupLOD = lod;
			const int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
			const studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

			int nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];

			// get the right LOD model number
			while(nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];
			}

			if(nModDescId == -1)
				continue;
	
			const studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

			// upload instance buffer
			instBuffer->Update(m_instances[i][lod], numInst, 0, true);

			// render model groups that in this body group
			for(int j = 0; j < modDesc->numGroups; j++)
			{
				int materialIndex = modDesc->pGroup(j)->materialIndex;
				IMaterial* pMaterial = model->GetMaterial(materialIndex);

				// sadly, instancer won't draw any transparent objects due to problems
				if(pMaterial->GetFlags() & (MATERIAL_FLAG_TRANSPARENT | MATERIAL_FLAG_ADDITIVE | MATERIAL_FLAG_MODULATE))
					continue;

				//materials->SetSkinningEnabled(true);

				
				materials->BindMaterial(pMaterial, 0);

				//m_pModel->PrepareForSkinning( m_boneTransforms );
				model->SetupVBOStream(0);
				g_pShaderAPI->SetVertexBuffer(instBuffer, 2);

				model->DrawGroup( nModDescId, j, false );

				//materials->SetSkinningEnabled(false);
			}
		}
	}

	g_pShaderAPI->SetVertexBuffer(nullptr, 2);
	materials->SetInstancingEnabled(false);
	m_hasInstances = false;
}
