# EGF rendering overview — `shared/studio`

Runtime side of EGF models (file formats: [public_egf_overview.md](public_egf_overview.md)): loading
and caching models and motion packages, GPU geometry, materials and skins, LOD selection and
drawing through the material system.

| Project | Files | Contents |
|---|---|---|
| `studioFileLib` | `shared/studiofile/StudioLoader.*` | `Studio_LoadModel`, `Studio_LoadMotionData`, `Studio_LoadPhysModel` and `Studio_Free*` — file loaders without GPU |
| `studioLib` | `shared/studio/StudioCache.*` | `g_studioCache` (`CStudioCache`) — model and motion package cache |
| | `shared/studio/StudioGeom.*` | `CEqStudioGeom` — a loaded model with GPU buffers |
| | `shared/studio/StudioVertex.*` | `EGFHwVertex` — GPU vertex streams |
| `public/render/StudioRenderDefs.h` | | `RenderBoneTransform`, `RenderBoneTransformID` (`'BSKN'`) |

Skeletal animation on top of it: [animating_egf_overview.md](animating_egf_overview.md).

## Studio cache — `g_studioCache`

```cpp
g_studioCache->Init(g_parallelJobs->GetJobMng());	// once, before loading
g_studioCache->PrecacheModel("models/error.egf");	// index 0 = fallback model

const int modelIdx = g_studioCache->PrecacheModel("models/cars/taxi");	// ".egf" added if missing
CEqStudioGeom* geom = g_studioCache->GetModel(modelIdx);
...
g_studioCache->Shutdown();
```

- `PrecacheModel(name)` returns a cache index (`STUDIOCACHE_INVALID_IDX` = `-1` if the file
  cannot be read). The same name returns the same index.
- `GetModel(idx)` returns the model, or **model 0** for an invalid index or a model that failed to
  load. Precache the error model first so that index 0 is a valid fallback.
- `FreeModel(idx)` deletes a model, `ReleaseCache()` everything (models are deleted even though
  `CEqStudioGeom` is ref-counted).
- `PrecacheMotionData(name, requestedBy)`, `GetMotionData(idx)`, `FreeMotionData(idx)` — the same
  for `.mop` packages (`StudioMotionData`). Models load their packages automatically.
- `GetGeomVertexFormat()` — the default vertex format `EGFVertex` with all four EGF streams.
- `GetErrorMaterial()` — `Skinned` material with the `error` texture.
- `studio_cacheInfo` (cheat) prints loaded models and packages; `job_modelLoader 0` loads models
  synchronously.

## Loading — `CEqStudioGeom`

`PrecacheModel` reads the `.egf` file immediately; the rest runs as jobs on the cache job manager
(`job_modelLoader 1`):

| Job | Work |
|---|---|
| `LoadEGF` | Materials (with search paths), `.skin` file, physics data (`.pod`), skeleton (`StudioJoint` hierarchy, IK links) |
| `LoadEGFHWGeom` | Vertex and index buffers |
| `LoadStudioMotion` | `<model>.mop` next to the model, packages listed in the EGF, packages added with `AddMotionPackage` |
| `FinishSyncJob` | Sets `MODEL_LOAD_OK` and completes the loading future |

- `GetLoadingState()` — `MODEL_LOAD_IN_PROGRESS`, `MODEL_LOAD_OK`, `MODEL_LOAD_ERROR`;
  `GetLoadingFuture()`; `EGF_LOADING_CRITICAL_SECTION(geom)` waits for loading.
- `GetStudioHdr()` is valid right after `PrecacheModel`. Materials, skins, joints, physics data,
  motion packages, the bounding box and GPU buffers are filled by the jobs: wait for the loading
  future (or check `MODEL_LOAD_OK`) before using them. `GetPhysData()` / `GetMotionDataIdxs()` /
  `GetBoundingBox()` only wait for the header, not for the jobs. The game initializes objects that
  use a model after its loading future completes; `CAnimatingEGF::InitAnimating` asserts
  `MODEL_LOAD_OK`.
- `AddMotionPackage(file)` — extra `.mop` for this model (queued if the model is still loading).

Accessors: `GetName`, `GetNameHash`, `GetCacheIdx`, `GetStudioHdr` (raw EGF data, lookup helpers
`Studio_FindBone`, `Studio_FindBodyGroupId`, ... from `egf/studiomodel.h`), `GetPhysData`,
`GetJoints` (bind pose: local, absolute and inverse absolute transforms), `GetMotionDataIdxs`,
`GetHwGeomRefs` (index ranges per mesh), `GetVertexBuffer(stream)`, `GetIndexBuffer`,
`GetIndexFormat`, `GetBoundingBox`.

Also: `CheckIntersectionWithRay(start, dir, bodyGroupFlags, lod)` and `MakeDecal(...)`.

