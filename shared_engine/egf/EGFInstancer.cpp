//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/IMaterialSystem.h"
#include "EGFInstancer.h"


struct EGFInstBuffer
{
	IVertexBuffer*	instanceVB{ nullptr };
	void*			instances{ nullptr };
	ushort			numInstances{ 0 };
	ushort			upToDateInstanes{ 0 };

	~EGFInstBuffer()
	{
		PPFree(instances);
		g_pShaderAPI->DestroyVertexBuffer(instanceVB);
	}

	void Init(int sizeOfInstance)
	{
		instanceVB = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, EGF_INST_POOL_MAX_INSTANCES, sizeOfInstance);
		instanceVB->SetFlags(VERTBUFFER_FLAG_INSTANCEDATA);

		instances = PPAlloc(sizeOfInstance * EGF_INST_POOL_MAX_INSTANCES);
	}
};

CBaseEqGeomInstancer::CBaseEqGeomInstancer()
{
}

CBaseEqGeomInstancer::~CBaseEqGeomInstancer()
{
	Cleanup();
}

void CBaseEqGeomInstancer::ValidateAssert()
{
	ASSERT_MSG(m_vertFormat != nullptr, "Instancer is not valid - did you forgot to initialize it???");
}

void CBaseEqGeomInstancer::InitEx( const VertexFormatDesc_t* instVertexFormat, int numAttrib, int sizeOfInstance )
{
	Cleanup();
	m_ownsVertexFormat = true;
	m_vertFormat = g_pShaderAPI->CreateVertexFormat("instancerFmt", instVertexFormat, numAttrib);
}

void CBaseEqGeomInstancer::Init( IVertexFormat* instVertexFormat, int sizeOfInstance)
{
	Cleanup();
	m_instanceSize = sizeOfInstance;
	m_vertFormat = instVertexFormat;
	m_ownsVertexFormat = false;

	m_bodyGroupBounds[0] = 255;
	m_bodyGroupBounds[1] = 0;
	m_lodBounds[0] = 255;
	m_lodBounds[1] = 0;
	m_matGroupBounds[0] = 255;
	m_matGroupBounds[1] = 0;
}

void CBaseEqGeomInstancer::Cleanup()
{
	if(m_vertFormat && m_ownsVertexFormat)
	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);
		g_pShaderAPI->ApplyBuffers();
		g_pShaderAPI->DestroyVertexFormat(m_vertFormat);
	}
	m_data.clear(true);
	m_vertFormat = nullptr;

	m_ownsVertexFormat = false;
	m_hasInstances = false;
}

bool CBaseEqGeomInstancer::HasInstances() const
{
	return m_hasInstances;
}

void* CBaseEqGeomInstancer::AllocInstance(int bodyGroup, int lod, int materialGroup)
{
	ushort bufId = MakeInstId(bodyGroup, lod, materialGroup);

	auto it = m_data.find(bufId);
	if (it == m_data.end())
	{
		it = m_data.insert(bufId);
		EGFInstBuffer& newBuffer = *it;
		newBuffer.Init(m_instanceSize);
	}

	EGFInstBuffer& buffer = *it;
	if (buffer.numInstances >= EGF_INST_POOL_MAX_INSTANCES)
	{
		// TODO: create new pool
		return nullptr;
	}

	m_bodyGroupBounds[0] = min(m_bodyGroupBounds[0], bodyGroup);
	m_bodyGroupBounds[1] = max(m_bodyGroupBounds[1], bodyGroup);
	m_lodBounds[0] = min(m_lodBounds[0], lod);
	m_lodBounds[1] = max(m_lodBounds[1], lod);
	m_matGroupBounds[0] = min(m_matGroupBounds[0], materialGroup);
	m_matGroupBounds[1] = max(m_matGroupBounds[1], materialGroup);

	m_hasInstances = true;

	const int instanceId = buffer.numInstances++;
	return (ubyte*)buffer.instances + instanceId * m_instanceSize;
}

