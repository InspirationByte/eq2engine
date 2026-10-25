# Material shaders overview — `public/materialsystem1/BaseShader.h`

A material (see [public_rendering_overview.md](public_rendering_overview.md)) names a shader in its
root section. That name selects a **material shader**: a C++ class that reads the material
variables, owns the GPU resources of the material (parameter buffers, textures), builds render
pipelines and binds the bind groups for each draw. Each material shader has a matching
**GPU shader** — a Slang source and a `.def` description compiled by ShaderCooker into
`shaders/<Name>.shd`.

| Part | Where |
|---|---|
| Interface | `public/materialsystem1/IMatSysShader.h` — `IMatSystemShader`, `EBindGroupId`, `IShaderMeshInstanceProvider`, registration macros |
| Base class | `public/materialsystem1/BaseShader.h/.cpp` (`BaseShader` static library) — `CBaseShader`, `BEGIN_SHADER_CLASS` |
| Engine shaders | `core/materialsystem1/Shaders/Base` (`eqBaseShaders`), material system built-ins (`DefaultShader.cpp`) |
| Application shaders | Any project that links `BaseShader`, e.g. `samples/GPUDriven/shader_baseunlit.cpp` |
| GPU shader sources | Game data `shadersSRC` folders (e.g. E2DevMain `build/E2Base/shadersSRC`, `build/GpuDriven/shadersSRC`) |

## Lifecycle

| Step | Called by | Shader method |
|---|---|---|
| Material is created and its shader is resolved | `CMaterial::InitShader` | factory → constructor, `Init()` → `SHADER_INIT_PARAMS` |
| Material is loaded (`QueueLoading`, `LoadShaderAndTextures`, first draw) | material loader | `SHADER_INIT_RESOURCES` (`InitResources`), then `InitShader` → `BuildPipelineShaderQuery`, shader modules preloaded |
| A material variable changed (by a proxy or code) or the material was reloaded | next `SetupMaterialPipeline` / `UpdateMaterialProxies` | `UpdateProxy(cmdRecorder)` — rewrite parameter buffers |
| A draw with this material | `g_matSystem->SetupMaterialPipeline(...)` | `SetupRenderPass` → pipeline from cache or created → `GetBindGroup` for every bind group |
| Material is released or reloaded | material system | `Unload()` |

`CBaseShader::Init` reads the common material variables before `SHADER_INIT_PARAMS` runs:

| Variable | Effect |
|---|---|
| `BaseTextureTransform`, `BaseTextureScale`, `BaseTextureFrame` | Texture transform proxies (`GetTextureTransform`) |
| `Address` (`wrap`, `mirror`, `clamp`), `Filtering` (`nearest`, `linear`, `bilinear`, `trilinear`, `aniso`) | Sampler state |
| `NoDraw`, `Wireframe`, `Decal`, `NoCull`, `ReverseCull`, `AlphaTest`, `ZOnly`, `ZTest`, `ZWrite`, `CastShadows`, `ReceiveShadows` | `MATERIAL_FLAG_*` (`GetFlags()`) |
| `Translucent`, `Additive`, `Modulate` | Blend mode (+ `MATERIAL_FLAG_TRANSPARENT`) |

Material variables are looked up with `editor.` prefix first in editor mode, and a value starting
with `$` refers to a global material variable.

## Writing a shader

```cpp
BEGIN_SHADER_CLASS(
	MyShader,
	SHADER_VERTEX_ID(DynMeshVertex),	// supported vertex formats
	SHADER_VERTEX_ID(EGFVertex),
)
	SHADER_INIT_PARAMS()		// read material variables
	{
		m_colorVar = GetMaterialVar("color", "[1 1 1 1]");
	}

	SHADER_INIT_RESOURCES()		// create GPU resources, load textures
	{
		m_paramsBuffer = MakeParameterUniformBuffer("myParams", BUFFERUSAGE_UNIFORM | BUFFERUSAGE_COPY_DST,
			m_colorVar.Get(), Vector4D(1, 1, 0, 0));
		SHADER_PARAM_TEXTURE(BaseTexture, m_baseTexture);
	}

	void BuildPipelineShaderQuery(Array<EqString>& shaderQuery) const override;
	void UpdateProxy(IGPUCommandRecorder* cmdRecorder) const override;
	void FillBindGroupLayout_Constant(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const override;
	void FillBindGroupLayout_RenderPass(const MeshInstanceFormatRef& meshInstFormat, BindGroupLayoutDesc& bindGroupLayout) const override;
	IGPUBindGroupPtr GetBindGroup(IShaderAPI* renderAPI, EBindGroupId bindGroupId, const BindGroupSetupParams& setupParams) const override;

	IGPUBufferPtr		m_paramsBuffer;
	MatTextureProxy		m_baseTexture;
	MatVec4Proxy		m_colorVar;
END_SHADER_CLASS
```

