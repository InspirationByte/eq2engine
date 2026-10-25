# Physics overview — `shared/physics` (eqPhysics)

eqPhysics is the engine's own rigid body physics. It is built as `physicsLib` on top of the
**Bullet collision library** (`bullet2`): Bullet is used only for shapes, narrow-phase contact
generation and ray / convex tests. Dynamics, contact response, freezing, constraints and the
broadphase are eqPhysics code.

Design points:

- Made for a fixed timestep (the game calls `SimulateStep` with a constant delta).
- Positions use `FVector3D` (`framework/math/FVector.h`). `FReal` was a fixed-point type; it is
  `float` by default (`FLOAT_AS_FREAL`, see [framework_math.md](framework_math.md)). World size is
  limited to ±32767 units (`EQPHYS_MAX_WORLDSIZE`).
- Y is up. Gravity pulls along -Y and is set per body (9.81 by default).
- Collision detection for moving bodies may run as jobs on `CEqJobManager`.

| File | Contents |
|---|---|
| `eqPhysics_World.h/.cpp` | `CEqPhysicsWorld` — object lists, simulation step, line and convex sweep tests, surface params |
| `eqCollision_Object.h/.cpp` | `CEqCollisionObject` — shape, transform, contents / collide mask, flags, collision list |
| `eqPhysics_Body.h/.cpp` | `CEqRigidBody` — mass, inertia, velocities, forces, freezing, impulse response |
| `eqCollision_Pair.h` | `eqCollisionInfo` (ray / sweep result), `eqContactPair` (contact during a step), `eqCollisionPairData` (reported collision) |
| `eqCollision_Callback.h/.cpp` | `IEqPhysCallback` — per-object simulation and collision callbacks |
| `eqPhysics_Defs.h/.cpp` | Constants, `eqPhysCollisionFilter`, `eqPhysSurfParam` |
| `eqPhysics_Broadphase.h/.cpp` | `CEqPhysicsBroadphase` — two `btDbvt` trees (dynamic and fixed) |
| `eqPhysics_Contstraint.h`, `eqPhysics_PointConstraint.*`, `eqPhysics_MaxDistConstraint.*` | `IEqPhysicsConstraint` and point / max distance constraints |
| `eqPhysics_Controller.h`, `eqPhysics_HingeJoint.*` | `IEqPhysController` and the hinge joint built from constraints |
| `eqBulletIndexedMesh.h/.cpp` | `CEqBulletIndexedMesh` — triangle mesh with per-subpart material, for static world geometry |
| `BulletShapeCache.h/.cpp` | `CBulletStudioShapeCache` — builds Bullet shapes for EGF physics models |
| `BulletConvert.h` | `EqBulletUtils` vector / matrix / quaternion conversion |
| `public/physics/IStudioShapeCache.h` | `IStudioShapeCache` interface, `g_studioShapeCache`, `CEmptyStudioShapeCache` |

## World setup

```cpp
CEqPhysicsWorld* physics = PPNew CEqPhysicsWorld(*g_parallelJobs->GetJobMng());

physics->InitWorld();				// Bullet dispatcher, jobs (ph_useJobs)
physics->InitGrid(levelBounds);		// broadphase; objects added before are inserted now

// surface params, one KeyValues section per surface
physics->AddSurfaceParamFromKV("asphalt", asphaltSection);

CEqCollisionObject::GetSurfaceParamId = [](const char* name) {
	return g_physics->FindSurfaceParamID(name);
};

// ...

physics->DestroyGrid();
physics->DestroyWorld();			// clears lists, objects are not deleted
```

- `SimulateStep` and the ray tests do nothing until `InitGrid` has created the broadphase.
  `InitGrid` ignores its bounding box: the broadphase is a dynamic AABB tree, not a grid.
- The world never owns objects. The game creates and deletes bodies, collision objects,
  constraints and controllers, and removes them from the world before deleting.

### Surface params

A surface param (`eqPhysSurfParam`) describes a physics material:

```
asphalt
{
	friction		0.8;
	restitution		0.1;
	tireFriction	0.2;
	tireTraction	1.0;
	contents		1;		// bits this surface has
	collideMask		-1;		// contents it collides with
	word			"A";	// game-defined surface letter (sounds, effects)
}
```

The index of a param is its ID. Collision objects store an ID (`GetSurfParamId`), triangle meshes
store one per subpart, and ray results return it in `eqCollisionInfo::materialIndex`.
`FindSurfaceParam(name)`, `FindSurfaceParamID(name)`, `GetSurfaceParamByID(id)`.

## Collision objects