void CBaseEqGeomInstancer::Upload()
{
	// update all HW instance buffers
	for (auto it = m_data.begin(); it != m_data.end(); ++it)
	{
		EGFInstBuffer& buffer = *it;
		if (buffer.upToDateInstanes == buffer.numInstances)
			continue;

		const int updateStart = buffer.upToDateInstanes;
		const int updateCount = buffer.numInstances - buffer.upToDateInstanes;
		void* dataStart = (ubyte*)buffer.instances + buffer.upToDateInstanes * m_instanceSize;
		const bool discard = (updateStart == 0);

		buffer.instanceVB->Update(dataStart, updateCount, updateStart, discard);
		buffer.upToDateInstanes = buffer.numInstances;
	}
}

void CBaseEqGeomInstancer::Invalidate()
{
	for (auto it = m_data.begin(); it != m_data.end(); ++it)
	{
		EGFInstBuffer& buffer = *it;
		buffer.upToDateInstanes = buffer.numInstances = 0;
	}
	m_hasInstances = false;

	m_bodyGroupBounds[0] = 255;
	m_bodyGroupBounds[1] = 0;
	m_lodBounds[0] = 255;
	m_lodBounds[1] = 0;
	m_matGroupBounds[0] = 255;
	m_matGroupBounds[1] = 0;
}

void CBaseEqGeomInstancer::Draw( int renderFlags, CEqStudioGeom* model )
{
	if(!model)
		return;

	ASSERT(model->GetInstancer() == this);

	Upload();

	// proceed to render
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());
	materials->SetInstancingEnabled(true);

	// skinning not yet supported. But we can support it with textures holding data
	materials->SetSkinningEnabled(false);

	g_pShaderAPI->SetVertexFormat(m_vertFormat);

	const studiohdr_t& studio = model->GetStudioHdr();
	for(int lod = m_lodBounds[0]; lod <= m_lodBounds[1]; lod++)
	{
		for(int bodyGrp = m_bodyGroupBounds[0]; bodyGrp <= m_bodyGroupBounds[1]; bodyGrp++)
		{
			int bodyGroupLOD = lod;
			const int nLodModelIdx = studio.pBodyGroups(bodyGrp)->lodModelIndex;
			const studiolodmodel_t* lodModel = studio.pLodModel(nLodModelIdx);

			int nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];

			// get the right LOD model number
			while(nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];
			}

			if(nModDescId == -1)
				continue;
	
			const studiomodeldesc_t* modDesc = studio.pModelDesc(nModDescId);

			for (int mGrp = m_matGroupBounds[0]; mGrp <= m_matGroupBounds[1]; mGrp++)
			{
				const ushort dataId = MakeInstId(bodyGrp, lod, mGrp);
				auto dataIt = m_data.find(dataId);
				if (dataIt == m_data.end())
					continue;

				EGFInstBuffer& buffer = *dataIt;
				IVertexBuffer* instBuffer = buffer.instanceVB;

				if (buffer.numInstances == 0)
					continue;

				// render model groups that in this body group
				for (int i = 0; i < modDesc->numGroups; i++)
				{
					const int materialIndex = modDesc->pGroup(i)->materialIndex;
					IMaterial* pMaterial = model->GetMaterial(materialIndex, mGrp);

					// sadly, instancer won't draw any transparent objects due to problems
					if (pMaterial->GetFlags() & (MATERIAL_FLAG_TRANSPARENT | MATERIAL_FLAG_ADDITIVE | MATERIAL_FLAG_MODULATE))
						continue;

					//materials->SetSkinningEnabled(true);
					materials->BindMaterial(pMaterial, 0);

					//m_pModel->PrepareForSkinning( m_boneTransforms );
					model->SetupVBOStream(0);
					g_pShaderAPI->SetVertexBuffer(instBuffer, 2);

					model->DrawGroup(nModDescId, i, false);

					//materials->SetSkinningEnabled(false);
				}
			} // mGrp
		} // bodyGrp
	} // lod

	g_pShaderAPI->SetVertexBuffer(nullptr, 2);
	materials->SetInstancingEnabled(false);

	Invalidate();
}
