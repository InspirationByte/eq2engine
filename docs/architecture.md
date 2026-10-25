# EQ2 Engine architecture

This document describes the source layout and the build-level module structure. It is derived from
the directory tree and the `premake5.lua` scripts; when they change, update this document.

Related documents:

| Document | Topic |
|---|---|
| [codestyle.md](codestyle.md) | Code style for new files |
| [framework_ds.md](framework_ds.md) | Containers, strings, smart pointers (`framework/ds`) |
| [framework_math.md](framework_math.md) | Math library (`framework/math`) |
| [framework_core.md](framework_core.md) | Console variables/commands, command line, profiler |
| [framework_platform.md](framework_platform.md) | Message boxes, timers |
| [framework_multithreading.md](framework_multithreading.md) | Threads, locks, signals, TLS, atomics |
| [framework_job_manager.md](framework_job_manager.md) | Job manager and `g_parallelJobs` |
| [keyvalues.md](keyvalues.md) | KeyValues format and API (`framework/utils/KeyValues.h`) |
| [public_core.md](public_core.md) | Core interfaces: interface mechanism, `g_eqCore`, logging, CPU info |
| [public_filesystem.md](public_filesystem.md) | File system: search paths, files, packages, file search |
| [public_localization.md](public_localization.md) | Localization: language files, tokens, helpers |
| [public_rendering_overview.md](public_rendering_overview.md) | Material system overview: materials, variables, proxies, textures, debug drawing, fonts |
| [public_egf_overview.md](public_egf_overview.md) | EGF (Entity Geometry Format): models, motion packages, physics models |
| [rendering_egf_overview.md](rendering_egf_overview.md) | EGF runtime (`shared/studio`): studio cache, model loading, GPU geometry, skins, LODs, drawing |
| [animating_egf_overview.md](animating_egf_overview.md) | EGF animation (`shared/animating`): sequences, activities, blending, pose controllers, events, IK |
| [rendering_matsystem_shaders_overview.md](rendering_matsystem_shaders_overview.md) | Material shaders: `CBaseShader`, pipelines, bind groups, `.def` files, overrides, `BaseUnlit` with GRIM |
| [rendering_grim_overview.md](rendering_grim_overview.md) | GRIM: GPU-driven instance rendering, its shaders, `samples/GPUDriven` |
| [sys_application_overview.md](sys_application_overview.md) | Application framework (`shared/sys`): start-up, game loop, app states, console, configs |
| [input_overview.md](input_overview.md) | Input: key codes, command bindings, vector actions, touch zones, controllers |
| [sound_emitter_system_overview.md](sound_emitter_system_overview.md) | Sound emitter system: script banks, sounding objects, parameter graphs, audio system |
| [scripting_esl_overview.md](scripting_esl_overview.md) | ESL (Eq Script Library): Lua bindings, engine Lua state |
| [tools_overview.md](tools_overview.md) | Content tools: egfCA, animCA, EGFMan2, FCompress, ShaderCooker, TexCooker, localization tool |

## Overview

Equilibrium 2 is a C++17 engine built with premake5. This repository is a git submodule of the game
project (The Driver Syndicate, `src_game/eq2engine`). The engine does not define a game application
itself: a game or sample workspace sets its options, includes the engine directory
(`include "eq2engine"` in `src_game/premake5.lua`) and adds its own application projects on top of
the engine libraries.

Application projects pull engine libraries and third-party libraries by premake usage name. For
example, the game's `DriversGame` project lists engine libraries (`sysLib`, `physicsLib`, ...) and
third-party usages (`bullet2`, `lua`, `Recast`, `lz4`, `meshoptimizer`) together in `uses { ... }`.
Some dependencies in `src_dependency/` (such as `recast`) are therefore used only by the game, not
by engine projects.

Layering, from bottom to top (a layer may only depend on layers below it):

1. **Dependencies** — `src_dependency/`: third-party libraries.
2. **Framework** — `framework/`: platform abstraction, PPMem, containers, math, utilities
   (`coreLib`, `frameworkLib`, static).
3. **Core** — `core/`: the `e2Core` shared library (file system, console, command line, logging,
   localization, jobs) and the material system with RHI backends (shared libraries).
4. **Shared components** — `shared/`: engine subsystems as static libraries (rendering utilities,
   audio, physics, UI, scripting, models, animation, network, system host).
5. **Applications** — `samples/`, `tools/`, `tests/` in this repository, and the game projects in
   the parent repository.

Public interfaces are declared in `public/` and implemented in `core/` or `shared/`.

## Directory map

