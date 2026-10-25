# Rendering overview — `public/materialsystem1`

Short overview of the material system interfaces. Rendering itself (RHI, command recording,
render passes, pipelines) will be described in separate documents.

## Modules

| Project | Kind | Contents |
|---|---|---|
| `eqMatSystem` | shared library | Material system: materials, material variables and proxies, texture loader, dynamic meshes, render manager |
| `eqBaseShaders` | shared library | Built-in material shaders (`core/materialsystem1/Shaders/Base`), loaded with `LoadShaderLibrary` |
| `eqNVRHI`, `eqWGPURHI`, `eqNullRHI` | shared libraries | RHI backends (D3D12/Vulkan, WebGPU, null) implementing `IShaderAPI`; selected by the `MaterialSystem.Renderer` list in `E2.CONFIG` |
| `BaseShader` | static library | `CBaseShader` base class for writing material shaders (`public/materialsystem1/BaseShader.*`) |

Public headers:

| Header | Contents |
|---|---|
| `IMaterialSystem.h` | `IMaterialSystem` (`g_matSystem`), init settings, frame and camera API |
| `IMaterial.h`, `IMaterialVar.h` | `IMaterial`, material flags, typed material variable proxies |
| `IMaterialProxy.h` | Material proxies — per-frame variable animation |
| `ITextureLoader.h` | Texture loading |
| `IMatSysShader.h`, `BaseShader.h` | Material shader interface and base class |
| `IDynamicMesh.h`, `MeshBuilder.h` | Per-frame dynamic geometry and its builder |
| `RenderDefs.h`, `SceneDefs.h` | Draw command, render pass, camera and fog definitions |
| `renderers/*` | RHI interfaces (`IShaderAPI`, `ITexture`, `IGPUBuffer`, `IGPUCommandRecorder`, `ISwapChain`, ...) — rendering docs |
| `public/render/IDebugOverlay.h` | Debug drawing (`debugoverlay`), implemented in `renderUtilLib` (`shared/render/DebugOverlay.cpp`) |
| `public/font/IFont.h`, `IFontCache.h` | Fonts and text rendering (`g_fontCache`), implemented in `fontLib` (`shared/font`) |

## Material system — `IMaterialSystem.h`

`g_matSystem` (`extern IMaterialSystem*`) is set by the application host (`shared/sys/sys_host.cpp`)
from `g_eqCore->GetInterface<IMaterialSystem>()` after the material system library is loaded; it
is initialized with `MaterialsInitSettings`: renderer library name, material
and texture paths, `MatSysRenderSettings` (threaded loading, editor mode, shadows, wireframe).

Main responsibilities:

- **Materials:** `GetMaterial(name)` (loads or returns cached), `CreateMaterial(name, params)` (from
  a `KVSection`), `IsMaterialExist`, `GetDefaultMaterial`, `QueueLoading`, `PreloadNewMaterials`,
  `WaitAllMaterialsLoaded`, `ReloadAllMaterials`, `ReleaseUnusedMaterials`.
- **Shaders:** `LoadShaderLibrary`, `RegisterShader`, `RegisterShaderOverride`, `ReloadShader`.
- **Proxies:** `RegisterProxy(factory, name)`, `CreateProxyByName`, `SetProxyDeltaTime`.
- **Global material variables:** `GetGlobalMaterialVarByName(name, default)` — variables shared by
  all materials (`$name` references in material files).
- **Frame:** `CreateSwapChain`, `BeginFrame(swapChain)` / `EndFrame()`, `GetFrameId`,
  `GetCurrentBackbuffer`, `GetDefaultDepthBuffer`, `CaptureScreenshot`, windowed mode and
  backbuffer size.
- **Camera and matrices:** `SetMatrix(mode, m)` / `GetMatrix`, `Setup2D`, `SetupProjection`,
  `SetupOrtho`, `GetViewProjection`, `GetCameraParams`; `SetFogInfo`.
