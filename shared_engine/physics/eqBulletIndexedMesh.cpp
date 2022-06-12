//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: eqPhysics bullet indexed mesh
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqBulletIndexedMesh.h"

CEqBulletIndexedMesh::CEqBulletIndexedMesh(ubyte* vertexBase, int vertexStride, ubyte* indexBase, int indexStride, int numVerts, int numIndices)
{
	m_vertexData = vertexBase;
	m_vertexStride = vertexStride;

	m_indexData = indexBase;
	m_indexStride = indexStride;

	m_numVerts = numVerts;
	m_numIndices = numIndices;

	m_indexType = m_indexStride == sizeof(short) ? PHY_SHORT : PHY_INTEGER;
}

void CEqBulletIndexedMesh::AddSubpart(int firstIndex, int numIndices, int firstVertex, int numVerts, int materialId)
{
	eqBulletMeshSubpart_t newPart;
	newPart.firstIndex = firstIndex;
	newPart.firstVertex = firstVertex;
	newPart.numIndices = numIndices;
	newPart.numVerts = numVerts;
	newPart.materialId = materialId;

	m_subparts.append( newPart );
}

int	CEqBulletIndexedMesh::getSubPartMaterialId( int subpart )
{
	if( m_subparts.numElem() )
	{
		ASSERT(subpart < m_subparts.numElem());

		return m_subparts[subpart].materialId;
	}

	return -1; // default
}

void CEqBulletIndexedMesh::getLockedVertexIndexBase(unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& stride,unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart)
{
	if( m_subparts.numElem() )
	{
		ASSERT(subpart < m_subparts.numElem());

		stride = m_vertexStride;
		type = PHY_FLOAT;
		numverts = m_subparts[subpart].numVerts;
		(*vertexbase) = m_vertexData;// + m_subparts[subpart].firstVertex*m_vertexStride;

		indexstride = m_indexStride*3;
		indicestype = m_indexType;
		numfaces = m_subparts[subpart].numIndices / 3;
		(*indexbase) = m_indexData + m_subparts[subpart].firstIndex*m_indexStride;
	}
	else
	{
		(void)subpart;

		stride = m_vertexStride;
		type = PHY_FLOAT;
		numverts = m_numVerts;
		(*vertexbase) = m_vertexData;

		indexstride = m_indexStride*3;
		indicestype = m_indexType;
		numfaces = m_numIndices / 3;
		(*indexbase) = m_indexData;
	}
}
		
void CEqBulletIndexedMesh::getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& stride,const unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart) const
{
	if( m_subparts.numElem() )
	{
		ASSERT(subpart < m_subparts.numElem());

		stride = m_vertexStride;
		type = PHY_FLOAT;
		numverts = m_subparts[subpart].numVerts;
		(*vertexbase) = m_vertexData;// + m_subparts[subpart].firstVertex*m_vertexStride;

		indexstride = m_indexStride*3;
		indicestype = m_indexType;
		numfaces = m_subparts[subpart].numIndices / 3;
		(*indexbase) = m_indexData + m_subparts[subpart].firstIndex*m_indexStride;
	}
	else
	{
		(void)subpart;

		stride = m_vertexStride;
		type = PHY_FLOAT;
		numverts = m_numVerts;
		(*vertexbase) = m_vertexData;

		indexstride = m_indexStride*3;
		indicestype = m_indexType;
		numfaces = m_numIndices / 3;
		(*indexbase) = m_indexData;
	}
}
	