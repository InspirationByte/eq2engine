# framework/core — console, command line, profiler

This document covers the console variable/command system, the command line and the profiling
interface. Headers live in `framework/core/` (`coreLib`) and `public/core/`; implementations of
the registries are in `core/e2core/` (`e2Core`). `ConVar.h`, `ConCommand.h`, `ConCommandBase.h` and
`profiler.h` are included by `core/core_common.h`.

## Console variables and commands

### Common base — `ConCommandBase.h`

`ConVar` and `ConCommand` derive from `ConCommandBase`: name, description, flags, and an optional
*variants* callback that supplies autocomplete values in the console.

Flags (`ECommandBaseFlags`):

| Flag | Meaning |
|---|---|
| `CV_UNREGISTERED` | Not registered automatically; call `ConCommandBase::Register(&var)` manually |
| `CV_CHEAT` | Allowed only when cheats (`__cheats`) are enabled |
| `CV_PROTECTED` | Init-only, cannot be changed at runtime from the console |
| `CV_INVISIBLE` | Hidden from console search and listings |
| `CV_ARCHIVE` | ConVar value is saved to the config file |
| `CV_CLIENTCONTROLS` | Client control command, disabled on pause or when the console is open |

Registration: unless `CV_UNREGISTERED` is set, the constructor registers the object in
`g_consoleCommands`; the destructor unregisters it. Console objects are normally global or
file-static, so they register during static initialization.

### ConVar — `ConVar.h`

A named value stored as string, float and int at the same time.

```cpp
DECLARE_CVAR(r_wireframe, "0", "Render in wireframe", CV_CHEAT);
DECLARE_CVAR_CLAMP(snd_volume, "1.0", 0.0f, 1.0f, "Master volume", CV_ARCHIVE);

static void languageChangeCb(ConVar* var, const char* oldValue)
{
	...
}
DECLARE_CVAR_CHANGE(language, "", languageChangeCb, "The language of the game", CV_ARCHIVE);

if (r_wireframe.GetBool())
	...
```

| Macro | Declares |
|---|---|
| `DECLARE_CVAR(name, value, desc, flags)` | File-static ConVar named `#name` |
| `DECLARE_CVAR_CLAMP(name, value, min, max, desc, flags)` | Static ConVar with value clamping |
| `DECLARE_CVAR_CHANGE(name, value, cb, desc, flags)` | Global ConVar with change callback |
| `DECLARE_CVAR_CLAMP_CHANGE(...)` | Clamp + change callback |
| `DECLARE_CVAR_VARIANTS(name, value, variantsCb, desc, flags)` | ConVar with autocomplete variants |
| `DECLARE_CVAR_RENAME(name, cvname, value, desc, flags)` | C++ name differs from console name |
| `DECLARE_CVAR_G` / `*_G` | Same as above, without `static` (global linkage) |
| `DECLARE_CVAR_F(name)` | `extern ConVar name;` — use a global ConVar from another file |
| `HOOK_TO_CVAR(name)` | Local `ConVar*` found by name through `g_consoleCommands` (other modules) |

Without macros: `ConVar name("name", "default", flags, ConVar::Desc(...), ConVar::Clamp(min, max),
ConVar::ChangeCallback(cb), ConVar::Variants(cb))` — initializers in any order.

API: `GetString`, `GetFloat`, `GetInt`, `GetBool` (`int > 0`); `SetValue(string)`, `SetFloat`,
`SetInt`, `SetBool`, `RevertToDefaultValue`, `GetDefaultValue`; `SetClamp`, `SetCallback`.
The change callback (`void cb(ConVar* var, const char* oldValue)`) is called after the value
changes.

Initial values: a value set for a ConVar before it is registered (`seti <cvar> <value>` in a
config, or a `+cvar value` on the command line executed before the module loaded) is stored and
applied when the ConVar registers.

### ConCommand — `ConCommand.h`

A named console command with a callback taking `CONCOMMAND_ARGUMENTS`
(`const ConCommand* cmd, ArrayCRef<EqStringRef> args`).

```cpp
DECLARE_CMD(map, "Loads a level", 0)
{
	if (CMD_ARGC == 0)
	{
		MsgWarning("Usage: map <name>\n");
		return;
	}
	LoadLevel(CMD_ARGV(0));
}
```