- **Helpers:** `GetDynamicMesh()`, `GetTransientUniformBuffer`, `GetWhiteTexture`,
  `GetErrorCheckerboardTexture`, `GetShaderAPI()` (RHI access).
- **Device loss:** `AddDestroyLostCallbacks` / `RemoveLostRestoreCallbacks`.

## Materials — `IMaterial.h`, `IMaterialVar.h`

A material is a text file in KeyValues format (`materials/<name>.mat`). The root section name is
the shader name; keys are material variables; `materialproxy` holds proxies:

```
DrvSynWater
{
	color				"[0.33 0.34 0.39]";
	bumpmap				"water/water60";
	cubemap				$env_cubemap;			// global material variable
	basetexturescale	"[0.35 0.25]";
	nocull				1;

	materialproxy
	{
		animatedtexture 60
		{
			frameVar	$bumptextureframe;
			framecount	60;
			framerate	#50;
		};
		sin $temp "$basetexturetransform[0]";
	};
};
```

- `IMaterialPtr` (ref-counted) is returned by the material system. `GetName`, `GetShaderName`,
  `GetFlags` (`EMaterialFlags`: culling, depth, decal, shadows, skinned, alpha test, transparency,
  sky, water, ...), `GetState` (`EMaterialLoadingState`), `IsError`, `WaitForLoading`,
  `GetBaseTexture`.
- Materials load asynchronously when the threaded loader is enabled; call `WaitForLoading()` or
  `LoadShaderAndTextures()` when the result is needed immediately.
- Variables: `GetMaterialVar(name, default)` creates or returns a variable, `FindMaterialVar(name)`
  only finds existing ones. Both return an untyped `MatVarProxyUnk`; convert it to a typed proxy:

```cpp
MatVec4Proxy colorVar = material->GetMaterialVar("color", "[1 1 1 1]");
colorVar.Set(Vector4D(1.0f, 0.5f, 0.5f, 1.0f));

MatTextureProxy baseTex = material->GetMaterialVar("basetexture");
```

Typed proxies: `MatIntProxy`, `MatFloatProxy`, `MatVec2Proxy`, `MatVec3Proxy`, `MatVec4Proxy`,
`MatM3x3Proxy`, `MatM4x4Proxy`, `MatStringProxy`, `MatTextureProxy`, `MatBufferProxy`. A proxy
holds a weak reference to the material's variables: it becomes invalid (`IsValid()`) when the
material is freed; it can be kept and reused across frames.

## Material proxies — `IMaterialProxy.h`

Proxies update material variables every frame (animation, time-based effects). They are declared
in the `materialproxy` section of a material; operands are `$variable`, `#constant`,
`"$var[index]"` for vector elements, and the constants `frametime` / `gametime`.

Custom proxies derive from `IMaterialProxy` (or `CBaseMaterialProxy`, which parses variables and
constants) and implement `InitProxy(material, section)` and `UpdateProxy(dt)`:

```cpp
DECLARE_PROXY(myproxy, CMyProxy);		// factory function

REGISTER_PROXY(myproxy, CMyProxy);		// at init, after g_matSystem is available
```

## Textures — `ITextureLoader.h`

`ITextureLoader` (`g_texLoader`, core interface registered by the material system):
`LoadTextureFromFileSync(name, samplerParams, flags)` and asynchronous
`LoadTextureFromFile(...)` returning `Future<ITexturePtr>`; `GetTexturePath` / `GetTextureSRCPath`.
Usually textures are referenced from materials and loaded by shaders; use the loader directly only
for textures not tied to a material.

## Material shaders — `IMatSysShader.h`, `BaseShader.h`

