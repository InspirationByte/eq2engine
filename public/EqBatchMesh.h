//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium batch mesh model (renderable) and mesh builder
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQBATCHMESH_H
#define EQBATCHMESH_H

#include "materialsystem/IMaterialSystem.h"

class IEqMesh;

class IEqMeshBatch
{
public:
				IEqMeshBatch(IMaterial* pMaterial);
	virtual		~IEqMeshBatch();


	// for unlocking issue
	int			m_numVerts;
	int			m_numIndices;

	// for rendering
	int			m_firstVertex;
	int			m_firstIndex;

	IMaterial*	m_pMaterial;

	IEqMesh*	m_owner;
};

// kind of mesh builder
template<class V>
class CEqMeshBatch : public IEqMeshBatch
{
public:
					CEqMeshBatch(IMaterial* pMaterial);
					~CEqMeshBatch<V>();


	void			ReleaseData();

	// TODO: mesh builder

	// direct access
	int				m_firstVertex;
	int				m_numVerts;

	int				m_firstIndex;
	int				m_numIndices;
};

//-------------------------------------------------------------------------------------

class IEqMesh
{
public:
					IEqMesh();
	virtual			~IEqMesh();

	// binds mesh for rendering
	void			Bind();

	// returns vertex size
	int				GetVertexSize();

	// locks mesh for modify
	virtual bool	Lock() = 0;

	// unlocks mesh (and uploads render data if possible)
	virtual bool	Unlock() = 0;

	void			SetVertexDesc(VertexFormatDesc_t* desc, int elemCount);

protected:

	// vertex size
	int				m_vertexStride;

	IVertexBuffer*	m_vertexBuffer;
	IIndexBuffer*	m_indexBuffer;
	IVertexFormat*	m_vertexFormat;

	void*			m_deviceLockVertices;
	void*			m_deviceLockIndices;

	bool			m_isLocked;
};

//-------------------------------------------------------------------------------------

template<class V>
class CEqMesh : public IEqMesh
{
public:
				CEqMesh();
				~CEqMesh<V>();

	// locks mesh for modify
	bool		Lock();

	// unlocks mesh (and uploads render data if possible)
	bool		Unlock();

	// returns batch list reference for modify (only when locked)
	void		GetBatchList(DkList<CEqMeshBatch<V>*>& batches);

protected:
	DkList<CEqMeshBatch<V>*>	m_batchs;
};

#endif // EQBATCHMESH_H