| Macro | Declares |
|---|---|
| `DECLARE_CMD(name, desc, flags) { ... }` | Command with the callback body following the macro |
| `DECLARE_CMD_F(name, desc, flags)` | Command with a forward-declared callback `CONCOMMAND_FN(name)` defined later |
| `DECLARE_CMD_FN(name, func, desc, flags)` | Static command using an existing function |
| `DECLARE_CMD_RENAME(name, cmdname, desc, flags) { ... }` | C++ name differs from console name |
| `DECLARE_CMD_VARIANTS(name, desc, variantsFn, flags) { ... }` | Command with autocomplete variants |
| `DECLARE_CMD_G` / `DECLARE_CMD_FN_G` / ... | Global (non-static) variants |
| `HOOK_TO_CMD(name)` | Local `ConCommand*` found by name |

Inside the callback: `CMD_ARGC` — number of arguments (the command name is not included),
`CMD_ARGV(i)` — argument `i` as `EqStringRef`.

Variants callback signature: `void fn(const ConCommandBase* cmd, Array<EqString>& list, const char* query)`.

### Console registry — `public/core/IConsoleCommands.h`

`g_consoleCommands` holds all registered objects and executes text commands:

- `FindCvar(name)`, `FindCommand(name)`, `FindBase(name)`, `GetAllCommands()`;
- `SetCommandBuffer`, `AppendToCommandBuffer`, `ClearCommandBuffer`,
  `ExecuteCommandBuffer(filterFn, quiet, failedCmds)` — run `;`-separated command text;
- `ParseFileToCommandBuffer(fileName)` — load a `.cfg` file (looked up also in `cfg/`);
- `PushConVarInitialValue(name, value)` — store a value for a ConVar that is not registered yet.

`CommandFilterFn` (`bool fn(const ConCommandBase* cmd, ArrayCRef<EqString> args)`) can reject
commands during execution, e.g. to allow only some commands at startup.

## Command line — `public/core/ICommandLine.h`

`g_cmdLine` holds the application arguments (initialized by `e2Core` at startup).

- `Find(arg, startFrom)` — index of an argument (case-insensitive), or `-1`.
- `GetArgumentsOf(index)` — the arguments following `index` up to the next `+` or `-` argument,
  joined with spaces (values with spaces are quoted). Returns a static buffer: copy it if it must
  be kept.
- `GetArgumentsOf(index, values, maxValues)` — the same arguments as an array.
- `GetParameters()` — all arguments.
- `ExecuteCommandLine(filterFn)` — executes every `+name args...` argument as a console command
  or ConVar assignment.

Convention: `-name [value]` is an application option read by code with `Find`, `+name [args]` is a
console command or ConVar executed through the console.

```
DrvSyn.exe -devmode +language russian +map test_level
```

```cpp
const int idx = g_cmdLine->Find("-devmode");
if (idx != -1)
	...

const char* value = g_cmdLine->GetArgumentsOf(g_cmdLine->Find("-game"));
```

## Profiler — `framework/core/profiler.h`

Profiling markers are compiled in all configurations except `Retail` (`PROFILE_ENABLE`); in
`Retail` the macros expand to nothing, so they can be left in code.

| Macro | Purpose |
|---|---|
| `PROF_EVENT(name)` | Scoped zone: starts here, ends at the end of the enclosing scope |
| `PROF_EVENT_F()` | Scoped zone named after the current function (`__func__`) |
| `PROF_MARKER(name)` | Instant marker |
| `PROF_FRAME_BEGIN(name)` / `PROF_FRAME_END(name)` | Frame boundaries |
| `PROF_RELEASE_THREAD_MARKERS()` | Releases the current thread's marker storage (call before a worker thread exits) |

```cpp
void CMyRenderer::Draw()
{
	PROF_EVENT_F();

	{
		PROF_EVENT("Draw Opaque");
		DrawOpaque();
	}
}
```

Names are `EqStringRef`; pass string literals or strings that outlive the zone. Only one
`PROF_EVENT` per scope is allowed (the macro declares a fixed variable name); open a nested `{ }`
block for further zones.

The implementation (`core/e2core/profiler_core.cpp`) forwards markers to Tracy and can record a
JSON trace with the console commands `ptrace_start` / `ptrace_stop` (file name in the
`ptrace_file` ConVar).