A material's root section selects a shader registered with the material system. Shaders implement
`IMatSystemShader`; in practice they derive from `CBaseShader` using the `BEGIN_SHADER_CLASS`,
`SHADER_INIT_PARAMS`, `SHADER_PARAM_TEXTURE`, ... macros, and are registered through
`DECLARE_INTERNAL_SHADERS` / `REGISTER_INTERNAL_SHADERS` in a shader library such as
`eqBaseShaders`. Shader authoring: [rendering_matsystem_shaders_overview.md](rendering_matsystem_shaders_overview.md).

## Dynamic geometry — `IDynamicMesh.h`, `MeshBuilder.h`

`g_matSystem->GetDynamicMesh()` returns a per-frame vertex/index buffer for immediate-style
drawing (debug drawing, UI, effects): `AllocateGeom`, `SetPrimitiveType`, `FillDrawCmd`, `Reset`.
`CMeshBuilder` wraps it with a convenient vertex-by-vertex API. `SetupDrawDefaultUP` draws simple
vertex arrays with the default material.

## Debug drawing — `public/render/IDebugOverlay.h`

`debugoverlay` (`extern IDebugOverlay*`, implemented in `renderUtilLib`) draws debug text,
shapes and graphs on top of the frame. Include `render/IDebugOverlay.h`.

Availability:

- `ENABLE_DEBUG_DRAWING` — defined in all configurations except `Profile` and `Retail`;
- `ENABLE_DEBUG_DRAWING_PROFILE` — defined in all configurations except `Retail`.

Wrap debug drawing code in `#ifdef ENABLE_DEBUG_DRAWING` (usually together with a debug ConVar), so
it is compiled out of optimized builds.

### Primitives

Primitives are built with fluent builders and submitted automatically when the builder goes out of
scope (end of the statement for a temporary):

```cpp
#ifdef ENABLE_DEBUG_DRAWING
if (snd_scriptsound_debug.GetBool())
{
	DbgSphere()
		.Position(position).Radius(radius)
		.Color(color_yellow)
		.Name(EqString::Format("emit %x", emitter))
		.Time(30.0f);

	DbgLine().Start(a).End(b).Color(color_red);
	DbgBox().Box(bbox).Color(color_green).Fill(false);
	DbgText3D().Position(pos).Text("speed %.2f", speed).Distance(100.0f);
}
#endif
```

| Builder | Shape | Setters |
|---|---|---|
| `DbgText3D()` | Text at a 3D position | `Text(fmt, ...)`, `Position`, `Distance` (max visible distance), `Color` |
| `DbgLine()` | Line | `Start`, `End`, `Color`, `ColorStart`, `ColorEnd` |
| `DbgBox()` | Axis-aligned box | `Box(bbox)`, `Mins`, `Maxs`, `CenterSize`, `Color`, `Fill` |
| `DbgOriBox()` | Oriented box | box setters + `Position`, `Rotation(quaternion)` |
| `DbgSphere()` | Sphere | `Position`, `Radius`, `Color`, `Fill` |
| `DbgCylinder()` | Cylinder | `Position`, `Radius`, `Height`, `Color` |
| `DbgPoly()` | Polygon (up to 20 points) | `Point`, `Points`, `ClipPlane(plane, origin, size)`, `Color`, `Fill`, `Outline` |
| `DbgVolume()` | Convex volume from planes | `Planes`, `Volume(volume)`, `Color` |

Common setters (call them last, they return the base builder):

- `Time(seconds)` — keep the primitive visible for the given time; `0` (default) — one frame.
- `Name(name)` — identifies a primitive, so a new primitive with the same name replaces the
  previous one (useful together with `Time` for per-object debug shapes).

The old-style functions `Text3D`, `Line3D`, `Box3D`, `Sphere3D`, `Cylinder3D`, `OrientedBox3D`,
`Polygon3D`, `Volume3D` are kept for compatibility; prefer the builders in new code.

### Text, graphs and custom drawing

- `debugoverlay->Text(color, fmt, ...)` — a line in the on-screen debug text list for the current
  frame; `TextFadeOut(position, color, fadeTime, fmt, ...)` — a message that fades out.
