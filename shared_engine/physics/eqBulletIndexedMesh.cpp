//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	ASSERT(numIndices > 0);
	ASSERT(numVerts > 0);

	MeshSubPart& newPart = m_subparts.append();

	newPart.firstIndex = firstIndex;
	newPart.firstVertex = firstVertex;
	newPart.numIndices = numIndices;
	newPart.numVerts = numVerts;
	newPart.materialId = materialId;
}

int	CEqBulletIndexedMesh::getSubPartMaterialId( int subpart )
{
	return m_subparts[subpart].materialId;
}

int CEqBulletIndexedMesh::getNumSubParts() const 
{
	return m_subparts.numElem();
}

void CEqBulletIndexedMesh::getLockedVertexIndexBase(unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& stride,unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart)
{
	stride = m_vertexStride;
	type = PHY_FLOAT;
	(*vertexbase) = m_vertexData;
	indexstride = m_indexStride * 3;
	indicestype = m_indexType;

	const MeshSubPart& subPart = m_subparts[subpart];

	numverts = subPart.numVerts;
	numfaces = subPart.numIndices / 3;
	(*indexbase) = m_indexData + subPart.firstIndex*m_indexStride;
}
		
void CEqBulletIndexedMesh::getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& stride,const unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart) const
{
	stride = m_vertexStride;
	type = PHY_FLOAT;
	(*vertexbase) = m_vertexData;
	indexstride = m_indexStride * 3;
	indicestype = m_indexType;

	const MeshSubPart& subPart = m_subparts[subpart];
	numverts = subPart.numVerts;
	numfaces = subPart.numIndices / 3;
	(*indexbase) = m_indexData + subPart.firstIndex*m_indexStride;
}
	