## GPU geometry — `EGFHwVertex`

Vertices are split into streams; a model has only the streams its data needs:

| Stream (`VertexStreamId`) | Struct | Data |
|---|---|---|
| `VERT_POS_UV` | `PositionUV` | Position (half4), UV (half2) |
| `VERT_TBN` | `TBN` | Tangent, binormal, normal (half) |
| `VERT_BONEWEIGHT` | `BoneWeights` | Up to 4 bone indices and weights |
| `VERT_COLOR` | `Color` | Packed RGBA |

Each stream struct has `GetVertexLayoutDesc()`. A layout's `userId` contains `EGF_FLAG | stream`,
which tells `Draw` (and GRIM archetypes) which EGF buffer to bind to it. Custom vertex formats
combine the EGF layouts they need with their own streams (for example an instance stream, see
[rendering_grim_overview.md](rendering_grim_overview.md)).

## Materials and skins

- Materials are found by name in the material search paths stored in the EGF (first match wins).
  They are requested with the instance format set by `CEqStudioGeom::SetInstanceFormatId` (`0` by
  default, see [rendering_matsystem_shaders_overview.md](rendering_matsystem_shaders_overview.md)).
  A material whose shader does not set `MATERIAL_FLAG_SKINNED` produces a warning.
- Skins are material sets: skins compiled into the EGF are numbered after the base skin; a
  `<model>.skin` file adds named skins (see [public_egf_overview.md](public_egf_overview.md)).
  `GetSkinCount()`, `GetSkinIdx(name)` (only skins from the `.skin` file have names),
  `GetMaterial(materialIdx, skinIdx)`, `GetMaterials(skinIdx)`.
- `QueueMaterialsLoading()` starts loading all materials of the model.

## LODs

- `SelectLod(distance)` — the last LOD whose switch distance (× `r_egf_LodScale`) is below the
  distance; LODs flagged `STUDIO_LOD_FLAG_MANUAL` are skipped. `r_egf_LodTest` forces a LOD,
  `r_egf_LodStart` skips the first LODs in the search.
- `FindManualLod(value)` — finds a manual LOD by its stored value.
- A body group without a mesh for the requested LOD falls back to the nearest lower LOD.

## Drawing

```cpp
RenderBoneTransform bones[128];
const int numBones = geom->ConvertBoneMatricesToQuaternions(animating.GetBoneMatrices(), bones);

CEqStudioGeom::DrawProps drawProps;
drawProps.bodyGroupFlags = bodyGroups;		// bit mask; 0 draws nothing
drawProps.skinIdx = skin;
drawProps.lod = geom->SelectLod(distanceToCamera);
if (numBones)
	drawProps.boneTransforms = g_matSystem->GetTransientUniformBuffer(bones, numBones * sizeof(RenderBoneTransform));

MeshInstanceData instData;
instData.count = 1;

g_matSystem->SetMatrix(MATRIXMODE_WORLD, worldMatrix);
geom->Draw(drawProps, instData, RenderPassContext(rendPassRecorder, nullptr));
```

`Draw` builds one `RenderDrawCmd` and submits every mesh of the selected body groups and LOD
through `g_matSystem->SetupDrawCommand`:

- Vertex format: `drawProps.vertexFormat` or the default `EGFVertex`. Only the EGF streams the
  model has are bound; an instance stream is bound when `instData` has a buffer.
- Skinning: when the model has bone weights and `boneTransforms` is set, the bone buffer is passed
  as uniform buffer `'BSKN'` and the format id becomes `<format>Skinned` (e.g. `EGFVertexSkinned`),
  so shaders compile a skinned variant. `ConvertBoneMatricesToQuaternions` converts animated
  bone matrices to bind-pose-relative quaternion + origin pairs (`RenderBoneTransform`).
- Material filter: `materialFlags` with `excludeMaterialFlags` draws only (or skips) meshes whose
  material has these flags (e.g. separate passes for transparent materials); `skipMaterials`
  leaves the material to the caller.
- Callbacks: `setupDrawCmd(drawCmd)` once before drawing (add uniform buffers, change the format),
  `setupBodyGroup(drawCmd, material, bodyGroup, meshIndex)` before each mesh.

For many instances of the same models use GRIM instead of per-object `Draw`.

## Users

| Code | Use |
|---|---|
| `shared/animating` | Skeleton and animation on `CEqStudioGeom` ([animating_egf_overview.md](animating_egf_overview.md)) |
| `shared/grim` | `CreateStudioDrawArchetype` — EGF models as GPU-driven archetypes |
| `shared/physics`, `shared/dkphysics` | Physics shapes from the model's POD data |
| `tools/egfMan2` (`CAnimatedModel`) | Model viewer: animation, LODs, body groups, skins, bones |
| `samples/GPUDriven` | Loads all models in `models/` and draws them with GRIM |
