//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: eqPhysics bullet indexed mesh
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqBulletIndexedMesh.h"
#include "BulletConvert.h"

using namespace EqBulletUtils;

void CEqBulletIndexedMesh::MeshSubPart::getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart)
{
	type = PHY_FLOAT;
	indicestype = m_indexType;

	(*vertexbase) = m_vertexData;
	(*indexbase) = m_indexData;

	if (subpart != m_partId)
	{
		stride = 0;
		indexstride = 0;
		numverts = 0;
		numfaces = 0;
		return;
	}

	stride = m_vertexStride;
	indexstride = m_triangleIndexStride;
	numverts = m_numVerts;
	numfaces = m_numTriangles;
}

void CEqBulletIndexedMesh::MeshSubPart::getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, const unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart) const
{
	type = PHY_FLOAT;
	indicestype = m_indexType;

	(*vertexbase) = m_vertexData;
	(*indexbase) = m_indexData;
	if (subpart != m_partId)
	{
		stride = 0;
		indexstride = 0;
		numverts = 0;
		numfaces = 0;
		return;
	}

	stride = m_vertexStride;
	indexstride = m_triangleIndexStride;
	numverts = m_numVerts;
	numfaces = m_numTriangles;
	
}

//void CEqBulletIndexedMesh::MeshSubPart::getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax) const
//{
//	ConvertPositionToBullet(*aabbMin, m_bbox.minPoint);
//	ConvertPositionToBullet(*aabbMax, m_bbox.maxPoint);
//}

CEqBulletIndexedMesh::CEqBulletIndexedMesh(ubyte* vertexBase, int vertexStride, ubyte* indexBase, int indexStride, int numVerts, int numIndices)
	: m_vertexData(vertexBase)
	, m_vertexStride(vertexStride)
	, m_indexData(indexBase)
	, m_triangleIndexStride(indexStride * 3)
	, m_indexType(indexStride == sizeof(short) ? PHY_SHORT : PHY_INTEGER)
	, m_numVerts(numVerts)
	, m_numTriangles(numIndices / 3)
{
}

void CEqBulletIndexedMesh::AddSubpart(int firstIndex, int numIndices, int firstVertex, int numVerts, int materialId)
{
	ASSERT(numIndices > 0);
	ASSERT(numVerts > 0);

	const int partId = m_subparts.numElem();
	MeshSubPart& newPart = m_subparts.append();

	newPart.m_vertexData = m_vertexData;
	newPart.m_indexData = m_indexData + firstIndex / 3 * m_triangleIndexStride;
	newPart.m_indexType = m_indexType;
	newPart.m_vertexStride = m_vertexStride;
	newPart.m_triangleIndexStride = m_triangleIndexStride;
	newPart.m_numTriangles = numIndices / 3;
	newPart.m_numVerts = m_numVerts;
	newPart.m_materialId = materialId;
	newPart.m_partId = partId;
}

void CEqBulletIndexedMesh::getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart)
{
	m_subparts[subpart].getLockedVertexIndexBase(vertexbase, numverts, type, stride, indexbase, indexstride, numfaces, indicestype, subpart);
}

void CEqBulletIndexedMesh::getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, const unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart) const
{
	m_subparts[subpart].getLockedReadOnlyVertexIndexBase(vertexbase, numverts, type, stride, indexbase, indexstride, numfaces, indicestype, subpart);
}

int CEqBulletIndexedMesh::getSubPartMaterialId(int subpart) const
{
	return m_subparts[subpart].m_materialId;
}

//void CEqBulletIndexedMesh::getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax) const
//{
//	ConvertPositionToBullet(*aabbMin, m_bbox.minPoint);
//	ConvertPositionToBullet(*aabbMax, m_bbox.maxPoint);
//}