//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

DECLARE_CVAR(ph_studioShapeMargin, "0.05", "Studio model shape marginal", CV_CHEAT);

// makes and caches shape. IsConvex defines that it was convex or not (also for internal use)
static btCollisionShape* ShapeCache_GenerateBulletShape(ArrayCRef<Vector3D> vertices, ArrayCRef<int> indices, EPhysShapeType type)
{
	const float margin = ph_studioShapeMargin.GetFloat();

	switch(type)
	{
		case PHYSSHAPE_TYPE_CONCAVE:
		case PHYSSHAPE_TYPE_MOVABLECONCAVE:
		{
			btTriangleIndexVertexArray* pTriMesh = new btTriangleIndexVertexArray(
				indices.numElem() / 3,
				(int*)indices.ptr(),
				sizeof(int)*3,
				vertices.numElem(),
				(float*)vertices.ptr(),
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

			for (int i = 0; i < indices.numElem(); i++)
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
		studioPhysObject_t& obj = studioData->objects[i];
		for(int j = 0; j < obj.object.numShapes; j++)
		{
			const int nShape = obj.object.shapeIndex[j];

			btCollisionShape* shape = ShapeCache_GenerateBulletShape(
									ArrayCRef(studioData->vertices, studioData->numVertices),
									ArrayCRef(studioData->indices + studioData->shapes[nShape].shapeInfo.startIndices, studioData->shapes[nShape].shapeInfo.numIndices),
									(EPhysShapeType)studioData->shapes[nShape].shapeInfo.type);

			// cast physics POD index to index in physics engine
			obj.shapeCache[j] = shape;

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
		const int nShape = arrayFindIndex(m_collisionShapes, (btCollisionShape*)studioData->shapes[i].cachedata);

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