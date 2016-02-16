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
#define MAX_INSTANCE_BODYGROUPS		4		// only 4 groups can be instanced
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
	ASSERTMSG(m_vertFormat != NULL, "Instancer is not valid - did you forgot to initialize it???");
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
inline IT& CEGFInstancer<IT>::NewInstance( ubyte bodyGroupFlags, int lod )
{
	static IT dummy;

	int idx = bodyGroupFlags-1;

	if(idx == -1)
		return dummy;	// nothing to render

	int numInst = m_numInstances[idx][lod];

	if(numInst >= MAX_EGF_INSTANCES)
		return dummy; // overflow

	if(!m_instances[idx][lod])
		m_instances[idx][lod] = new IT[MAX_EGF_INSTANCES];

	m_hasInstances = true;

	// assign instance
	IT& theInst = m_instances[idx][lod][numInst];

	numInst++;
	m_numInstances[idx][lod] = numInst;

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

	studiohdr_t* pHdr = model->GetHWData()->pStudioHdr;

	// proceed to render
	materials->SetInstancingEnabled(true);

	IT* instData;

	for(int lod = 0; lod < MAX_INSTANCE_LODS; lod++)
	{
		for(int i = 0; i < pHdr->numbodygroups; i++)
		{
			// don't do empty instances
			if(m_numInstances[i][lod] == 0)
				continue;

			int numInst = m_numInstances[i][lod];
			m_numInstances[i][lod] = 0;

			// before lock we have to unbind our buffer
			g_pShaderAPI->ChangeVertexBuffer( NULL, 2 );

			// upload instance buffer
			if( m_instanceBuf->Lock(0, numInst, (void**)&instData, false))
			{
				memcpy(instData, m_instances[i][lod], sizeof(IT)*numInst);
				m_instanceBuf->Unlock();
			}

			int bodyGroupLOD = lod;	// TODO: select lods or instance them

			int nLodModelIdx = pHdr->pBodyGroups(i)->lodmodel_index;
			int nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];

			// get the right LOD model number
			while(nModDescId == -1 && bodyGroupLOD > 0)
			{
				bodyGroupLOD--;
				nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];
			}

			if(nModDescId == -1)
				continue;

			// render model groups that in this body group
			for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
			{
				//materials->SetSkinningEnabled(true);

				IMaterial* pMaterial = model->GetMaterial(nModDescId, j);
				materials->BindMaterial(pMaterial, false);

				g_pShaderAPI->SetVertexFormat(m_vertFormat);

				model->SetupVBOStream( 0 );
				g_pShaderAPI->SetVertexBuffer(m_instanceBuf, 2);

				model->DrawGroup( nModDescId, j, false );

				//materials->SetSkinningEnabled(false);
			}

			g_pShaderAPI->SetVertexBuffer(NULL, 2);
		}
	}

	materials->SetInstancingEnabled(false);
	m_hasInstances = false;
}

#endif // EGFINSTANCER_H