`CEqCollisionObject` is a shape with a transform. The same class is used for static objects and
ghost objects (triggers), and it is the base of `CEqRigidBody`.

```cpp
CEqCollisionObject* obj = PPNew CEqCollisionObject();
obj->Initialize(boxMins, boxMaxs);		// box
// Initialize(radius) - sphere, Initialize(radius, height) - cylinder,
// Initialize(points, margin) - convex hull,
// Initialize(physModel, objIdx) - EGF physics object,
// Initialize(indexedMesh, internalEdges) - triangle mesh (static geometry)

obj->SetTransform(Transform3D(rotation, position));
obj->SetContents(OBJECTCONTENTS_SOLID_GROUND);	// game-defined bits
obj->SetCollideMask(COLLIDEMASK_ALL);
obj->SetUserData(this);							// usually the game object

physics->AddStaticObject(obj);
```

- **Contents and collide mask:** two objects collide if `A.contents & B.collideMask` or
  `A.collideMask & B.contents` is non-zero. Bit meanings are defined by the game. Triangle mesh
  subparts are also filtered by their surface param's `contents` / `collideMask`.
- **Friction, restitution, ERP:** `SetFriction`, `SetRestitution`, `SetErp`. For static objects
  they multiply the surface param values. ERP is added to `ph_erp` for position correction.
- **Moving a static or ghost object:** `SetPosition` / `SetOrientation` / `SetTransform` only mark
  the bounds dirty. Call `physics->SetupCollisionObjectBroadphase(obj)` afterwards to update the
  broadphase. Rigid bodies are updated by the world every step.
- **Shapes from EGF models:** physics models (`.pod`) are loaded with the studio model, and
  `g_studioShapeCache->InitStudioCache` creates their shapes (see
  [rendering_egf_overview.md](rendering_egf_overview.md)). A game with eqPhysics registers a
  `CBulletStudioShapeCache` as `g_studioShapeCache`; samples without physics register
  `CEmptyStudioShapeCache`. A physics object with several shapes becomes a `btCompoundShape`.
  `Initialize(physModel, objIdx)` asserts if `CEqCollisionObject::GetSurfaceParamId` is not set.
- **Triangle meshes:** `CEqBulletIndexedMesh` wraps existing vertex and index buffers (they are not
  copied and must stay alive). `AddSubpart(firstIndex, numIndices, firstVertex, numVerts, materialId)`
  adds a batch with a surface param ID. `internalEdges = true` builds the Bullet internal edge
  info to reduce bumps on triangle edges, which is slow.

### Object flags — `ECollisionObjectFlags`

| Flag | Meaning |
|---|---|
| `COLLOBJ_NO_RAYCAST` | Skipped by line and sweep tests, unless the filter has `EQPHYS_FILTER_FLAG_FORCE_RAYCAST` |
| `COLLOBJ_COLLISIONLIST` | Keep collisions of the step in `GetCollisionList()` (up to `PHYSICS_COLLISION_LIST_MAX` = 8) |
| `COLLOBJ_DISABLE_COLLISION_CHECK` | The object does not look for collisions with dynamic objects itself, others still collide with it |
| `COLLOBJ_DISABLE_RESPONSE` | No collision response in either direction (includes `COLLOBJ_DISABLE_COLLISION_CHECK`) |
| `COLLOBJ_ISGHOST` | Ghost object, set by `AddGhostObject` |
| `COLLOBJ_MIGHT_MOVE` | Game marker for static objects moved by scripts, not used by eqPhysics itself |

### Ghost objects

`AddGhostObject(obj)` adds a trigger volume. It replaces the object's flags with
`COLLOBJ_ISGHOST | COLLOBJ_DISABLE_RESPONSE | COLLOBJ_NO_RAYCAST` (plus `COLLOBJ_COLLISIONLIST`
if no callback is attached). Bodies entering it report collisions without any response, so the
game can detect them through `IEqPhysCallback::OnCollide` or the collision list.

## Rigid bodies

```cpp
CEqRigidBody* body = PPNew CEqRigidBody();
body->Initialize(physModel, 0);
body->SetMass(1500.0f);					// also computes inertia from the shape
body->SetCenterOfMass(FVector3D(0, -0.2f, 0));
body->SetFriction(0.5f);
body->SetRestitution(0.2f);
body->SetContents(OBJECTCONTENTS_VEHICLE);
body->SetCollideMask(COLLIDEMASK_VEHICLE);
body->SetTransform(spawnTransform);
body->SetUserData(this);

physics->AddBody(body);					// moveable = true: simulated

// later
physics->RemoveBody(body);
delete body;
```

