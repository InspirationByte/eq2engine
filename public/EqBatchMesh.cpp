//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium batch mesh model (renderable) and mesh builder
//////////////////////////////////////////////////////////////////////////////////

#include "EqBatchMesh.h"
#include "eqlevel.h"

//-------------------------------------------------------------------------------------

IEqMeshBatch::IEqMeshBatch(IMaterial* pMaterial)
{
	m_pMaterial = pMaterial;
	m_pMaterial->Ref_Grab();
}

IEqMeshBatch::~IEqMeshBatch()
{
	materials->FreeMaterial(m_pMaterial);
}

//-------------------------------------------------------------------------------------

template<class V>
CEqMeshBatch<V>::CEqMeshBatch(IMaterial* pMaterial)
{

}

template<class V>
CEqMeshBatch<V>::~CEqMeshBatch()
{
	ReleaseData();
}

template<class V>
void CEqMeshBatch<V>::ReleaseData()
{
	m_verts.clear();
	m_indices.clear();
}

//-------------------------------------------------------------------------------------

IEqMesh::IEqMesh() : 
	m_vertexBuffer(NULL),
	m_indexBuffer(NULL),
	m_vertexFormat(NULL),
	m_isLocked(false),
	m_vertexStride(0)
{

}

IEqMesh::~IEqMesh()
{
	// delete VBO
	if(m_vertexBuffer)
		g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);

	if(m_indexBuffer)
		g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);

	if(m_vertexFormat)
		g_pShaderAPI->DestroyVertexFormat(m_vertexFormat);
}

// binds mesh for rendering
void IEqMesh::Bind()
{
	ASSERT(!m_isLocked);
	ASSERT(m_vertexStride > 0);

	if(!m_vertexFormat || !m_indexBuffer || !m_vertexFormat)
		ASSERT(!"IEqMesh: no data created");

	g_pShaderAPI->SetVertexFormat(m_vertexFormat);
	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, 0);
	g_pShaderAPI->SetIndexBuffer(m_indexBuffer);
}

// returns vertex size
int	IEqMesh::GetVertexSize()
{
	return m_vertexStride;
}

void IEqMesh::SetVertexDesc(VertexFormatDesc_t* desc, int elemCount)
{
	if(m_vertexFormat)
		g_pShaderAPI->DestroyVertexFormat( m_vertexFormat );

	// create new
	m_vertexFormat = g_pShaderAPI->CreateVertexFormat( desc, elemCount );
}

template<class V>
CEqMesh<V>::CEqMesh() : IEqMesh()
{
	m_vertexStride = sizeof(V);
}

template<class V>
CEqMesh<V>::~CEqMesh()
{
	for(int i = 0; i < m_batchs.numElem(); i++)
	{
		delete m_batchs[i];
	}

	m_batchs.clear();
}

// locks mesh for modify
template<class V>
bool CEqMesh<V>::Lock()
{
	if(m_batchs.numElem() == 0)
	{
		// do nothing?
		return false;
	}

	int lock_count_v = 0;
	int lock_count_i = 0;

	for(int i = 0; i < m_batchs.numElem(); i++)
	{
		lock_count_v += m_batchs[i]->m_numVerts;
		lock_count_i += m_batchs[i]->m_numIndices;
	}

	void*	deviceLockVertices;
	void*	deviceLockIndices;

	if( !m_vertexBuffer->Lock(0, lock_count_v, &deviceLockVertices, false) || 
		!m_indexBuffer->Lock(0, lock_count_i, &deviceLockIndices, false))
	{
		ASSERTMSG(false, "CEqMesh::Lock(): Can't lock mesh!");
	}

	return true;
}

// unlocks mesh (and uploads render data if possible)
template<class V>
bool CEqMesh<V>::Unlock()
{

}

// returns batch list reference for modify (only when locked)
template<class V>
void CEqMesh<V>::GetBatchList(DkList<CEqMeshBatch<V>*>& batches)
{
	batches = &m_batches;
}

CEqMesh<eqlevelvertex_t> g_meshTest;