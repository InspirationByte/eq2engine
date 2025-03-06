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
public:
	class MeshSubPart;

	CEqBulletIndexedMesh(ubyte* vertexBase, int vertexStride, ubyte* indexBase, int indexStride, int numVerts, int numIndices);
	
	void			AddSubpart(int firstIndex, int numIndices, int firstVertex, int numVerts, int materialId);
	MeshSubPart*	GetSubpart(int index) const { return const_cast<MeshSubPart*>(&m_subparts[index]); }
	int				GetSubpartMaterialIdx(int subpart) const;

	int				getNumSubParts() const { return m_subparts.numElem(); }

	void			getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0);
	void			getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, const unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) const;
	void			unLockVertexBase(int subpart) {};
	void			unLockReadOnlyVertexBase(int subpart) const {};
	void			preallocateVertices(int numverts) {};
	void			preallocateIndices(int numindices) {};

	//bool			hasPremadeAabb() const { return true; }
	//void			getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax) const;

	class MeshSubPart : public btStridingMeshInterface
	{
		friend class CEqBulletIndexedMesh;
	public:
		int		getNumSubParts() const { return 1; }
		int		getSubPartMaterialId() const { return m_materialId; }

		void	getLockedVertexIndexBase(unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0);
		void	getLockedReadOnlyVertexIndexBase(const unsigned char** vertexbase, int& numverts, PHY_ScalarType& type, int& stride, const unsigned char** indexbase, int& indexstride, int& numfaces, PHY_ScalarType& indicestype, int subpart = 0) const;
		void	unLockVertexBase(int subpart) {};
		void	unLockReadOnlyVertexBase(int subpart) const {};
		void	preallocateVertices(int numverts) {};
		void	preallocateIndices(int numindices) {};

		//bool	hasPremadeAabb() const { return true; }
		//void	getPremadeAabb(btVector3* aabbMin, btVector3* aabbMax) const;

	protected:
		ubyte*			m_vertexData{ nullptr };
		ubyte*			m_indexData{ nullptr };

		BoundingBox		m_bbox;

		PHY_ScalarType	m_indexType{ PHY_FLOAT };
		int				m_vertexStride{ 0 };

		int				m_triangleIndexStride{ 0 };
		int				m_numTriangles{ 0 };
		int				m_numVerts{ 0 };
		int				m_materialId{ -1 };
		int				m_partId{ -1 };
	};
	
protected:

	Array<MeshSubPart>	m_subparts{ PP_SL }; // or batches
	
	BoundingBox	m_bbox;

	ubyte*		m_vertexData{ nullptr };
	ubyte*		m_indexData{ nullptr };
	PHY_ScalarType	m_indexType{ PHY_UCHAR };
	int			m_vertexStride{ 0 };
	int			m_triangleIndexStride{ 0 };
	int			m_numTriangles{ 0 };
	int			m_numVerts{ 0 };
};