- Graphs: create a `DDGraphBucket(name, color, maxValue, updateTime, dynamicScaling)`, add samples
  with `Graph_AddValue(bucket, value)` and draw it each frame with `Graph_DrawBucket(bucket)`.
- `Draw2DFunc(fn, time, hashId)` / `Draw3DFunc(fn, time, hashId)` — custom drawing callback
  `bool fn(IGPURenderPassRecorder*)` executed in the 2D or 3D debug pass.

The application host calls `SetMatrices(proj, view)` and `Draw(width, height, timeScale)` once per
frame; game code only submits primitives.

## Fonts — `public/font/IFont.h`, `IFontCache.h`

`g_fontCache` (`extern IEqFontCache*`, implemented in `fontLib`) loads font families and returns
`IEqFont` instances. Include `font/IFontCache.h`.

### Font description

Font families are listed in `resources/fonts.res` (loaded at `Init()`; more files can be added with
`LoadFontDescriptionFile`). Each family has size entries with font files per style; the files are
`resources/fonts/<name>.fnt`:

```
Roboto
{
	30
	{
		reg		"Roboto-Regular";
		bld		"Roboto-Bold";
		itl		"Roboto-Italic";
		b+i		"Roboto-BoldItalic";
	}
}

#include "resources/fonts_game.res";
```

- `reg` is required; `bld`, `itl`, `b+i` are optional.
- `GetFont(name, bestSize, styleFlags, defaultIfNotFound)` picks the last size entry not larger
  than `bestSize` (or the first entry if all are larger), so list sizes in ascending order. The
  requested style (`TEXT_STYLE_BOLD`, `TEXT_STYLE_ITALIC`, both) falls back to regular if that
  file is missing. Unknown family names fall back to the `default` family.
- Family names are case-sensitive.
- `ReloadFonts()` reloads all fonts.

### Drawing text

```cpp
IEqFont* font = g_fontCache->GetFont("Roboto", 30, TEXT_STYLE_BOLD);

FontStyleParam params;
params.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
params.align = TEXT_ALIGN_HCENTER;
params.textColor = color_white;

font->SetupRenderText(L"&#FF4040;Red&; and white", position, params, renderPassRecorder);
const float width = font->GetStringWidth(L"Some text", params);
```

- `SetupRenderText(text, start, params, renderPassRecorder)` records text drawing into a render
  pass (narrow UTF-8 or wide strings). Text is usually drawn from UI or debug draw callbacks that
  provide the `IGPURenderPassRecorder`.
- Measuring: `GetStringWidth(text, params, charCount, breakOnChar)`, `GetLineHeight(params)`,
  `GetBaselineOffs(params)`; glyph data: `GetFontCharById`, `GetScaledCharacter`.
- `FontStyleParam`: `align` (`ETextAlignment`), `styleFlag` (`ETextStyleFlag`), `textColor`,
  shadow settings (`shadowOffset`, `shadowColor`, `shadowAlpha`, `shadowWeight`), `scale` and
  `textWeight` (for SDF fonts), `layoutBuilder`.
- Style flags: `TEXT_STYLE_SHADOW`, `TEXT_STYLE_MONOSPACE`, `TEXT_STYLE_FROM_CAP`,
  `TEXT_STYLE_USE_TAGS`, `TEXT_STYLE_SCISSOR`, `TEXT_STYLE_UPPERCASE`, `TEXT_STYLE_LOWERCASE`.
- Tags (with `TEXT_STYLE_USE_TAGS`): `&#RRGGBB;text&;` — colored text; `&;` closes the tag. Only
  color is supported.
- Custom layouts: implement `ITextLayoutBuilder` (`OnNewLine`, `LayoutChar`) and set it in
  `params.layoutBuilder`; `CRectangleTextLayoutBuilder` (`shared/font/FontLayoutBuilders.h`) wraps
  text inside a rectangle.
