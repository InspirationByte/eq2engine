# EGF overview — `public/egf`

**EGF — Entity Geometry Format.** (Older headers and comments call it "Equilibrium Geometry
Format" / "Equilibrium Graphics File"; the current name is Entity Geometry Format.)

`public/egf/` defines the binary file formats for models, animations and physics shapes. The
headers describe on-disk structures only; loading and runtime use live in `shared/`.
Include `egf/model.h`, which pulls in all format headers.

## File types

| Extension | Constant | Header | Contents | Built by |
|---|---|---|---|---|
| `.egf` | `s_egfGeomExt` | `studiomodel.h` | Model: meshes, LODs, body groups, materials, bones, attachments, IK chains | `tools/egfca` |
| `.mop` | `s_egfMotionPackageExt` | `motionpackage.h` | Motion package: animations, sequences, events, pose controllers | `tools/animca` |
| `.pod` | `s_egfPhysicsObjectExt` | `physmodel.h` | POD (Physics Object Data): collision shapes, objects, joints | `tools/egfca` |
| `.skin` | `s_egfSkinExt` | — | Optional text KeyValues file next to the model with additional skins (material groups) | hand-written (compiler support planned) |

Tools:

| Tool | Purpose |
|---|---|
| `tools/egfca` (egfCA) | EGF compiler: builds `.egf` models and `.pod` physics object data from source models and a compile script |
| `tools/animca` (animCA) | Motion package compiler: builds `.mop` animation packages |
| `tools/egfMan2` | Model viewer (Windows, sysLib + ImGui) |

Companion files are found by the model name: `models/car.egf` automatically uses
`models/car.mop` (if it exists) and `models/car.skin`. Skins defined in the `egfca` script are
stored inside the `.egf` itself; a `.skin` file adds more skins without recompiling:

```
materialPath	"models/cars/";
red				"car_body_red" "car_glass" "car_wheels";
blue			"car_body_blue" "car_glass" "car_wheels";
```

Each skin lists one material per model material, in the model's material order.

Source model formats (ESM/ESA, FBX, OBJ) are converted by the tools through `egfLib`
(`shared/egf/`, tools only); the Blender add-on in `tools/imp-exp blender/` exports ESM/ESA.

## Model — `studiomodel.h`

- Signature `EQUILIBRIUM_MODEL_SIGNATURE` (`'EQGF'`), version `EQUILIBRIUM_MODEL_VERSION` (13).
- `studioHdr_t` is the whole file loaded into memory; sections are reached by offsets through
  accessor methods (`pBone(i)`, `pBodyGroups(i)`, `pIkChain(i)`, ...).

Structure:

```
studioHdr_t
├── body groups (studioBodyGroup_t)  ── name → LOD model index
├── LOD models (studioLodModel_t)    ── mesh group index per LOD (up to MAX_MODEL_LODS = 8)
├── LOD params (studioLodParams_t)   ── switch distances, flags
├── mesh groups (studioMeshGroupDesc_t)
│   └── meshes (studioMeshDesc_t)    ── material index, primitive type, vertex streams, indices
├── materials (studioMaterialDesc_t) + material search paths
├── motion packages (motionPackageDesc_t) ── .mop files used by the model
├── bones (studioBoneDesc_t)         ── name, parent, rest position/rotation
├── transforms / attachments (studioTransform_t)
└── IK chains (studioIkChain_t, studioIkLink_t)
```

- Vertex data is split into streams (`EStudioVertexStreamType`): position + UV, TBN, bone weights
  (up to `MAX_MODEL_VERTEX_WEIGHTS` = 4), color. `vertexType` flags of a mesh tell which streams
  exist.
- Runtime limits: `MAX_STUDIOMATERIALS` (32) materials, `MAX_MOTIONPACKAGES` (8) motion packages
  per model.
- Lookup helpers: `Studio_FindBoneId`, `Studio_FindBone`, `Studio_FindBodyGroupId`,
  `Studio_FindBodyGroup`, `Studio_FindIkChain`, transform lookup — all case-insensitive.

## Motion package — `motionpackage.h`

Lump-based file (`'EQMP'`, version 7):

- **Animations** (`animationdesc_t`) — named ranges of frames; each frame (`animframe_t`) stores
  bone position and angles for every bone.
- **Sequences** (`sequencedesc_t`) — what game code plays: activity name and slot, framerate,
  transition time, flags (`SEQFLAG_LOOP`, `SEQFLAG_SLOTBLEND`), up to `MAX_BLEND_WIDTH` blended
  animations driven by a pose controller, layered sequences, events.
- **Events** (`sequenceevent_t`) — command + parameter fired at a frame.
- **Pose controllers** (`posecontroller_t`) — named blend parameter with a range.

## Physics model (POD — Physics Object Data) — `physmodel.h`

Lump-based file (`'PODI'`, version 2):

- Usage (`EPhysModelUsage`): rigid / compound body, ragdoll, dynamic (jointed props).
- Shapes (`physgeominfo_t`) — ranges in the shared vertex/index data; type concave, movable
  concave or convex.
- Objects (`physobject_t`) — up to `MAX_PHYS_GEOM_PER_OBJECT` shapes, mass, mass center, surface
  properties, body part.
- Joints (`physjoint_t`) — connect two objects with angular limits.

## Runtime structures — `model.h`

In-memory representations filled by the loaders:

- `StudioPhysData` — loaded physics model: objects (`StudioPhyObjData` with name and shape cache
  references), joints, shapes, vertices, indices, usage type. `PhysModel_FindObjectId(data, name)`.
- `StudioMotionData` — loaded motion package: animations, sequences, events, pose controllers,
  frames.
- `StudioJoint` — runtime skeleton joint: absolute / inverse / local transforms, children, bone
  descriptor, parent, IK link.
- Generic lump file headers `lumpfilehdr_t` / `lumpfilelump_t` used by `.mop` and `.pod`.

## Where it is used

Runtime details: [rendering_egf_overview.md](rendering_egf_overview.md) (loading, drawing) and
[animating_egf_overview.md](animating_egf_overview.md) (animation).

| Code | Purpose |
|---|---|
| `shared/studiofile` (`studioFileLib`) | `Studio_LoadModel`, `Studio_LoadMotionData`, `Studio_LoadPhysModel` and matching `Studio_Free*` |
| `shared/studio` (`studioLib`) | `g_studioCache` — model cache (`PrecacheModel`, `GetModel` → `CEqStudioGeom`), motion data cache (`PrecacheMotionData`); GPU geometry |
| `shared/animating` (`animatingLib`) | Skeleton, sequence playback, pose controllers, events |
| `shared/physics`, `shared/dkphysics` | Physics shape cache from `.pod` data |
| `shared/egf` (`egfLib`), `tools/egfca`, `tools/animca`, `tools/egfMan2` | Building and viewing EGF files |