| Path | Contents |
|---|---|
| `framework/core/` | `core_common.h`, PPMem allocator (`ppmem.h`), console variables/commands base, profiler macros — see [framework_core.md](framework_core.md) |
| `framework/core/platform/` | OS abstraction: files, threads, atomics, timers, job manager, asserts, platform defines — see [framework_platform.md](framework_platform.md), [framework_multithreading.md](framework_multithreading.md), [framework_job_manager.md](framework_job_manager.md) |
| `framework/ds/` | Engine containers and primitives: `Array`, `List`, `Map`, `SlottedArray`, `EqString`, `EqStringRef`, `EqFunction`, `Event`, `Future`, `CRefPtr`, `CWeakPtr`, `defer` — see [framework_ds.md](framework_ds.md) |
| `framework/math/` | Vectors, matrices, quaternions, planes, volumes, bounding boxes, splines, fixed-point math, random — see [framework_math.md](framework_math.md) |
| `framework/utils/` | `KeyValues`, tokenizer, CRC32, Base64, texture atlas / rectangle packing, async work queue, misc geometry — KeyValues: see [keyvalues.md](keyvalues.md) |
| `framework/imaging/` | Image loading and texture formats |
| `framework/types.natvis` | Debugger visualizers for engine types |
| `core/e2core/` | `e2Core` implementation: core, file system, command line, console commands, logger, localization, parallel jobs, CPU services, exception handler, profiler |
| `core/materialsystem1/` | Material system: materials, material vars and proxies, dynamic mesh, texture loader |
| `core/materialsystem1/Renderers/` | Render manager/worker and RHI backends: `Shared/` (common ShaderAPI code), `Empty/` (null), `NVRHI/` (D3D12/Vulkan), `WGPU/` (WebGPU) |
| `core/materialsystem1/Shaders/` | Material shader definitions: `Base/` (built into `eqBaseShaders`), `Engine/` |
| `public/core/` | Core interfaces: `IDkCore`, `IFileSystem`, `IConsoleCommands`, `ICommandLine`, `ILocalize`, `IEqParallelJobs`, `IEqCPUServices`, `InterfaceManager` — see [public_core.md](public_core.md) |
| `public/materialsystem1/` | Material system interfaces, `BaseShader`, `MeshBuilder`, render definitions; `renderers/` holds RHI interfaces (`IShaderAPI`, `ITexture`, `IGPUBuffer`, ...) |
| `public/{audio,font,input,physics,render,egf,dkphysics}/` | Public headers of the corresponding `shared/` libraries |
| `shared/dpk/` | DPK package and ZIP readers/writer |
| `shared/font/` | Font loading, caching and text layout |
| `shared/render/` | Render utilities: debug overlay, particles, decals, sprites, render lists, blur/sort compute |
| `shared/grim/` | GRIM — GPU-driven rendering instance manager |
| `shared/studiofile/`, `shared/studio/` | EGF model loading and studio geometry cache |
| `shared/egf/` | EGF/motion package generator and source model loaders (ESM, FBX, OBJ) — tools only |
| `shared/animating/` | Skeletal animation, bone setup, activities and events |
| `shared/audio/` | Audio system on OpenAL, sound emitters/scripts/mixer; `source/` holds WAV/OGG sources |
| `shared/physics/` | EqPhysics engine on top of Bullet collision |
| `shared/dkphysics/` | Legacy Darktech physics on Jolt (deprecated, kept for EGFMan2) |
| `shared/equi/` | EqUI interface library: manager, controls, scheme |
| `shared/imgui_backend/` | ImGui host and material-system renderer backend |
| `shared/scripting/` | ESL — Lua binding library |
| `shared/movie/` | WebM movie player: AV1 video (dav1d), Vorbis audio, YUV to RGBA conversion on GPU (`MovieYUV` shader) |
| `shared/network/` | UDP networking |
| `shared/input/` | Input command binder |
| `shared/sys/` | Application host: main loop, window, states, console, joystick, config loader; `sys/scripting/` exposes engine subsystems to ESL, `sys/imgui/` is the ImGui platform backend |
| `shared/components/`, `shared/entity/` | Component system and data tables (no engine project; compiled by consumers) |
| `tools/` | Content tools (see below); `localization_tool/` is a separate C# project, `imp-exp blender/` is a Blender add-on |
| `samples/` | Sample applications, each with its own workspace `premake5.lua` |
| `tests/` | GoogleTest-based test applications |
| `src_dependency/` | Third-party libraries, each with its own `premake5.lua` |
| `utils/` | premake5 binaries, premake modules (`.premake_modules/`), environment preparation scripts |
| `android-project/` | Android Gradle project |
| `build/` | Generated project files, objects and binaries (not source) |

## Premake

The build is described with premake5 (binaries in `utils/premake5`, `utils/premake5.exe`).
`premake5.lua` in the engine root is the entry point included by workspaces; every module directory
has its own `premake5.lua`. Stock premake is extended by modules from `utils/.premake_modules/`,
loaded at the top of `premake5.lua`:

