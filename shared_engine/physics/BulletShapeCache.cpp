//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
bool CBulletStudioShapeCache::IsShapeCachePresent( StudioPhyShapeData& shapeInfo )
{
	CScopedMutex m(s_shapeCacheMutex);
	if (arrayFindIndex(m_collisionShapes, reinterpret_cast<btCollisionShape*>(shapeInfo.cacheRef)) != -1)
		return true;

	return false;
}

// initializes whole studio shape model with all objects
void CBulletStudioShapeCache::InitStudioCache(StudioPhysData& studioData)
{
	for (StudioPhyShapeData& shapeData : studioData.shapes)
	{
		const physgeominfo_t& shapeInfo = shapeData.desc;

		btCollisionShape* shape = ShapeCache_GenerateBulletShape(
			studioData.vertices,
			ArrayCRef(studioData.indices.ptr() + shapeInfo.startIndices, shapeInfo.numIndices),
			static_cast<EPhysShapeType>(shapeInfo.type));

		{
			CScopedMutex m(s_shapeCacheMutex);
			m_collisionShapes.append(shape);
			shapeData.cacheRef = shape;
		}
	}

	for (StudioPhyObjData& obj : studioData.objects)
	{
		for (int i = 0; i < obj.desc.numShapes; ++i)
			obj.shapeCacheRefs[i] = studioData.shapes[i].cacheRef;
	}
}

void CBulletStudioShapeCache::DestroyStudioCache(StudioPhysData& studioData)
{
	for(StudioPhyShapeData& shapeData : studioData.shapes)
	{
		const int shapeIdx = arrayFindIndex(m_collisionShapes, reinterpret_cast<btCollisionShape*>(shapeData.cacheRef));

		if (shapeIdx == -1)
			continue;
		btCollisionShape* shape = m_collisionShapes[shapeIdx];

		if(shape->getUserPointer() )
		{
			btTriangleIndexVertexArray* mesh = (btTriangleIndexVertexArray*)shape->getUserPointer();
			delete mesh;
		}

		delete shape;

		{
			CScopedMutex m(s_shapeCacheMutex);
			m_collisionShapes.fastRemoveIndex(shapeIdx);
		}
	}
}

// does all shape cleanup
void CBulletStudioShapeCache::Cleanup_Invalidate()
{
	CScopedMutex m(s_shapeCacheMutex);
	for(btCollisionShape* shape : m_collisionShapes)
	{
		if(shape->getUserPointer() )
		{
			btTriangleIndexVertexArray* mesh = (btTriangleIndexVertexArray*)shape->getUserPointer();
			delete mesh;
		}

		delete shape;
	}

	m_collisionShapes.clear(true);
}