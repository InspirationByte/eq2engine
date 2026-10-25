# public/core — core interfaces

`public/core/` declares the services of the `e2Core` shared library (implemented in
`core/e2core/`). Every module accesses them through global singletons. `InterfaceManager.h` and
`Logger.h` are included by `core/core_common.h`; include the other headers when needed:

```cpp
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
```

| Header | Singleton | Purpose |
|---|---|---|
| `IDkCore.h` | `g_eqCore` | Core init/shutdown, config, module loading, interface registry |
| `IFileSystem.h` | `g_fileSystem` | Layered file system, packages, file search — see [public_filesystem.md](public_filesystem.md) |
| `IPackFileReader.h` | — | Reader of a single package file |
| `IConsoleCommands.h` | `g_consoleCommands` | ConVar/ConCommand registry and command buffer — see [framework_core.md](framework_core.md) |
| `ICommandLine.h` | `g_cmdLine` | Application command line — see [framework_core.md](framework_core.md) |
| `ILocalize.h` | `g_localizer` | Localized strings — see [public_localization.md](public_localization.md) |
| `IEqParallelJobs.h` | `g_parallelJobs` | Engine job manager — see [framework_job_manager.md](framework_job_manager.md) |
| `IEqCPUServices.h` | `g_cpuCaps` | CPU information and instruction set support |
| `Logger.h` | — | `Msg`, `MsgWarning`, `MsgError`, `DevMsg` |
| `InterfaceManager.h` | — | Interface declaration / export macros |

## Interface mechanism — `InterfaceManager.h`

- Core interfaces derive from `IEqCoreModule` (`IsInitialized()`, `GetInterfaceName()`) and declare
  a unique versioned name with `CORE_INTERFACE("E2_Filesystem_011")`. Change the version string
  when the interface layout changes.
- `INTERFACE_SINGLETON(IInterface, g_name)` (in the interface header) declares the global accessor
  `g_name`. Inside `e2Core` it calls the exported getter directly; in other modules it looks the
  interface up by name on first use. Use it like a pointer: `g_fileSystem->Open(...)`.
- `EXPORTED_INTERFACE(IInterface, CImplementation)` (in the implementation `.cpp` inside `e2Core`)
  creates the static instance and exports its getter.
- Export macros: `IEXPORTS` (exported from `e2Core`, imported elsewhere), `DLL_EXPORT`,
  `DLL_IMPORT`, `FUNC_EXPORTS`, `CLASS_EXPORTS`.

Interfaces implemented in other modules (material system, renderers, game libraries) register
themselves at runtime with `g_eqCore->RegisterInterface(this)` and unregister on destruction:

```cpp
class IMyService : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_MyService_001")
	...
};

CMyService::CMyService()	{ g_eqCore->RegisterInterface(this); }
CMyService::~CMyService()	{ g_eqCore->UnregisterInterface<CMyService>(); }

IMyService* service = g_eqCore->GetInterface<IMyService>();
```

## Core — `IDkCore.h` (`g_eqCore`)

- `Init(CoreAppInitParameters)` — `appName`, `appConfigName` (default `E2.CONFIG`) and the command
  line; `Shutdown()`. Called by the application host, not by modules.
- `GetConfig()` — the parsed application config (`E2.CONFIG`) as a `KVSection`; see
  [keyvalues.md](keyvalues.md). Sections such as `ApplicationDebug`, `FileSystem`, `Console`,
  `MaterialSystem`, `RegionalSettings` are read by the corresponding subsystems.
- `GetDebugSettings()` — `CoreDebugSettings` from `ApplicationDebug`: full crash dumps,
  crash on assert, assert prompt in debugger, print memory leaks on exit.
- `GetApplicationName()`.
- Modules: `OpenModule(name, outError)`, `CloseModule`, `GetProcedureAddress`;
  `OnModuleLoaded` / `OnModuleUnloaded` notifications.
- Interface registry: `RegisterInterface<T>(iface)`, `UnregisterInterface<T>()`,
  `GetInterface<T>()` (or the name-based overloads).
- `AddExceptionCallback` / `RemoveExceptionCallback` — functions called when the application
  crashes (e.g. to flush state).

```cpp
const KVSection* regionalCfg = g_eqCore->GetConfig().FindSection("RegionalSettings", KV_FLAG_SECTION);
```

## File system — `IFileSystem.h` (`g_fileSystem`)

Layered file access (game/data/root search paths, packages, file search). See
[public_filesystem.md](public_filesystem.md).

## Logging — `Logger.h`

| Macro / function | Output |
|---|---|
| `Msg(fmt, ...)` | Normal message |
| `MsgInfo`, `MsgAccept` | Informational / success (highlighted) |
| `MsgWarning(fmt, ...)` | Warning |
| `MsgError(fmt, ...)` | Error |
| `DevMsg(DEVMSG_<CATEGORY>, fmt, ...)` | Developer message, printed only when the category is enabled |

- Messages are printf-style (format checked by the compiler) and should end with `\n`.
- Developer categories: `DEVMSG_CORE`, `DEVMSG_FS`, `DEVMSG_LOCALE`, `DEVMSG_MATSYSTEM`,
  `DEVMSG_RENDER`, `DEVMSG_SOUND`, `DEVMSG_NETWORK`, `DEVMSG_GAME`. Enable them with the
  `developer <core|filesys|locale|matsys|renderer|sound|network|game|all|disable>` console command
  or `DeveloperMode` in the `ApplicationDebug` config section.
- `SetSpewFunction(fn)` redirects log output (`void fn(ESpewType type, const char* text)`), e.g.
  to the in-game console.

## Localization — `ILocalize.h` (`g_localizer`)

Token-based localized strings, language files and `LocalizedString` / `DKLOC` helpers. See
[public_localization.md](public_localization.md).

## CPU information — `IEqCPUServices.h` (`g_cpuCaps`)

`GetFrequency()`, `GetCPUCount()`, `GetCPUVendor()`, `GetBrandName()`, `GetCPUFamily()` and
instruction set checks (`IsCPUHasSSE2()`, `IsCPUHasSSE41()`, `IsCPUHasSSE42()`, ...).
`cpuInstrDefs.h` contains SIMD helper macros and vector types for intrinsics.
