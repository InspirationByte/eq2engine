//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF model instancer
//////////////////////////////////////////////////////////////////////////////////

#ifndef EGFINSTANCER_H
#define EGFINSTANCER_H

#include "IEqModel.h"
#include "materialsystem/IMaterialSystem.h"

#define MAX_EGF_INSTANCES			256
#define MAX_INSTANCE_BODYGROUPS		8		// only 4 groups can be instanced
#define MAX_INSTANCE_LODS			4		// only 4 lods can be instanced

//---------------------------------------------------------------------

// for each bodygroup
template <class IT>
class CEGFInstancer : public IEqModelInstancer
{
friend class IEqModel;

public:
					CEGFInstancer();
					~CEGFInstancer();

	void			Init( VertexFormatDesc_t* instVertexFormat, int numAttrib );
	void			Init( IVertexFormat* instVertexFormat, IVertexBuffer* instBuffer );
	void			Cleanup();

	void			ValidateAssert();

	IT&				NewInstance( ubyte bodyGroupFlags, int lod );
	bool			HasInstances() const;

	virtual void	Draw( int renderFlags, IEqModel* model );

protected:

	IVertexFormat*		m_vertFormat;
	IVertexBuffer*		m_instanceBuf;

	IT*					m_instances[MAX_INSTANCE_BODYGROUPS][MAX_INSTANCE_LODS];
	ushort				m_numInstances[MAX_INSTANCE_BODYGROUPS][MAX_INSTANCE_LODS];

	bool				m_hasInstances;
	bool				m_preallocatedHWBuffer;
};

//-------------------------------------------------------

template <class IT>
inline CEGFInstancer<IT>::CEGFInstancer() :
	m_vertFormat(NULL),
	m_instanceBuf(NULL),
	m_preallocatedHWBuffer(false)
{
	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		memset(m_numInstances[i], 0, sizeof(m_numInstances[i]));
		memset(m_instances[i], 0, sizeof(m_instances[i]));
	}
}

template <class IT>
inline CEGFInstancer<IT>::~CEGFInstancer()
{
	Cleanup();
}

template <class IT>
inline void CEGFInstancer<IT>::ValidateAssert()
{
	ASSERTMSG(m_vertFormat != NULL && m_instanceBuf != NULL, "Instancer is not valid - did you forgot to initialize it???");
}

template <class IT>
inline void CEGFInstancer<IT>::Init( VertexFormatDesc_t* instVertexFormat, int numAttrib )
{
	Cleanup();

	m_vertFormat = g_pShaderAPI->CreateVertexFormat(instVertexFormat, numAttrib);
	m_instanceBuf = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_EGF_INSTANCES, sizeof(IT));
	m_instanceBuf->SetFlags( VERTBUFFER_FLAG_INSTANCEDATA );

	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		memset(m_numInstances[i], 0, sizeof(m_numInstances[i]));
		memset(m_instances[i], 0, sizeof(m_instances[i]));
	}

	m_hasInstances = false;
}

template <class IT>
inline void CEGFInstancer<IT>::Init( IVertexFormat* instVertexFormat, IVertexBuffer* instBuffer )
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

	m_hasInstances = false;
}

template <class IT>
inline void CEGFInstancer<IT>::Cleanup()
{
	if((m_instanceBuf || m_vertFormat) && !m_preallocatedHWBuffer)
	{
		g_pShaderAPI->Reset(STATE_RESET_VBO);
		g_pShaderAPI->ApplyBuffers();

		if(m_instanceBuf) g_pShaderAPI->DestroyVertexBuffer(m_instanceBuf);
		if(m_vertFormat) g_pShaderAPI->DestroyVertexFormat(m_vertFormat);
	}

	m_instanceBuf = NULL;
	m_vertFormat = NULL;

	for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
	{
		for(int j = 0; j < MAX_INSTANCE_LODS; j++)
		{
			delete [] m_instances[i][j];
			m_instances[i][j] = NULL;
			m_numInstances[i][j] = 0;
		}
	}

	m_hasInstances = false;
	m_preallocatedHWBuffer = false;
}

template <class IT>
inline IT& CEGFInstancer<IT>::NewInstance( ubyte bodyGroup, int lod )
{
	static IT dummy;

	if(bodyGroup == 0xFF)
		return dummy;

	int numInst = m_numInstances[bodyGroup][lod];

	if(numInst >= MAX_EGF_INSTANCES)
		return dummy; // overflow

	if(!m_instances[bodyGroup][lod])
		m_instances[bodyGroup][lod] = new IT[MAX_EGF_INSTANCES];

	m_hasInstances = true;

	// assign instance
	IT& theInst = m_instances[bodyGroup][lod][numInst];

	numInst++;
	m_numInstances[bodyGroup][lod] = numInst;

	return theInst;
}

template <class IT>
inline bool CEGFInstancer<IT>::HasInstances() const
{
	return m_hasInstances;
}

template <class IT>
inline void CEGFInstancer<IT>::Draw( int renderFlags, IEqModel* model )
{
	if(!model)
		return;

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	studiohdr_t* pHdr = model->GetHWData()->studio;

	ASSERTMSG(pHdr->numBodyGroups <= MAX_INSTANCE_BODYGROUPS, "Model got too many body groups! Tell it to programmer or reduce body group count.");

	// proceed to render
	materials->SetInstancingEnabled(true);

	for(int lod = 0; lod < MAX_INSTANCE_LODS; lod++)
	{
		for(int i = 0; i < pHdr->numBodyGroups; i++)
		{
			// don't do empty instances
			if(m_numInstances[i][lod] == 0)
				continue;

			int numInst = m_numInstances[i][lod];
			m_numInstances[i][lod] = 0;

			// before lock we have to unbind our buffer
			g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

			// upload instance buffer
			m_instanceBuf->Update(m_instances[i][lod], numInst, 0, true);

			int bodyGroupLOD = lod;
			int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
			studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

			int nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];

			// get the right LOD model number
			while(nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];
			}

			if(nModDescId == -1)
				continue;
	
			studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

			// render model groups that in this body group
			for(int j = 0; j < modDesc->numGroups; j++)
			{
				//materials->SetSkinningEnabled(true);

				int materialIndex = modDesc->pGroup(j)->materialIndex;
				materials->BindMaterial( model->GetMaterial(materialIndex), 0);

				//m_pModel->PrepareForSkinning( m_boneTransforms );

				g_pShaderAPI->SetVertexFormat(m_vertFormat);

				model->SetupVBOStream( 0 );
				g_pShaderAPI->SetVertexBuffer(m_instanceBuf, 2);

				model->DrawGroup( nModDescId, j, false );

				//materials->SetSkinningEnabled(false);
			}
		}
	}

	materials->SetInstancingEnabled(false);
	m_hasInstances = false;
}

#endif // EGFINSTANCER_H