- `BEGIN_SHADER_CLASS(Name, vertex ids...)` / `END_SHADER_CLASS` declare the class
  `CNameShader` in its own namespace and register a factory under `Name` (`DEFINE_SHADER`). The
  name must match the GPU shader name (the `.shd` package is `shaders/<Name>.shd`).
- The vertex id list is the set of vertex formats (`SHADER_VERTEX_ID(Format)` =
  `StringIdConst24("Format")`) the shader accepts; format `0` is always added.
- Registration: shaders of a shader library are registered by `REGISTER_INTERNAL_SHADERS()`
  in its `InitShaderLibrary` (`LoadShaderLibrary`); shaders compiled into the application are
  registered by `sysLib` (`sys_host.cpp`). `RegisterShader` loads the shader package.
- `DEFINE_SHADER_NOFACTORY(Name)` registers a GPU-only shader (e.g. compute) without a material
  class, so its package is loaded.

Helpers in `CBaseShader`:

- Material variables: `GetMaterialVar(name, default)` (creates the variable),
  `FindMaterialVar(name)`, `GetMaterialValue<T>(name, default)`; proxies `MatVec4Proxy`,
  `MatTextureProxy`, ...
- Textures: `SHADER_PARAM_TEXTURE(Var, member)` loads the texture named by the variable (error
  checkerboard if missing), `SHADER_PARAM_TEXTURE_NOERROR`, `SHADER_PARAM_TEXTURE_FIND` (already
  loaded textures only).
- `MakeParameterUniformBuffer(name, usage, values...)` — packs values with std430-like alignment
  into a new buffer.
