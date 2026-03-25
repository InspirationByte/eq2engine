//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics model cache for bullet physics
//				Generates real shapes for Bullet Collision
///////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"

#include "DkJoltPCH.h"
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>

#include "DkJoltShapeCache.h"
#include "egf/physmodel.h"

static Threading::CEqMutex s_shapeCacheMutex;

DECLARE_CVAR(ph_studioShapeMargin, "0.05", "Studio model shape marginal", CV_CHEAT);	// same as JPH::cDefaultConvexRadius

// makes and caches shape. IsConvex defines that it was convex or not (also for internal use)
static JPH::Ref<JPH::Shape> jphGenerateCollisionShape(ArrayCRef<Vector3D> vertices, ArrayCRef<int> indices, EPhysShapeType type)
{
	const float margin = ph_studioShapeMargin.GetFloat();

	switch(type)
	{
		case PHYSSHAPE_TYPE_CONCAVE:
		case PHYSSHAPE_TYPE_MOVABLECONCAVE:
		{
			JPH::VertexList jphVertList;
			JPH::IndexedTriangleList jphTriList;
			jphVertList.reserve(vertices.numElem());

			for (int i = 0; i < vertices.numElem(); ++i)
				jphVertList.push_back(Convert::ToFloat3(vertices[i]));

			jphTriList.reserve(indices.numElem() / 3);
			for (int i = 0; i < indices.numElem(); i += 3)
			{
				JPH::IndexedTriangle jphTri(indices[i], indices[i + 1], indices[i + 2]);
				jphTriList.push_back(jphTri);
			}

			JPH::MeshShapeSettings jphMeshSettings(jphVertList, jphTriList);
			auto jphResult = jphMeshSettings.Create();
			return jphResult.Get();
		}
		case PHYSSHAPE_TYPE_CONVEX:
		{
			JPH::Array<JPH::Vec3> jphVertList;
			jphVertList.reserve(vertices.numElem());
			for (int i = 0; i < vertices.numElem(); ++i)
				jphVertList.push_back(Convert::ToVec3(vertices[i]));

			JPH::ConvexHullShapeSettings jphHullSettings(jphVertList, ph_studioShapeMargin.GetFloat());
			auto jphResult = jphHullSettings.Create();
			return jphResult.Get();
		}
	}

	MsgError("InternalGenerateShape: Shape type %d is invalid!\n", type);

	return nullptr;
}

bool CDkJoltStudioShapeCache::IsInitialized() const
{
	return true; 
}

// checks the shape is initialized for the cache
bool CDkJoltStudioShapeCache::IsShapeCachePresent( StudioPhyShapeData& shapeInfo )
{
	Threading::CScopedMutex m(s_shapeCacheMutex);
	const int idx = arrayFindIndex(m_collisionShapes, reinterpret_cast<JPH::Shape*>(shapeInfo.cacheRef));
	return idx >= 0;
}

// initializes whole studio shape model with all objects
void CDkJoltStudioShapeCache::InitStudioCache(StudioPhysData& studioData)
{
	for (StudioPhyShapeData& shapeData : studioData.shapes)
	{
		const physgeominfo_t& shapeInfo = shapeData.desc;

		JPH::Ref<JPH::Shape> shape = jphGenerateCollisionShape(
			studioData.vertices,
			ArrayCRef(studioData.indices.ptr() + shapeInfo.startIndices, shapeInfo.numIndices),
			static_cast<EPhysShapeType>(shapeInfo.type));

		{
			Threading::CScopedMutex m(s_shapeCacheMutex);
			shape->AddRef();
			m_collisionShapes.append(shape);
			shapeData.cacheRef = shape;
		}
	}

	for (StudioPhyObjData& obj : studioData.objects)
	{
		for (int i = 0; i < obj.desc.numShapes; ++i)
			obj.shapeCacheRefs[i] = studioData.shapes[obj.desc.shapeIndex[i]].cacheRef;
	}
}

void CDkJoltStudioShapeCache::DestroyStudioCache(StudioPhysData& studioData)
{
	for(StudioPhyShapeData& shapeData : studioData.shapes)
	{
		Threading::CScopedMutex m(s_shapeCacheMutex);
		const int shapeIdx = arrayFindIndex(m_collisionShapes, reinterpret_cast<JPH::Shape*>(shapeData.cacheRef));
		if (shapeIdx == -1)
			continue;

		m_collisionShapes[shapeIdx]->Release();
		m_collisionShapes.fastRemoveIndex(shapeIdx);
	}
}

// does all shape cleanup
void CDkJoltStudioShapeCache::Cleanup_Invalidate()
{
	Threading::CScopedMutex m(s_shapeCacheMutex);

	for(JPH::Shape* jphShape : m_collisionShapes)
		jphShape->Release();
	m_collisionShapes.clear(true);
}