- **Forces and impulses** are accumulated and applied on the next integration:
  `ApplyLinearForce`, `ApplyLinearImpulse`, `ApplyAngularForce`, `ApplyAngularImpulse`,
  `ApplyForce(relPos, force)`, `ApplyImpulse(relPos, impulse)`, and world-position variants
  `ApplyWorldForce`, `ApplyWorldImpulse`.
- **Velocities:** `GetLinearVelocity`, `GetAngularVelocity`, `GetVelocityAtWorldPoint`,
  `SetLinearVelocity`, `SetAngularVelocity`. `SetLinearFactor` / `SetAngularFactor` lock axes.
- **Gravity:** `SetGravity(value)` per body, along -Y.
- **Interpolation:** `GetPrevPosition` / `GetPrevOrientation` return the transform before the
  last step.
- **Min frame time:** `SetMinFrameTime(time, ignoreMotion)` makes a body integrate and test
  collisions less often than every step (for distant or cheap objects).
- `CEqRigidBody::CopyValues(dest, src)` copies the simulation state (for example when replacing
  a body).

### Moveable list and freezing

`AddBody(body, moveable)` adds a body to the dynamic list. With `moveable = true` it is also put
in the moveable list, which is what the step simulates (`AddToMoveableList` /
`RemoveFromMoveableList`). `IEqPhysCallback::OnStartMove` / `OnStopMove` are called on these
changes.

A body freezes automatically when its linear and angular velocity stay low for 0.5 s. A frozen
body is not integrated and not tested for collisions, but stays in the moveable list. Other
bodies hitting it wake it up.

| Call / flag | Effect |
|---|---|
| `TryWake(velocityCheck)` | Wakes up the body unless `BODY_FORCE_FREEZE` is set (with `velocityCheck`, only if it moves) |
| `Wake()` | Wakes up the body and clears the forced freeze |
| `Freeze(preserveForces)` | Forced freeze (`BODY_FORCE_FREEZE`): nothing wakes the body except `Wake()` |
| `IsFrozen()` | Frozen by timeout or by force |
| `BODY_NO_AUTO_FREEZE` | Never freeze automatically |
| `BODY_DISABLE_DAMPING` | No angular damping |
| `BODY_INFINITEMASS` | The body receives no response from other bodies (trains) |
| `BODY_ISCAR` | Car body: car vs car contacts use `ph_carVsCarErp` and are reported with `COLLPAIRFLAG_NO_SOUND` on the second car |

## Simulation step

```cpp
// fixed timestep, possibly several iterations per frame
for (int i = 0; i < numIterations; ++i)
	physics->SimulateStep(PHYSICS_FRAME_INTERVAL, i);
```

`SimulateStep(deltaTime, iteration)` runs these stages:

1. `PreApply` of enabled constraints, `Update` of enabled controllers.
2. `IEqPhysCallback::PreSimulate` for every moveable body. This runs as `PreSimulateJob` when
   `ph_useJobs` was enabled at `InitWorld`, so callbacks must be thread-safe.
3. Integrate moveable bodies (forces, gravity, velocities, freeze check) and update their
   broadphase bounds. Bodies that are not frozen are collected for the next stages.
4. `GetPreIntegrateEvent()` handlers.
5. Collision detection for the collected bodies (`CollisionDetectionJob` with jobs): broadphase
   box query, then Bullet contact generation against static, ghost and dynamic objects. Contacts
   are stored in the body (up to 32 per step).
6. Recalculate body velocities from the movement.
7. Process contact pairs: `OnPreCollide`, position correction (ERP), impulse response with
   friction, then `OnCollide` and the collision lists. Then `PostSimulate` of the body.
8. `GetPostIntegrateEvent()` handlers.
9. `Apply` of enabled constraints (one pass).

### Callbacks — `IEqPhysCallback`

Derive from `IEqPhysCallback` and attach it to one object (`Attach(obj)`, protected, normally
called by the game object that owns both):

| Method | When |
|---|---|
| `OnStartMove()` / `OnStopMove()` | Body added to / removed from the moveable list |
| `PreSimulate(dt)` | Before integration. Apply forces here (engine, wheels, buoyancy) |
| `PostSimulate(dt)` | After the body's contacts are processed |
| `OnPreCollide(eqContactPair& pair)` | Before response. The pair can be changed, for example by setting `COLLPAIRFLAG_OBJECTA_NO_RESPONSE` / `COLLPAIRFLAG_OBJECTB_NO_RESPONSE` |
| `OnCollide(const eqCollisionPairData& pair)` | After response, with `appliedImpulse`, `impactVelocity`, position and normal. `bodyA` is always the object the callback is attached to |

