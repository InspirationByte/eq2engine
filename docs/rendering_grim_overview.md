# GRIM overview — `shared/grim`

**GRIM — GPU-driven Rendering and Instance Manager** (`grimLib`). It keeps per-instance data in GPU
buffers and renders large numbers of instanced meshes. Filtering, visibility culling, LOD selection
and the building of indirect draw commands all run in compute shaders. The CPU only uploads
changed data and records one `DrawIndexedIndirect` per mesh batch.

| File | Contents |
|---|---|
| `GrimDefs.h` | Handle types (`GRIMArchetype`, `GRIMInstanceRef`), limits, `GRIMArchetypeDesc` |
| `GrimInstanceAllocator.h/.cpp` | `GRIMBaseInstanceAllocator`, `GRIMInstanceAllocator<Components...>` — instances and their components |
| `GrimComponentPool.h` | Component pools and the component definition macros |
| `GrimSynchronizedPool.h/.cpp` | `GRIMSyncrhronizedPool<T>` — CPU array mirrored to a GPU buffer or texture |
| `GrimBaseRenderer.h/.cpp` | `GRIMBaseRenderer` — archetypes, compute passes, drawing; `GRIMInstanceDebug` ImGui UI |

`grimLib` uses `renderUtilLib` and ImGui. Usage example: `samples/GPUDriven` (see
[below](#example--samplesgpudriven)). GRIM works through the RHI (`IShaderAPI`) and the material
system (see [public_rendering_overview.md](public_rendering_overview.md)).

## Concepts

| Term | Meaning |
|---|---|
| **Archetype** | What to draw: vertex and index buffers, a vertex format with an instance stream, and up to `GRIM_MAX_INSTANCE_LODS` (8) LODs. Each LOD is a list of batches (index range + material + topology). An EGF model with a set of body groups and a skin becomes one archetype. |
| **Instance** | One object to draw: an archetype id, a group mask and a *root* — indices of its components. |
| **Component** | A piece of per-instance data (transform, scale, ...) stored in its own pool, so instances only allocate what they use. Up to `GRIM_INSTANCE_MAX_COMPONENTS` (8) component types. |
| **Render state** (`GRIMRenderState`) | Per view: buffers with the visible instance ids and indirect draw commands. One for each camera or pass that culls on its own. |

On the GPU side:

```
instance i ──► archetypeIds[i], groupMasks[i]
          └──► instRoots[i].components[COMPONENT_ID] ──► inst<Component>s[idx]

archetype a ──► drawLodsList[a].firstLodInfo ──► GPULodInfo (distance, firstBatch, next)
                                                 └──► GPUIndexedBatch (firstIndex, indexCount, cmdIdx, next)
```

## Components and the instance allocator

A component is a plain struct with a definition macro. The matching shader struct must have the
same layout.

```cpp
// header
enum EGPUInstanceComponentId : int
{
	GPUINST_PROP_ID_TRANSFORM = 0,
	GPUINST_PROP_ID_SCALE,
};

struct InstTransform
{
	DEFINE_GPU_BUFFER_INSTANCE_COMPONENT(GPUINST_PROP_ID_TRANSFORM, InstTransform);

	Quaternion	orientation{ qidentity };
	Vector3D	position{ vec3_zero };
	float		boundingSphere{ 1.0f };
};

using MyInstanceAllocator = GRIMInstanceAllocator<InstTransform, InstScale>;

// source file: component update shader (see "Shaders")
INIT_GPU_INSTANCE_COMPONENT(InstTransform, "InstanceUtils")
INIT_GPU_INSTANCE_COMPONENT(InstScale, "InstanceUtils")
```

| Macro | Pool | Storage |
|---|---|---|
| `DEFINE_GPU_BUFFER_INSTANCE_COMPONENT(ID, Name)` | `GRIMBufferComponentPool` | Storage buffer synced to the GPU |
| `DEFINE_GPU_TEXTURE_INSTANCE_COMPONENT(ID, Name, Format, SizeX, SizeY)` | `GRIMTextureComponentPool` | Texture array (one layer per item), storage or sampled |
| `DEFINE_INSTANCE_COMPONENT(ID, Name)` | `GRIMComponentPool` | CPU only, not synced |
| `INIT_GPU_INSTANCE_COMPONENT[_EX](Name, UpdateShaderName[, initial, extend])` | | Defines the pool size (default 3072 / 1024) and creates the update compute pipeline from shader `UpdateShaderName`, variant `Name` |
| `INIT_INSTANCE_COMPONENT[_EX](Name[, initial, extend])` | | The same for CPU-only components |

Allocator API (`GRIMInstanceAllocator<...>`):

- `Initialize(instanceShaderName, instancesReserve)` / `Shutdown()`. The shader name is the
  update shader for roots and `int` arrays (variants `InstRoot`, `InstInt`).
- `AddInstance<TComps...>(archetype)` — allocates an instance and the listed components; returns
  the instance id. `AddTempInstance` — the same, but the instance is freed by the next
  `SyncInstances` (at most 128 at a time).
- `Set(instanceId, values...)` — writes component values; unchanged values (compared with
  `memcmp`) are not uploaded again. Components the instance does not have are skipped.
- `Add<TComp>(id)`, `Remove<TComp>(id)`, `Has<TComp>(id)`.
- `SetArchetype(id, archetype)` (body group changes etc.), `SetGroupMask(id, mask)`,
  `FreeInstance(id)`, `FreeAll(dealloc, reserve)`.
- `SyncInstances(cmdRecorder)` — uploads changes: grows GPU buffers when needed and runs the
  update pipelines for the changed items. Call it once per frame before drawing.
- GPU buffers: `GetRootBuffer()`, `GetInstanceArchetypesBuffer()`, `GetInstanceGroupMaskBuffer()`,
  `GetComponentPool<T>().GetBuffer()` / `.GetTexture()`. They are re-created when they grow, so
  rebuild bind groups when `GetBufferUpdateToken()` changes.

Reserved values:

- Instance 0 and item 0 of every pool are defaults created at construction. An instance without a
  component points to item 0 and reads its default value (`TComp{}`).
- `COM_UINT_MAX` marks the components of a freed instance; a freed instance has archetype
  `GRIM_INVALID_ARCHETYPE` and is skipped by the filter pass.
- Group mask `0` means "always drawn"; the default mask is `COM_UINT_MAX`.

The allocator and the pools guard their data with read-write locks, so instances can be added and
updated from worker threads. `SyncInstances` records GPU commands and should not overlap with
other changes.

## Archetypes

```cpp
// from an EGF model: body groups as a bit mask (must not be 0), skin index
GRIMArchetype arch = renderer.CreateStudioDrawArchetype(geom, instVertexFormat, bodyGroupFlags, skinIdx);

// from own buffers
GRIMArchetypeDesc desc;
desc.name = "grass";
desc.vertexBuffers[0] = posUvBuffer;	// one per non-instance stream of the format
desc.indexBuffer = indexBuffer;
desc.indexFormat = INDEXFMT_UINT16;
desc.meshInstanceFormat = instVertexFormat;
desc.batches.append({ material, PRIM_TRIANGLES, firstIndex, indexCount });
desc.lods.append({ 0, 1, 0.0f });		// firstBatch, batchCount, switch distance
GRIMArchetype arch2 = renderer.CreateDrawArchetype(desc);
```

- The vertex format must contain a layout with `VERTEX_STEPMODE_INSTANCE`: during drawing that
  stream is bound to the visible instance ids buffer. For EGF models, streams are matched by the
  EGF stream ids in the layout `userId`; models with bone weights are marked as supporting
  skinning.
- Creation, `UpdateDrawArchetype` and `DestroyDrawArchetype` are thread-safe and deferred: the id
  is returned at once, the data is built in `SyncArchetypes(cmdRecorder)`.
- Materials with `MATERIAL_FLAG_INVISIBLE` are skipped for EGF archetypes.
- LOD selection: LOD `i` is used from its `distance` up to the next LOD's distance; the last LOD
  has no far limit. Give the first LOD distance `0`: the compute cull shader drops instances closer
  than the first LOD distance.

## Renderer

`GRIMBaseRenderer` is abstract. The application derives from it because the visibility test needs
the application's own components (positions, bounds):

```cpp
class MyGRIMRenderer : public GRIMBaseRenderer
{
public:
	MyGRIMRenderer(MyInstanceAllocator& alloc) : GRIMBaseRenderer(alloc) {}
	void Init();	// create m_cullInstancesBindingLayout, then GRIMBaseRenderer::Init()

	// IShaderMeshInstanceProvider: instance data for material shaders
	void FillBindGroupLayoutDesc(BindGroupLayoutDesc& bindGroupLayout) const override;
	void GetInstancesBindGroup(int bindGroupIdx, IGPUBindingLayout* pipelineLayout, IGPUBindGroupPtr& outBindGroup, uint& lastUpdateToken) const override;

	void VisibilityCullInstances_Compute(IntermediateState& intermediate) override;
	void VisibilityCullInstances_Software(IntermediateState& intermediate) override;
};
```

- `Init()` creates the compute pipelines; the cull pipeline (`InstancesCull`) uses
  `m_cullInstancesBindingLayout`, which the derived class creates first.
- `IShaderMeshInstanceProvider` — material shaders call `GetInstancesBindGroup` for
  `BINDGROUP_INSTANCES` to bind the instance roots and component buffers in vertex shaders.
- `VisibilityCullInstances_Compute` dispatches the application's cull shader over the filtered
  instances. It writes the visible instances with their selected LOD and counts instances per
  `(archetype, LOD)` bound. `VisibilityCullInstances_Software` does the same on the CPU.
- Extend `GRIMRenderState` with view data (frustum, camera position, parameter buffers) and cast
  it in the cull functions.

### Frame

```cpp
// once per frame, before any PrepareDraw
allocator.SyncInstances(cmdRecorder);
renderer.SyncArchetypes(cmdRecorder);

// per view
renderer.PrepareDraw(cmdRecorder, renderState, maxObjects);
submit(cmdRecorder);
// after the GPU finished the commands
renderer.PostPrepareDraw(renderState);

// in a render pass
renderer.Draw(renderState, RenderPassContext(rendPassRecorder, nullptr));
```

`PrepareDraw` records the compute passes:

| Step | Shader (variant) | Result |
|---|---|---|
| Filter | `InstanceFilter` (`Filter`, `CalcWorkGroups`) | Valid instances that pass the group masks, with the forced LOD (`overrideLodIdx`), plus dispatch sizes |
| Cull | `InstancesCull` (application) | Visible instances with their LOD; instance counts per bound |
| Bounds | `InstanceCalcBounds` (`InitBounds`, `FillInstanceIds`) | Start offset of each `(archetype, LOD)` range; instance ids sorted into the ranges |
| Indirect | `InstancePrepareDrawIndirect` | `GPUDrawIndexedIndirectCmd` for every batch: index range from the batch, instance range from the bound |

- `PostPrepareDraw` maps the copy of the indirect commands back to the CPU and marks the draw
  invocations with `instanceCount > 0`. Until then every invocation is drawn; `Draw` waits for the
  readback if it is still in flight.
- `Draw` skips inactive invocations, hidden archetypes and archetypes with no instances. It groups
  the rest by material, vertex format and topology, sets up each material pipeline once and issues
  `DrawIndexedIndirect` per batch. `RenderPassContext::beforeMaterialSetup` may replace or drop
  materials (e.g. for a depth pass); `GRIMRenderState::drawCallMaterialGroupByFlags` selects whether
  grouping uses the original or the replaced material.
- Group masks: an instance is drawn when `(mask & include) != 0` and `(mask & exclude) == 0`, with
  `include = renderState.groupMaskInclude & settings.groupMaskInclude` and exclude combined with OR.

### Draw settings and software mode

`GRIMDrawSettings` (`Get/SetDrawSettings`): group masks, `overrideLodIdx` (`-1` — by distance),
`waitForMaterials` (load materials synchronously), `forceSoftware`. `skipOcclusionCulling` and
`dbgDrawIndices` are not read by the base renderer; they are for derived renderers (the debug UI
only toggles `dbgDrawIndices`).

Software mode (`forceSoftware` or `grim_dbgSoftwareMode 1`) runs filtering, culling, bounds and
indirect command generation on the CPU and uploads the results. Use it to debug the compute
path.

## Shaders

The shader sources live in the game data, not in this repository. They are compiled by
`ShaderCooker` (see [tools_overview.md](tools_overview.md)). In the E2DevMain workspace:

**Engine base** — `build/E2Base/shadersSRC`:

| File | Shader | Purpose |
|---|---|---|
| `GRIMRenderer.def` | `InstanceFilter`, `InstanceCalcBounds`, `InstancePrepareDrawIndirect`, `InstancePreparePools` | GRIM compute shaders; `InstancePreparePools` updates the archetype buffers (`GPULodList`, `GPULodInfo`, `GPUIndexedBatch`) |
| `source_ComputeInstanceFilter.fx` | `InstanceFilter` | Filter pass |
| `source_ComputeInstanceCalcBounds.fx` | `InstanceCalcBounds` | Bounds and instance id sorting |
| `source_ComputeInstancePrepareDrawIndirect.fx` | `InstancePrepareDrawIndirect` | Indirect commands |
| `source_ComputeInstanceUtils.fx` | — | Generic update kernel: `destData[sourceInfo[i]] = sourceData[i]`; included with `InstanceType` defined |
| `common/instance_defs.h` | — | GPU structs, LOD / archetype packing, `instance_CalcLodIdx` (with `ENABLE_LOD_INFO`) |
| `common/instances.h` | — | `InstRoot` |

**Application** — `build/GpuDriven/shadersSRC` (the sample; a game provides its own):

| File | Purpose |
|---|---|
| `ComputeInstanceRendering.def` | `InstanceUtils` — update shader with variants `InstRoot`, `InstInt` (required) and one per component (`InstTransform`, `InstScale`); `InstancesCull` |
| `source_ComputeCullInstances.fx` | Frustum culling by bounding sphere and LOD selection |
| `game/instances.h` | Component ids and shader structs, instance buffer bindings in `BINDGROUP_INSTANCES`, `InstTransformVertex`, `InstRotateVector[Inv]` |
| `game/instance_occlusion.h` | View parameters (`SViewParams`), `frustum_IsSphereInside` |
| `VertexLayouts/EGFVertexGameObj.h` | EGF vertex layout plus the `Instance` stream (`IInstanceIndex`) |
| `BaseShadersExt.def` | Adds the `EGFVertexGameObj` layout to `BaseUnlit` as `BaseUnlitGame` |

- Compute shader variants are selected by layout name: the C++ side passes
  `ShaderLayoutId(StringIdConst24("Filter"))`, the shader checks `CURRENT_VERTEX_ID == VID_Filter`.
- Material vertex shaders read the instance id from the instance stream and fetch the data through
  the root: `InstTransformVertex(instRoots[_in_instance.instanceId], pos)` (see
  `source_BaseUnlit.fx`). `INST_COMPONENT_<Name>` defines enable the buffers of each component.

Keep these in sync between C++ and shaders:

- `GPUIndexedBatch`, `GPULodInfo`, `GPULodList`, `GPUInstanceBound`, `GPUInstanceInfo`
  (`GrimBaseRenderer.h` ↔ `common/instance_defs.h`);
- the work group sizes (`GPUVIS_*`), `ARCHETYPE_BITS` / `LOD_BITS`, `MAX_INSTANCE_LODS`,
  `GPUINST_MAX_COMPONENTS`;
- component ids and struct layouts (application header ↔ `game/instances.h`);
- the binding layouts created in C++ ↔ `vk_binding` indices in the shaders.

## Example — `samples/GPUDriven`

The sample draws up to `inst_count` (10000 by default) random EGF models.

| File | Contents |
|---|---|
| `instancer.h/.cpp` | `InstTransform`, `InstScale` components; `DemoGRIMInstanceAllocator`; `DemoGRIMRenderer` with compute and software culling; `DemoRenderState` (frustum, view position); instance id vertex layout `GetGPUInstanceVertexLayout()` |
| `gpudriven_main.cpp` | `CState_GpuDrivenDemo` — setup, instance placement, per-frame sync / prepare / draw |
| `shader_baseunlit.cpp` | `BaseUnlitGame` material shader — binds `BINDGROUP_INSTANCES` through the instance provider |
| `states.cpp` | Shader overrides (`g_matSystem->RegisterShaderOverride`): `BaseUnlit` materials use `BaseUnlitGame` with the `EGFVertexGameObj` format; `Skinned` and `DrvSynVehicle` always use `BaseUnlitGame` |

Flow:

1. Create vertex format `EGFVertexGameObj` = EGF position/UV stream + instance id stream.
2. For each model in `models/`: `CreateStudioDrawArchetype(geom, format, 3)` (body groups 0 and 1).
3. `Initialize("InstanceUtils", reserve)` on the allocator, `Init()` on the renderer.
4. `AddInstance<InstTransform>(archetype)` + `Set(id, transform)` for each object. `InstScale` is
   not allocated, so the instances use the default scale item.
5. Each frame: `SyncInstances`, `SyncArchetypes`, `PrepareDraw`; submit and wait;
   `PostPrepareDraw`; then `Draw` in the backbuffer render pass. With `inst_update_once 1`,
   culling runs once and later frames reuse the stored render state.

Debug UI: "ENGINE/RENDER/INSTANCE MANAGER DEBUG UI" menu (`DemoInstManagerDebugDrawUI` →
`GRIMInstanceDebug::DrawUI`).

## Debugging

- `GRIMInstanceDebug::DrawUI(renderer)` — ImGui window: statistics (archetypes, instances, draw
  calls, memory of every pool), software mode switch, "Invalidate all data", the archetype list
  with per-archetype visibility, highlighting and details (LODs, batches, materials, vertex
  buffers). Available when `GRIM_INSTANCES_DEBUG_ENABLED` (not in `Profile` / `Retail`).
- Cheat ConVars: `grim_dbgSoftwareMode`, `grim_dbgOnlyMaterial <name>` (draw only one material),
  `grim_dbgLogArchetypes`, `grim_dbgValidate`.
- With device validation enabled, every indirect draw gets a debug marker with the archetype,
  material, LOD and instance count.
- Multi-draw indirect (`GRIM_MULTI_DRAW_SUPPORT`) is in development and disabled.

## Limits

- 8 LODs per archetype, 8 component types, 2^24 archetypes (`ARCHETYPE_BITS`).
- Draw invocations are linked with `uint16` indices in `GRIMRenderState::drawInfoLinkList`, so at
  most 65535 draw infos (batches of all archetypes and LODs).
- Instance buffers are re-created and copied when they grow; reserve enough instances up front.
