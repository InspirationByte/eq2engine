//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: eqPhysics bullet indexed mesh
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <BulletCollision/CollisionShapes/btStridingMeshInterface.h>

class CEqBulletIndexedMesh : public btStridingMeshInterface
{
	friend class CEqCollisionBroadphaseGrid;
public:

	CEqBulletIndexedMesh(ubyte* vertexBase, int vertexStride, ubyte* indexBase, int indexStride, int numVerts, int numIndices);
	~CEqBulletIndexedMesh() {}

	void							AddSubpart(int firstIndex, int numIndices, int firstVertex, int numVerts, int materialId);

	//------------------------------------------

	void							getLockedVertexIndexBase(unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& stride,unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart=0);

	void							getLockedReadOnlyVertexIndexBase(const unsigned char **vertexbase, int& numverts,PHY_ScalarType& type, int& stride,const unsigned char **indexbase,int & indexstride,int& numfaces,PHY_ScalarType& indicestype,int subpart=0) const;

	/// unLockVertexBase finishes the access to a subpart of the triangle mesh
	/// make a call to unLockVertexBase when the read and write access (using getLockedVertexIndexBase) is finished
	void							unLockVertexBase(int subpart) {};

	void							unLockReadOnlyVertexBase(int subpart) const {};

	/// getNumSubParts returns the number of seperate subparts
	/// each subpart has a continuous array of vertices and indices
	int								getNumSubParts() const;

	int								getSubPartMaterialId( int subpart );

	void							preallocateVertices(int numverts) {};
	void							preallocateIndices(int numindices) {};
protected:

	struct MeshSubPart
	{
		int firstIndex;
		int firstVertex;
		int numIndices;
		int numVerts;

		int materialId;
	};

	Array<MeshSubPart>	m_subparts{ PP_SL }; // or batches

	ubyte*				m_vertexData;
	int					m_vertexStride;

	ubyte*				m_indexData;
	int					m_indexStride;

	int					m_numVerts;
	int					m_numIndices;

	PHY_ScalarType		m_indexType;
};