`COLLPAIRFLAG_USER_PROCESSED` .. `COLLPAIRFLAG_USER_PROCESSED4` are free for game use.

## Line and convex sweep tests

```cpp
eqCollisionInfo coll;
eqPhysCollisionFilter filter(playerBody);		// exclude own body

if (physics->TestLineCollision(start, end, coll, COLLIDEMASK_RAYCAST, &filter))
{
	// coll.position, coll.normal, coll.fract (0..1 along the line),
	// coll.hitobject, coll.materialIndex (surface param ID)
}

// sweep a Bullet shape, e.g. one of an object's shapes
physics->TestConvexSweepCollision(shape, rotation, start, end, coll, mask, &filter);
```

- Only objects whose contents match `rayMask` are tested. Triangle mesh subparts are tested if
  their surface param's `collideMask` matches `rayMask`.
- Back faces of triangle meshes are ignored by line tests.
- **Filter** (`eqPhysCollisionFilter`): up to 8 objects (`AddObject`), `type`
  `EQPHYS_FILTER_TYPE_EXCLUDE` (default) or `EQPHYS_FILTER_TYPE_INCLUDE_ONLY`, `flags` to choose
  static and / or dynamic objects, `EQPHYS_FILTER_FLAG_BY_USERDATA` to compare user data
  instead of object pointers, `EQPHYS_FILTER_FLAG_FORCE_RAYCAST`, and `ignoreContentsMask`.
- `TestLineSingleObject` / `TestConvexSweepSingleObject` test one object.
- Tests take a read lock and can be called from several threads, including from `PreSimulate`
  callbacks.

## Constraints and controllers

`IEqPhysicsConstraint` adds impulses between two bodies: `PreApply(dt)` at the start of the step,
`Apply(dt)` at the end. Constraints are created disabled.

| Class | Purpose |
|---|---|
| `CEqPointConstraint` | Keeps two body points together, `allowedDistance` and `timescale` for softness |
| `CEqMaxDistConstraint` | Keeps two body points within `maxDistance` |
| `CEqHingeJoint` (controller) | Hinge with angle limits from one point and two max distance constraints (car doors, hoods). `Break()` / `Restore()` remove / restore the limit, `SetExtraTorque` |

```cpp
CEqHingeJoint* hinge = PPNew CEqHingeJoint();
hinge->Init(carBody, doorBody, hingeAxis, hingePosRelToCar, halfWidth,
	DEG2RAD(70.0f), DEG2RAD(0.0f), sidewaysSlack);
physics->AddController(hinge);		// adds its constraints to the world
hinge->SetEnabled(true);
```

`CONSTRAINT_FLAG_BODYA_NOIMPULSE` / `CONSTRAINT_FLAG_BODYB_NOIMPULSE` apply impulses to one body
only. `IEqPhysController` is any object with `Update(dt)` called at the start of each step while
enabled.

## Threading

- The broadphase is protected by a read / write lock. Adding and removing objects takes the write
  lock, but the object lists are not locked: add and remove objects outside `SimulateStep`.
- With `ph_useJobs 1` (read at `InitWorld`), `PreSimulate` callbacks and collision detection run
  on worker threads. Collision response and `OnCollide` / `PostSimulate` run on the calling
  thread.

## Console variables and debugging

| ConVar | Purpose |
|---|---|
| `ph_useJobs` | Use jobs for `PreSimulate` and collision detection (read at `InitWorld`) |
| `ph_erp` | Position correction factor for contacts (0.15) |
| `ph_carVsCarErp` | Same for car vs car contacts (0.15) |
| `ph_studioShapeMargin` | Collision margin of shapes created from EGF models (0.05) |
| `ph_showContacts` | Draw contact points, normals and depth |
| `ph_showCollisionResponses` | Draw applied impulses |
| `ph_debugRigidBody` | Draw body state text |

`DebugDrawBodies(mode)` draws body bounds through the debug overlay, frozen bodies in green.
Modes 1–3 draw oriented boxes (mode 3 also draws static objects), modes 0 and 4 draw world-space
AABBs of bodies.

## dkPhysics (legacy)

`shared/dkphysics` (`dkPhysicsLib`, `public/dkphysics`) is the old Equilibrium 1 "DarkTech"
physics interface, now running on Jolt. It is deprecated and built only with tools
(`ENABLE_TOOLS`), because EGFMan2 still uses it to preview physics models and ragdolls. It will
be removed. Do not use it in new code; use eqPhysics.