| Module | Purpose |
|---|---|
| `usage` | `usage` / `uses` dependency model (see below) |
| `properties` | `property` / `properties` reusable settings blocks (see below) |
| `unitybuild` | `unitybuild`, `maxfilesinunity`, `excludefromunity` — MSVC unity build support |
| `androidndk` | `androidndk` action for Android builds |
| `vscode` | `vscode` action and `vscode_*` launch settings |
| `wxwidgets` | wxWidgets integration for GUI tools |

### usage / uses

A `usage "<name>"` block declares what a consumer receives: include directories, defines, links
and nested `uses`. A project lists its dependencies with `uses { ... }` instead of writing
`links` / `includedirs` by hand.

- Using a project links it and resolves its nested `uses`; if a usage with the same name exists,
  the project is applied first, then the usage.
- `uses` are recursive. Link dependencies of static libraries pass through to the final consumer.
- `uses` does not respect configuration filters; a `usage` block may contain filters.
- Names are case-insensitive (`coreLib` and `corelib` are the same).
- Every engine project and third-party library is available by name: `uses { "e2Core", "sysLib",
  "bullet2", "Recast" }`. Plain usages without a project also exist: `public` (`public/` and
  `framework/` include paths), `shared` (`shared/` include path), `eqRHIBaseLib`.
- `--usage-optimization-off` disables usage-resolution caching to debug dependency issues.

### property / properties

A `property "<name>"` block is a named set of settings (with filters) that projects and workspaces
apply with `properties { ... }`. Properties are resolved when projects are baked; a property must
not apply other properties.

Properties defined in `premake5-properties.lua`:

| Property | Effect |
|---|---|
| `e2_ws_settings`, `e2_ws_configurations` | Workspace language, output directories, configurations and platforms |
| `windows_msvc`, `gcc_clang` | Compiler-specific options and disabled warnings |
| `staticlib`, `sharedlib` | Project kind |
| `unitybuild` | Unity build, up to 30 files per unit (unless `BUILD_SINGLE_FILE`) |
| `app` | Debug directory/environment for executables |
| `tools` | Disables the project in `Profile` and `Retail` |
| `thirdpartylib` | Output directory for third-party libraries |

Some properties come from dependencies: `concurrency_vis` (`src_dependency/cv_sdk`), `live_pp`
(`src_dependency/LivePP`), `tracy` (`src_dependency/tracy`). Workspaces define their own, e.g.
`gameapp`.

A typical engine project:

```lua
project "fontLib"
	properties { "staticlib", "unitybuild" }
	uses { "public", "shared" }
	files {
		"font/**",
		PUBLIC_DIR.."/font/"
	}
```

## Build projects

Premake groups and projects. "Uses" lists engine projects; third-party usages are in parentheses.

### Framework (`framework/premake5.lua`)

| Project | Kind | Sources | Uses |
|---|---|---|---|
| `coreLib` | static | `framework/core/**` | public |
| `frameworkLib` | static | `framework/{ds,utils,math,imaging}/**` | public (stb) |

### Core (`core/premake5.lua`)

| Project | Kind | Sources | Uses |
|---|---|---|---|
| `e2Core` | shared | `core/e2core/**`, `public/core/**` | coreLib, frameworkLib, dpkLib |
| `BaseShader` | static | `public/materialsystem1/BaseShader*` | public |
| `eqMatSystem` | shared | `core/materialsystem1/*` | coreLib, frameworkLib, e2Core, BaseShader |
| `eqBaseShaders` | shared | `core/materialsystem1/Shaders/*`, `Shaders/Base/**` | coreLib, frameworkLib, e2Core, BaseShader |
| `eqNullRHI` | shared | `Renderers/Empty/**` | e2Core, eqRHIBaseLib |
| `eqNVRHI` | shared | `Renderers/NVRHI/**` (D3D12 on Windows, Vulkan) | e2Core, eqRHIBaseLib (nvrhi) |
| `eqWGPURHI` | shared | `Renderers/WGPU/**` | e2Core, eqRHIBaseLib (wgpu-dawn) |

`eqRHIBaseLib` is a usage (not a project) that adds `Renderers/Shared/**` to every RHI backend.
The material system group is built only when `ENABLE_MATSYSTEM` is on.

### Components (`shared/premake5.lua`)

All are static libraries using `public` and `shared` include paths.

