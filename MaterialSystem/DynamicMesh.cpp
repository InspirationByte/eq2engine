//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem dynamic mesh
//////////////////////////////////////////////////////////////////////////////////

#include "DynamicMesh.h"
#include "DebugInterface.h"

// FIXME: subdivide on streams???

#define MAX_DYNAMIC_VERTICES	32767
#define MAX_DYNAMIC_INDICES		32767

CDynamicMesh::CDynamicMesh() :
	m_primType( PRIM_TRIANGLES ),
	m_vertexFormat(NULL),
	m_vertexBuffer(NULL),
	m_indexBuffer(NULL),
	m_numVertices(0),
	m_numIndices(0),
	m_vertices(NULL),
	m_indices(NULL),
	m_vboDirty(false),
	m_vertexFormatAttribs(0),
	m_vertexFormatDesc(NULL)
{

}

CDynamicMesh::~CDynamicMesh()
{
}

bool CDynamicMesh::Init( VertexFormatDesc_t* desc, int numAttribs )
{
	if(m_vertexBuffer != NULL && m_indexBuffer != NULL && m_vertexFormat != NULL)
		return true;

	ASSERTMSG(numAttribs > 0, "CDynamicMesh::Init - numAttribs is ZERO!\n");

	m_vertexFormatAttribs = numAttribs;
	m_vertexFormatDesc = new VertexFormatDesc_t[numAttribs];

	int vertexSize = 0;

	for(int i = 0; i < numAttribs; i++)
	{
		m_vertexFormatDesc[i] = desc[i];

		int stream = desc[i].m_nStream;
		int vecCount = desc[i].m_nSize;

		ASSERTMSG(stream == 0, "Error - you should pass STREAM 0 only to CDynamicMesh::Init!\n");

		vertexSize += vecCount * attributeFormatSize[desc[i].m_nFormat];
	}

	m_vertexStride = vertexSize;

	m_vertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_DYNAMIC_VERTICES, m_vertexStride, NULL);
	m_indexBuffer = g_pShaderAPI->CreateIndexBuffer(MAX_DYNAMIC_INDICES, sizeof(uint16), BUFFER_DYNAMIC, NULL);
	m_vertexFormat = g_pShaderAPI->CreateVertexFormat(m_vertexFormatDesc, m_vertexFormatAttribs);

	return (m_vertexBuffer != NULL && m_indexBuffer != NULL && m_vertexFormat != NULL);
}

void CDynamicMesh::Destroy()
{
	if(m_vertexBuffer == NULL && m_indexBuffer == NULL && m_vertexFormat == NULL)
		return;

	Reset();
	Unlock();

	g_pShaderAPI->DestroyIndexBuffer(m_indexBuffer);
	g_pShaderAPI->DestroyVertexBuffer(m_vertexBuffer);
	g_pShaderAPI->DestroyVertexFormat(m_vertexFormat);

	m_indexBuffer = NULL;
	m_vertexBuffer = NULL;
	m_vertexFormat = NULL;

	delete [] m_vertexFormatDesc;
	m_vertexFormatDesc = NULL;

	m_vertexFormatAttribs = 0;
}

// returns a pointer to vertex format description
void CDynamicMesh::GetVertexFormatDesc(VertexFormatDesc_t** desc, int& numAttribs)
{
	*desc = m_vertexFormatDesc;
	numAttribs = m_vertexFormatAttribs;
}

// sets the primitive type (chooses the way how to allocate geometry parts)
void CDynamicMesh::SetPrimitiveType( PrimitiveType_e primType )
{
	m_primType = primType;
}

PrimitiveType_e	CDynamicMesh::GetPrimitiveType() const
{
	return m_primType;
}

void CDynamicMesh::AddStripBreak()
{
	// may be FAN also needs it, I forgot it =(
	if(m_primType != PRIM_TRIANGLE_STRIP)
		return;

	if(m_numIndices == 0)
		return; // no problemo

	int num_ind = m_numIndices;

	uint16 nIndicesCurr = 0;

	// if it's a second, first I'll add last index (3 if first, and add first one from fourIndices)
	if( num_ind > 0 )
	{
		uint16 lastIdx = m_indices[ num_ind-1 ];
		nIndicesCurr = lastIdx+1;

		// add two last indices to make degenerates
		uint16 degenerate[2] = {lastIdx, nIndicesCurr};

		memcpy(&m_indices[m_numIndices], degenerate, sizeof(uint16) * 2);
		m_numIndices += 2;
	}
}

// allocates geometry chunk. Returns the start index. Will return -1 if failed
// addStripBreak is for PRIM_TRIANGLE_STRIP. Set it false to work with current strip
int CDynamicMesh::AllocateGeom( int nVertices, int nIndices, void** verts, uint16** indices, bool addStripBreak /*= true*/ )
{
	if(nVertices == 0)
		return -1;

	if(!Lock())
		return -1;

	if(addStripBreak)
		AddStripBreak();

	int startVertex = m_numVertices;
	int startIndex = m_numIndices;

	// apply offsets first
	m_numVertices += nVertices;

	if(indices && nIndices)
		m_numIndices += nIndices;

	// give the pointers
	*verts = (ubyte*)m_vertices + startVertex * m_vertexStride;

	memset(*verts, 0, m_vertexStride*nVertices);

	// indices are optional
	if(indices && nIndices)
	{
		*indices = &m_indices[startIndex];
		memset(*indices, 0, sizeof(uint16)*nIndices);
	}

	memset(*verts, 0, m_vertexStride*nVertices);

	return startVertex;
}

bool CDynamicMesh::Lock()
{
	if(!m_vboDirty)
	{
		if(!m_vertexBuffer->Lock(0, MAX_DYNAMIC_VERTICES, &m_vertices, false))
			return false;

		if(!m_indexBuffer->Lock(0, MAX_DYNAMIC_INDICES, (void**)&m_indices, false))
			return false;

		m_vboDirty = true;
	}

	return true;
}

void CDynamicMesh::Unlock()
{
	if(m_vboDirty)
	{
		m_vertexBuffer->Unlock();
		m_indexBuffer->Unlock();

		m_vboDirty = false;
	}
}

// uploads buffers and renders the mesh. Note that you has been set material and adjusted RTs
void CDynamicMesh::Render()
{
	if(m_numVertices == 0)
		return;

	bool drawIndexed = m_numIndices > 0;

	// get resource back
	Unlock();

	g_pShaderAPI->Reset(STATE_RESET_VBO);

	g_pShaderAPI->SetVertexFormat(m_vertexFormat);
	g_pShaderAPI->SetVertexBuffer(m_vertexBuffer, 0);

	if(drawIndexed)
		g_pShaderAPI->SetIndexBuffer(m_indexBuffer);

	g_pShaderAPI->ApplyBuffers();

	if(drawIndexed)
		g_pShaderAPI->DrawIndexedPrimitives(m_primType, 0, m_numIndices, 0, m_numVertices);
	else
		g_pShaderAPI->DrawNonIndexedPrimitives(m_primType, 0, m_numVertices);
}

// resets the dynamic mesh
void CDynamicMesh::Reset()
{
	m_numVertices = 0;
	m_numIndices = 0;
}