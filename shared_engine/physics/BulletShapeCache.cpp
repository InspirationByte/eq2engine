//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model cache for bullet physics
//				Generates real shapes for Bullet Collision
///////////////////////////////////////////////////////////////////////////////////

#include <btBulletCollisionCommon.h>

#include "core/core_common.h"
#include "core/ConVar.h"

#include "BulletShapeCache.h"
#include "BulletConvert.h"

using namespace EqBulletUtils;
using namespace Threading;
static CEqMutex s_shapeCacheMutex;

ConVar ph_studioShapeMargin("ph_studioShapeMargin", "0.05", "Studio model shape marginal", CV_CHEAT);

// makes and caches shape. IsConvex defines that it was convex or not (also for internal use)
btCollisionShape* InternalGenerateShape(int numVertices, Vector3D* vertices, int *indices, int numIndices, EPhysShapeType type, float margin)
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

			btCollisionShape* shape = nullptr;

			if(type == PHYSSHAPE_TYPE_MOVABLECONCAVE)
			{
				ASSERT_FAIL("InternalGenerateShape: cannot create PHYSSHAPE_TYPE_MOVABLECONCAVE shape because unsupported for now\n");
				//shape = new btGImpactMeshShape(pTriMesh);
				delete pTriMesh;
				return nullptr;
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

			for (int i = 0; i < numIndices; i++)
			{
				btVector3 vec;
				ConvertPositionToBullet(vec, vertices[indices[i]]);
				convexShape->addPoint(vec, false);
			}

			convexShape->setMargin(EQ2BULLET(0.1f * margin));
			convexShape->recalcLocalAabb();

			return convexShape;
		}
	}

	MsgError("InternalGenerateShape: Shape type %d is invalid!\n", type);

	return nullptr;
}

CBulletStudioShapeCache::CBulletStudioShapeCache()
{
}

bool CBulletStudioShapeCache::IsInitialized() const 
{
	return true; 
}

const char* CBulletStudioShapeCache::GetInterfaceName() const 
{
	return SHAPECACHE_INTERFACE_VERSION; 
}

// checks the shape is initialized for the cache
bool CBulletStudioShapeCache::IsShapeCachePresent( studioPhysShapeCache_t* shapeInfo )
{
	CScopedMutex m(s_shapeCacheMutex);

	for(int i = 0; i < m_collisionShapes.numElem(); i++)
	{
		if(shapeInfo->cachedata == m_collisionShapes[i])
			return true;
	}

	return false;
}

// initializes whole studio shape model with all objects
void CBulletStudioShapeCache::InitStudioCache( studioPhysData_t* studioData )
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
									(EPhysShapeType)studioData->shapes[nShape].shape_info.type, ph_studioShapeMargin.GetFloat());

			// cast physics POD index to index in physics engine
			studioData->objects[i].shapeCache[j] = shape;

			{
				CScopedMutex m(s_shapeCacheMutex);
				m_collisionShapes.append(shape);
			}

			studioData->shapes[nShape].cachedata = shape;
		}
	}
}

void CBulletStudioShapeCache::DestroyStudioCache( studioPhysData_t* studioData )
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

		{
			CScopedMutex m(s_shapeCacheMutex);
			m_collisionShapes.fastRemoveIndex(nShape);
		}
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