- `GetTextureTransform`, `CreateAtlasBuffer` (rectangles of the material's atlas),
  `FillBindGroupLayout_Constant_Samplers` / `FillBindGroup_Constant_Samplers` (one shared
  `MaterialSampler`).

## Shader variants — `BuildPipelineShaderQuery`

`BuildPipelineShaderQuery` returns the list of defines for this material. They are fixed at
`InitShader` time and form the shader query: the RHI picks the matching precompiled permutation
from the package. Every define in the query must exist in the `.def` file; unknown defines are an
error.

```cpp
void BuildPipelineShaderQuery(Array<EqString>& shaderQuery) const override
{
	if (GetMaterialValue("NoFog", false) == false)
		shaderQuery.append("DOFOG");
	if (GetMaterialValue("VertexColor", false))
		shaderQuery.append("VERTEXCOLOR");
	if (m_flags & MATERIAL_FLAG_ALPHATESTED)
		shaderQuery.append("ALPHATEST");
}
```

## Pipelines

`SetupRenderPass` computes a pipeline id from the shader query, the vertex format and its used
streams, the material flags (cull, depth test / write, decal, Z-only, blend mode), the topology,
the presence of an instance provider, the render target formats and MSAA. The pipeline is looked up
in the shader object, then in the material-system-wide cache for this shader name
(`GetRenderPipelineCache`), and created otherwise:

- `FillRenderPipelineDesc` builds the description: shader name and query, vertex layouts from the
  mesh instance format (only the streams in `usedLayoutBits`; an instance stream must be bound),
  depth state, blending from the blend mode, color targets. Z-only passes and `ZOnly` materials
  get no fragment stage. Override it for special states.
- The binding layout is made of `FillBindGroupLayout_Constant`, `_RenderPass`, `_Transient` and, if
  a mesh instance provider is passed, its `FillBindGroupLayoutDesc` (group 3).
- Pipelines are created on a job thread; the draw is skipped (`SetupMaterialPipeline` returns
  `false`) until the pipeline is ready. `RenderPassContext::waitForPipelines` or device
  validation create them synchronously. `r_showPipelineCacheMisses` logs pipelines created during
  rendering; `r_pipelineCacheDisable` disables the shared cache.

## Bind groups

| Group | `EBindGroupId` | Lifetime | Typical contents |
|---|---|---|---|
| 0 | `BINDGROUP_CONSTANT` | Material lifetime | Material parameters, textures, samplers |
| 1 | `BINDGROUP_RENDERPASS` | One render pass | Camera, fog, per-pass buffers |
| 2 | `BINDGROUP_TRANSIENT` | One draw | Per-object data (e.g. bones) |
| 3 | `BINDGROUP_INSTANCES` | Instance manager buffers | GPU instance data from `IShaderMeshInstanceProvider` (GRIM) |

`GetBindGroup(renderAPI, id, setupParams)` returns the group for each id. Helpers:

- `CreatePersistentBindGroup` — stored in `setupParams.pipelineInfo.bindGroup[id]` and reused (for
  constant groups).
- `CreateBindGroup` — a new group each call (render pass / transient data).
- `CreateSharedBindGroup` / `GetSharedBindGroup` — one group shared by all materials of the shader
  class, with a version number.
- `GetEmptyBindGroup` — for unused groups.

`setupParams.uniformBuffers` holds buffers passed by the caller with a signature
(`RenderDrawCmd::AddUniformBuffer`), e.g. the camera buffer `s_matSysCameraBufferId`
(`'CMRA'`). Without one, shaders take the current camera from `g_matSystem->GetCameraParams` into a
transient uniform buffer.

GPU side bindings use the same group and binding indices; ShaderCooker defines
`BINDGROUP_CONSTANT` ... `BINDGROUP_INSTANCES` as `0` ... `3`:

```hlsl
[[vk::binding(0, BINDGROUP_RENDERPASS)]] ConstantBuffer<MatSysCamera> camera;
[[vk::binding(0, BINDGROUP_CONSTANT)]]   ConstantBuffer<SMaterialParams> materialParams;
[[vk::binding(1, BINDGROUP_CONSTANT)]]   SamplerState BaseTextureSampler;
[[vk::binding(2, BINDGROUP_CONSTANT)]]   Texture2D BaseTexture;
```

## GPU shader — `.def` and Slang source

```
shader BaseUnlit
{
	SourceFile "source_BaseUnlit.fx";
	SourceKind { Vertex; Fragment; }

	VertexLayouts { DynMeshVertex; EGFVertex; EGFVertexSkinned; PFXVertex; }

	Pass builtin_Fog { define DOFOG; }
	MatVar bool AlphaTest { eq 1 { define ALPHATEST; } }
	MatVar bool VertexColor { eq 1 { define VERTEXCOLOR; } }
	...
}
```

- `VertexLayouts` — vertex formats to compile for; each must have a Slang layout in
  `VertexLayouts/<Name>.h`. The name must equal the C++ vertex format name: the pipeline uses
  `StringId24` of the format name as `shaderVertexLayoutId`.
- Every nested section with `define` adds a switchable define; ShaderCooker compiles all
  combinations of defines for every vertex layout (`SkipCombo` excludes combinations). Section
  names (`Pass`, `MatVar`, `BlendMode`) only group and document the defines.
- ShaderCooker defines for each permutation: `VERTEX` / `FRAGMENT` / `COMPUTE`,
  `CURRENT_VERTEX_ID`, `VID_<Layout>` for all layouts, `VERTEX_LAYOUT` (the layout type),
  `BINDGROUP_*`, plus the selected defines.
- `shaderExt <file.def> <SourceShader> [<NewShader>] { ... }` copies another shader's description,
  optionally under a new name, and replaces the listed sections (e.g. `VertexLayouts`).

Vertex inputs are taken from the layout type: `VERTEX_LAYOUT::Position`, `VERTEX_LAYOUT::UVSet`,
`VERTEX_LAYOUT::Instance`, ...

## Shader resolution and overrides

`CMaterial::InitShader` resolves the shader with `g_matSystem->GetShaderFactory(shaderName,
instanceFormatId)`:

1. Overrides registered with `RegisterShaderOverride(shaderName, func)` are checked, newest
   first; `func(instanceFormatId)` returns a replacement name or `nullptr`.
2. Unknown shader, or a material vertex format the shader does not list → shader `Error`
   (overridden to `BaseUnlit` by `eqBaseShaders`).

`instanceFormatId` is the vertex format a material was requested for:
`g_matSystem->GetMaterial(name, instanceFormatId)`. Materials are cached by name, so the first
request decides it. EGF models request their materials with the format set by
`CEqStudioGeom::SetInstanceFormatId` (`0` by default). Format `0` means "any": the
`SetupMaterialPipeline` format check is skipped.

## Example — `BaseUnlit` and GRIM (`samples/GPUDriven`)

`samples/GPUDriven/shader_baseunlit.cpp` defines `BaseUnlitGame`, a copy of the engine `BaseUnlit`
(`core/materialsystem1/Shaders/Base/BaseUnlit.cpp`) for instanced rendering with GRIM (see
[rendering_grim_overview.md](rendering_grim_overview.md)).

C++ side:

| Part | `BaseUnlitGame` |
|---|---|
| Vertex formats | `EGFVertexSkinned` (+ `0`) |
| `SHADER_INIT_PARAMS` | Sets `MATERIAL_FLAG_SKINNED`; reads `color` |
| `SHADER_INIT_RESOURCES` | `paramBuffer` uniform (color, texture transform); `BaseTexture` |
| Query | `DOFOG` unless `NoFog`, `VERTEXCOLOR`, `ALPHATEST` |
| `UpdateProxy` | Rewrites color and texture transform into the parameter buffer |
| Group 0 (constant) | `materialParams` (b0), `BaseTextureSampler` (b1), `BaseTexture` (b2) — persistent |
| Group 1 (render pass) | `camera` (b0) — passed buffer `'CMRA'` or the current camera |
| Group 2 (transient) | Empty |
| Group 3 (instances) | `setupParams.meshInstProvider->GetInstancesBindGroup(...)`, i.e. GRIM's instance roots and component buffers |

GPU side (`build/GpuDriven/shadersSRC`):

- `BaseShadersExt.def`: `shaderExt BaseUnlit.def BaseUnlit BaseUnlitGame { VertexLayouts {
  DynMeshVertex; EGFVertexGameObj; } }` — the same `source_BaseUnlit.fx` compiled as
  `BaseUnlitGame` for the instanced layout.
- `VertexLayouts/EGFVertexGameObj.h` — EGF streams plus the `Instance` stream (`instanceId`).
- `source_BaseUnlit.fx` enables instancing for `EGFVertexGameObj` (`INST`): it includes
  `game/instances.h` (instance buffers in `BINDGROUP_INSTANCES`) and transforms the vertex with
  `InstTransformVertex(instRoots[_in_instance.instanceId], pos)` before `camera.viewProj`.

How a draw comes together:

1. The sample creates the vertex format `EGFVertexGameObj` (EGF position/UV stream + GRIM instance
   id stream) and EGF archetypes with it.
2. `states.cpp` registers overrides: materials of `Skinned` and `DrvSynVehicle` always use
   `BaseUnlitGame`; `BaseUnlit` materials use it when requested for `EGFVertexGameObj`.
3. `GRIMBaseRenderer::Draw` calls `g_matSystem->SetupMaterialPipeline(material, nullptr, topology,
   archetypeFormat, renderPassCtx, this)` with itself as the mesh instance provider.
4. `CBaseShader::SetupRenderPass` builds a pipeline for `EGFVertexGameObj` with group 3 from
   `DemoGRIMRenderer::FillBindGroupLayoutDesc`, then binds groups 0–3; group 3 comes from
   `DemoGRIMRenderer::GetInstancesBindGroup`, re-created when the instance buffers change.
5. GRIM binds the vertex buffers (the instance stream = visible instance ids) and issues
   `DrawIndexedIndirect`.
