//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - eqPhysics world
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IEqParallelJobs.h"
#include "utils/KeyValues.h"

#include "physics/eqPhysics_World.h"
#include "physics/eqCollision_Object.h"
#include "physics/eqBulletIndexedMesh.h"
#include "physics/BulletShapeCache.h"
#include "physics/PhysicsCollisionGroup.h"

#include "egfman2_physics.h"

static constexpr const float PHYSICS_FRAME_INTERVAL = 1.0f / 60.0f;
static constexpr const int PHYSICS_MAX_STEPS_PER_FRAME = 4;
static constexpr const float PHYSICS_FLOOR_SIZE = 512.0f;
static constexpr const int PHYSICS_FLOOR_GRID = 32;
static constexpr const int PHYSICS_FLOOR_VERTS = PHYSICS_FLOOR_GRID * PHYSICS_FLOOR_GRID;
static constexpr const int PHYSICS_FLOOR_INDICES = (PHYSICS_FLOOR_GRID - 1) * (PHYSICS_FLOOR_GRID - 1) * 6;
static constexpr const char* PHYSICS_DEFAULT_SURFACE = "default";

static CBulletStudioShapeCache	s_shapeCache;
static CEqPhysicsWorld*			s_physWorld = nullptr;
static CEqCollisionObject*		s_floorObj = nullptr;
static CEqBulletIndexedMesh*	s_floorMesh = nullptr;
static float					s_dtAccumulator = 0.0f;

// mesh keeps pointers to the floor geometry
static Vector3D					s_floorVertices[PHYSICS_FLOOR_VERTS];
static uint16					s_floorIndices[PHYSICS_FLOOR_INDICES];

// Bullet triangle normal is cross(v1 - v0, v2 - v0), floor triangles face +Y
static void BuildFloorGeometry()
{
	for (int c = 0; c < PHYSICS_FLOOR_GRID; c++)
	{
		for (int r = 0; r < PHYSICS_FLOOR_GRID; r++)
		{
			const float posX = (static_cast<float>(r) / (PHYSICS_FLOOR_GRID - 1) - 0.5f) * 2.0f * PHYSICS_FLOOR_SIZE;
			const float posZ = (static_cast<float>(c) / (PHYSICS_FLOOR_GRID - 1) - 0.5f) * 2.0f * PHYSICS_FLOOR_SIZE;
			s_floorVertices[r + c * PHYSICS_FLOOR_GRID] = Vector3D(posX, 0.0f, posZ);
		}
	}

	int numIndices = 0;
	for (int c = 0; c < PHYSICS_FLOOR_GRID - 1; c++)
	{
		for (int r = 0; r < PHYSICS_FLOOR_GRID - 1; r++)
		{
			const uint16 index1 = r + c * PHYSICS_FLOOR_GRID;
			const uint16 index2 = r + (c + 1) * PHYSICS_FLOOR_GRID;
			const uint16 index3 = (r + 1) + c * PHYSICS_FLOOR_GRID;
			const uint16 index4 = (r + 1) + (c + 1) * PHYSICS_FLOOR_GRID;

			s_floorIndices[numIndices++] = index1;
			s_floorIndices[numIndices++] = index2;
			s_floorIndices[numIndices++] = index3;

			s_floorIndices[numIndices++] = index3;
			s_floorIndices[numIndices++] = index2;
			s_floorIndices[numIndices++] = index4;
		}
	}
}

void PhysicsLibInit()
{
	g_eqCore->RegisterInterface(&s_shapeCache);
}

void PhysicsLibShutdown()
{
	g_eqCore->UnregisterInterface<CBulletStudioShapeCache>();
}

void PhysicsSceneInit()
{
	ASSERT(s_physWorld == nullptr);
	s_physWorld = PPNew CEqPhysicsWorld(*g_parallelJobs->GetJobMng());

	// models refer to game surface params, all of them fall back to the default one
	KVSection defaultSurfParam;
	defaultSurfParam.SetKey("friction", 0.8f);
	defaultSurfParam.SetKey("restitution", 0.1f);
	defaultSurfParam.SetKey("contents", -1);
	defaultSurfParam.SetKey("collideMask", -1);
	s_physWorld->AddSurfaceParamFromKV(PHYSICS_DEFAULT_SURFACE, defaultSurfParam);

	CEqCollisionObject::GetSurfaceParamId = [](const char* surfParamName) -> int {
		const int surfParamId = s_physWorld->FindSurfaceParamID(surfParamName);
		return surfParamId != -1 ? surfParamId : s_physWorld->FindSurfaceParamID(PHYSICS_DEFAULT_SURFACE);
	};

	s_physWorld->InitWorld();
	s_physWorld->InitGrid(BoundingBox(Vector3D(-PHYSICS_FLOOR_SIZE), Vector3D(PHYSICS_FLOOR_SIZE)));

	BuildFloorGeometry();

	// Bullet classes have their own operator new
	s_floorMesh = new CEqBulletIndexedMesh(reinterpret_cast<ubyte*>(s_floorVertices), sizeof(Vector3D),
											reinterpret_cast<ubyte*>(s_floorIndices), sizeof(uint16),
											PHYSICS_FLOOR_VERTS, PHYSICS_FLOOR_INDICES);
	s_floorMesh->AddSubpart(0, PHYSICS_FLOOR_INDICES, 0, PHYSICS_FLOOR_VERTS, s_physWorld->FindSurfaceParamID(PHYSICS_DEFAULT_SURFACE));

	s_floorObj = PPNew CEqCollisionObject();
	s_floorObj->Initialize(s_floorMesh, false);

	// broadphase picks up only objects with updated bounds
	s_floorObj->SetTransform(Transform3D(vec3_zero, qidentity));
	s_floorObj->SetContents(COLLISION_GROUP_WORLD);
	s_floorObj->SetCollideMask(0);
	s_physWorld->AddStaticObject(s_floorObj);

	s_dtAccumulator = 0.0f;
}

void PhysicsSceneShutdown()
{
	if (!s_physWorld)
		return;

	s_physWorld->RemoveStaticObject(s_floorObj);
	SAFE_DELETE(s_floorObj);
	SAFE_DELETE(s_floorMesh);

	s_physWorld->DestroyGrid();
	s_physWorld->DestroyWorld();
	SAFE_DELETE(s_physWorld);

	CEqCollisionObject::GetSurfaceParamId = nullptr;
}

void PhysicsSimulate(float frameTime)
{
	s_dtAccumulator = min(s_dtAccumulator + frameTime, PHYSICS_FRAME_INTERVAL * PHYSICS_MAX_STEPS_PER_FRAME);

	int iteration = 0;
	while (s_dtAccumulator >= PHYSICS_FRAME_INTERVAL)
	{
		s_physWorld->SimulateStep(PHYSICS_FRAME_INTERVAL, iteration++);
		s_dtAccumulator -= PHYSICS_FRAME_INTERVAL;
	}
}

CEqPhysicsWorld& PhysicsGetWorld()
{
	ASSERT(s_physWorld);
	return *s_physWorld;
}
