//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model cache for bullet physics
//				Generates real shapes for Bullet Collision
//
///////////////////////////////////////////////////////////////////////////////////

#include "BulletShapeCache.h"

#include "btBulletCollisionCommon.h"

#include "BulletConvert.h"

#include "eqGlobalMutex.h"

#include "ConVar.h"
#include "DebugInterface.h"

using namespace EqBulletUtils;
using namespace Threading;


ConVar ph_studioShapeMargin("ph_studioShapeMargin", "0.05", "Studio model shape marginal", CV_CHEAT);

// makes and caches shape. IsConvex defines that it was convex or not (also for internal use)
btCollisionShape* InternalGenerateShape(int numVertices, Vector3D* vertices, int *indices, int numIndices, EPODShapeType type, float margin)
{
	switch(type)
	{
		case PHYSSHAPE_TYPE_CONCAVE:
		case PHYSSHAPE_TYPE_MOVABLECONCAVE:
		{
			btTriangleIndexVertexArray* pTriMesh = new btTriangleIndexVertexArray(
				(int)numIndices/3,
				(int*)indices,
				sizeof(int)*3,
				numVertices,
				(float*)vertices,
				sizeof(Vector3D)
				);

			btCollisionShape* shape = NULL;

			if(type == PHYSSHAPE_TYPE_MOVABLECONCAVE)
			{
				ASSERTMSG(false, "InternalGenerateShape: cannot create PHYSSHAPE_TYPE_MOVABLECONCAVE shape because unsupported for now\n");
				//shape = new btGImpactMeshShape(pTriMesh);
				delete pTriMesh;
				return NULL;
			}
			else
				shape = new btBvhTriangleMeshShape(pTriMesh, true);

			// that's to be destroyed
			shape->setUserPointer(pTriMesh);

			return shape;
		}
		case PHYSSHAPE_TYPE_CONVEX:
		{
			btConvexHullShape* convexShape = new btConvexHullShape;

			for(int i = 0; i < numIndices; i++)
				convexShape->addPoint(ConvertPositionToBullet(vertices[indices[i]]));

			convexShape->setMargin(EQ2BULLET(0.1f * margin));

			return convexShape;
		}
	}

	MsgError("InternalGenerateShape: Shape type %d is invalid!\n", type);

	return NULL;
}

CBulletStudioShapeCache::CBulletStudioShapeCache() : m_mutex(GetGlobalMutex(MUTEXPURPOSE_PHYSICS))
{

}

// checks the shape is initialized for the cache
bool CBulletStudioShapeCache::IsShapeCachePresent( physmodelshapecache_t* shapeInfo )
{
	Threading::CScopedMutex m( m_mutex );

	for(int i = 0; i < m_collisionShapes.numElem(); i++)
	{
		if(shapeInfo->cachedata == m_collisionShapes[i])
			return true;
	}

	return false;
}

// initializes whole studio shape model with all objects
void CBulletStudioShapeCache::InitStudioCache( physmodeldata_t* studioData )
{
	// cache shapes using model info.
	for(int i = 0; i < studioData->numObjects; i++)
	{
		for(int j = 0; j < studioData->objects[i].object.numShapes; j++)
		{
			int nShape = studioData->objects[i].object.shape_indexes[j];

			btCollisionShape* shape = InternalGenerateShape(
									studioData->numVertices,
									studioData->vertices,
									studioData->indices + studioData->shapes[nShape].shape_info.startIndices,
									studioData->shapes[nShape].shape_info.numIndices,
									(EPODShapeType)studioData->shapes[nShape].shape_info.type, ph_studioShapeMargin.GetFloat());

			// cast physics POD index to index in physics engine
			studioData->objects[i].shapeCache[j] = shape;

			m_mutex.Lock();
			m_collisionShapes.append(shape);
			m_mutex.Unlock();

			studioData->shapes[nShape].cachedata = shape;
		}
	}
}

void CBulletStudioShapeCache::DestroyStudioCache( physmodeldata_t* studioData )
{
	for(int i = 0; i < studioData->numShapes; i++)
	{
		int nShape = m_collisionShapes.findIndex((btCollisionShape*)studioData->shapes[i].cachedata);

		if( m_collisionShapes[nShape]->getUserPointer() )
		{
			btTriangleIndexVertexArray* mesh = (btTriangleIndexVertexArray*)m_collisionShapes[nShape]->getUserPointer();

			delete mesh;
		}

		delete m_collisionShapes[nShape];

		m_mutex.Lock();
		m_collisionShapes.fastRemoveIndex( nShape );
		m_mutex.Unlock();
	}
}

// does all shape cleanup
void CBulletStudioShapeCache::Cleanup_Invalidate()
{
	for(int i = 0; i < m_collisionShapes.numElem(); i++)
	{
		if( m_collisionShapes[i]->getUserPointer() )
		{
			btTriangleIndexVertexArray* mesh = (btTriangleIndexVertexArray*)m_collisionShapes[i]->getUserPointer();
			delete mesh;
		}

		delete m_collisionShapes[i];
	}

	m_collisionShapes.clear();
}

// expose interface
static CBulletStudioShapeCache s_BulletShapeCache;
IStudioShapeCache* g_pStudioShapeCache = (IStudioShapeCache*)&s_BulletShapeCache;