| Project | Sources | Uses |
|---|---|---|
| `dpkLib` | `dpk/**` | (lz4, zlib) |
| `fontLib` | `font/**` | — |
| `renderUtilLib` | `render/**` | — |
| `grimLib` | `grim/**` | renderUtilLib (imgui) |
| `studioFileLib` | `studiofile/**` | (bullet2, zlib) |
| `studioLib` | `studio/**` | studioFileLib |
| `animatingLib` | `animating/**` | studioLib |
| `equiLib` | `equi/**` | fontLib (imgui) |
| `imguiBackendLib` | `imgui_backend/**` | (imgui) |
| `networkLib` | `network/**` except `NETThread.cpp` | (zlib) |
| `soundSystemLib` | `audio/**` | (minivorbis, openal-soft, imgui) |
| `physicsLib` | `physics/**` | (bullet2) |
| `movieLib` | `movie/**` | (dav1d, nestegg, minivorbis; disabled on Android) |
| `scriptLib` | `scripting/**` | (lua) |
| `sysLib` | `sys/**`, `input/**` | scriptLib, renderUtilLib, equiLib, movieLib, imguiBackendLib (SDL2, imgui_lua, tracy) |
| `egfLib` | `egf/**` | studioFileLib (bullet2, zlib, openfbx, meshoptimizer) — `ENABLE_TOOLS` only |
| `dkPhysicsLib` | `dkphysics/**` | studioLib, animatingLib (jolt) — `ENABLE_TOOLS` only |

### Tools (`tools/premake5.lua`, `ENABLE_TOOLS`)

Console apps: `fcompress` (package compression), `egfca` (model compiler), `animca` (animation
compiler), `texcooker` (texture cooker), `shadercooker` (Slang shader batch compiler).
GUI app (`ENABLE_GUI_TOOLS`, Windows only): `egfMan2` (model viewer on `sysLib` + ImGui, uses `nfd`).
Tools are not built in `Profile` and `Retail` configurations.
`egfinfo` and `matsys_wxapp_template` have no project.

### Tests (`premake5-tests.lua`, `tests/premake5.lua`)

Workspace `auto_tests` (tools and material system off, single-file build), generated into
`build/auto_tests/`. Projects: `testsCommonLib`, `ds_tests` (`tests/ds/`), `ppmem_tests`
(`tests/ppmem/`), `scripting_tests` (`tests/scripting/`), all on GoogleTest. PPMem is enabled only in
`Debug`; other configurations define `PPMEM_DISABLED` and `ppmem_tests` skips there.

### Samples

`samples/GPUDriven` and `samples/Demo_RockPaperScissors` each define a workspace that includes the
engine and one application project (`WindowedApp`, or `SharedLib` on Android) built on `sysLib`.

## Build options and configurations

Options a workspace can set before including the engine (defaults in `premake5.lua`):
`WORKSPACE_NAME`, `ENGINE_DIR`, `WINSDK_VER`, `BUILD_SINGLE_FILE` (disables unity build),
`ENABLE_TOOLS`, `ENABLE_GUI_TOOLS`, `ENABLE_MATSYSTEM`, `ENABLE_TESTS`, `ENABLE_LIVEPP`.
Dependency paths for SDL2 and OpenAL can be overridden with `SDL2_DIR` / `OPENAL_DIR`.

`BUILD_DIR` (default `<main script dir>/build`) holds everything a workspace generates: project files
(including its own copies of the engine projects), intermediates, libraries and binaries. Each
workspace therefore builds independently; workspaces sharing a main script folder must set
different `BUILD_DIR` values, as `premake5-tests.lua` does.

Configurations: `Debug`, `Release` (optimized, asserts and debug UI on), `ReleaseAsan`,
`Profile` (`_PROFILE`, no asserts, no RTTI), `Retail` (`_RETAIL`, no debug features, no RTTI).
Platforms: x64 on Windows and Linux (clang), arm/arm64 on Android.

Engine projects use the `unitybuild` property (up to 30 files per unity unit), so `.cpp` files of a
project may be compiled together: avoid file-scope name clashes between `.cpp` files of one project.

## Module model

- Subsystems in shared libraries expose abstract `I`-prefixed interfaces from `public/`; consumers
  access them through global singletons (`g_` prefix, e.g. `g_fileSystem`, `g_localizer`).
- `e2Core` registers and exports core interfaces; other DLLs (material system, RHI backends,
  shaders) are separate shared libraries loaded at runtime.
- Static libraries in `shared/` are linked directly into the application.

## Where to put new code

| New code | Location |
|---|---|
| Container, math, generic utility with no engine services | `framework/` |
| Core service exposed to all modules | interface in `public/core/`, implementation in `core/e2core/` |
| Rendering backend-independent material/shader code | `core/materialsystem1/` |
| New engine subsystem | `shared/<name>/` + project in `shared/premake5.lua`; public headers in `public/<name>/` if other modules need them |
| Content tool | `tools/<name>/` + project in `tools/premake5.lua` |
| Tests | `tests/<area>/` + project in `tests/premake5.lua` |
| Third-party library | `src_dependency/<name>/` with its own `premake5.lua`, included from `src_dependency/premake5.lua` |
