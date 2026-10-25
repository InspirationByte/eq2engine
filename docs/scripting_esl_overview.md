# ESL overview — `shared/scripting`, `shared/sys/scripting`

**ESL — Eq Script Library.** A Lua 5.4 binding library (`src_dependency/lua54`) for exposing engine
C++ types and functions to Lua and for calling Lua from C++.

| Part | Project | Contents |
|---|---|---|
| `shared/scripting/` | `scriptLib` | Generic binder: `esl::ScriptState`, type binding macros, Lua references and tables, calls into Lua, events |
| `shared/sys/scripting/` | `sysLib` | The engine Lua state (`eslSys`) and bindings of engine systems: core, console, file system, KeyValues, math, input, sound, debug drawing, material system, EqUI, movie player, animation |
| `tests/scripting/` | `scripting_tests` | GTest tests of `scriptLib`; also the most complete set of binding examples |

Headers to include:

```cpp
#include "scripting/esl.h"			// ScriptState, Object, Any, traits
#include "scripting/esl_luaref.h"	// LuaRawRef, LuaRef<TYPE>, LuaTable
#include "scripting/esl_bind.h"		// binding macros, FunctionCall
#include "scripting/esl_event.h"	// LuaEvent (optional)
```

In engine code `shared/sys/scripting/sys_esl.h` includes the first three and adds the `eslSys` API.

## Core types — `esl.h`, `esl_luaref.h`

- `esl::ScriptState` — thin wrapper over `lua_State*` (implicitly converts to it):
  - running code: `RunChunk(text, name)`, `RunFileBuffer(stream, name)`,
    `LoadFileBuffer(stream, name)`;
  - globals: `SetGlobal(name, value)`, `GetGlobal<T>(name)`;
  - classes: `RegisterClass<T>()`, `RegisterClassStatic<T>(key, value)`, `GetClassTable<T>()`,
    `GetClassStatic<T, V>(key)`;
  - values: `PushValue(value)`, `GetValue<T>(index)`, `CreateTable()`, `MakeObject<T>(args...)`;
  - calls: `CallFunction<R>(globalName, args...)`;
  - GC control: `GCStop`, `GCRestart`, `GCStep`, `GCCollect`; `ThrowError(fmt, ...)`.
- `esl::LuaRawRef` / `esl::LuaRef<LUA_T...>` — registry reference that keeps a Lua value alive from
  C++. Aliases: `LuaFunctionRef`, `LuaTableRef`, `LuaUserRef`.
- `esl::LuaTable` — table reference with `Get<V>(key)`, `SafeGet(key, default)`, `Set(key, value)`,
  `HasKey`, `Remove`, `operator[]`, `Length()`, `IPairs()` iteration.
- `esl::ResultWithValue<V>` — result of `GetValue`, `GetGlobal`, `LuaTable::Get` and Lua calls:
  `success`, `errorMessage`, `value` (`*result`). It holds a `runtime::StackGuard` that restores
  the Lua stack top when the result is destroyed.
- `esl::Object<T>` — value on the Lua stack typed as `T` (returned by `MakeObject`).
- `esl::Any<N>` — return type for functions that push `N` values themselves (multiple returns), or
  that receive multiple returns from a Lua call.
- `esl::runtime::*` — low-level stack functions (`PushValue`, `GetValue`, `SetGlobal`, ...) used by
  the wrappers.

## Binding a type

A binding has two parts: the type declaration (a header, visible to every binding that uses the
type) and the member list (one `.cpp`).

```cpp
// declaration: C++ type, Lua class name, push type
EQSCRIPT_BIND_TYPE_NO_PARENT(Spline, "Spline", esl::BY_REF)
EQSCRIPT_BIND_TYPE_WITH_PARENT(TerrainSpline, Spline, "TerrainSpline")	// inherits push type

// members
EQSCRIPT_TYPE_BEGIN(Spline)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_CONSTRUCTOR(int)
	EQSCRIPT_BIND_FUNC(Load)
	EQSCRIPT_BIND_FUNC_NAMED("SaveStandard", Save)
	EQSCRIPT_BIND_VAR(name)
	EQSCRIPT_BIND_VAR_EX_GET_SET(flags, GetFlags, SetFlags)
EQSCRIPT_TYPE_END

// registration in a Lua state
state.RegisterClass<Spline>();
state.RegisterClass<TerrainSpline>();
```

```lua
spl = TerrainSpline.new()
spl:Load(section)
spl.name = "road"
```

- `RegisterClass<T>()` creates the global class table `_G[className]` (scripts may add Lua
  functions to it) and the metatable. `new` exists only when the type has constructors.
- Members: `EQSCRIPT_BIND_FUNC[_NAMED][_OVERLOAD]`, `EQSCRIPT_BIND_STATIC_FUNC[_OVERLOAD]` (a free
  function whose first argument is the object), `EQSCRIPT_BIND_VAR[_NAMED]`,
  `EQSCRIPT_BIND_VAR_EX_SET` / `EQSCRIPT_BIND_VAR_EX_GET_SET` (property with setter / getter),
  `EQSCRIPT_BIND_OP(add)` and `EQSCRIPT_BIND_OP_CUSTOM` / `EQSCRIPT_BIND_OP_TOSTRING` (metamethods),
  `EQSCRIPT_CLONE_FUNC()` (`obj:Clone()`), `EQSCRIPT_BIND_EVENT`.
- Member names must be unique per class: bind overloads under different names with the
  `_NAMED_OVERLOAD` variants.
- `EQSCRIPT_PUBLIC_BINDER(Class)` inside a class gives the binder access to private members.
- A bound function may take `esl::ScriptState` (or `const esl::ScriptState&`) as its first
  parameter; it receives the calling state and is not a Lua argument.
- Free functions are exposed with `EQSCRIPT_CFUNC(func)` / `EQSCRIPT_CFUNC_OVERLOAD(func, R, (Args))`
  as a global or table value: `state.SetGlobal("StringId", EQSCRIPT_CFUNC(StringId))`.

## Push types and ownership

The push type set in the declaration defines how objects of the type live in Lua:

| Push type | Lua representation | Lifetime |
|---|---|---|
| `esl::BY_VALUE` | Object stored inside the userdata (copy), or a reference to a C++ object | Copies are owned by Lua; for math and other small value types |
| `esl::BY_REF` | Pointer to a C++ object | Owned by C++, except objects created by `T.new()` or returned with `ToLua<T*>`, which Lua deletes on GC |
| `esl::REF_PTR` | Pointer to a `RefCountedObject` | Lua holds a reference (`Ref_Grab` / `Ref_Drop`) |
| `esl::WEAK_REF` | Pointer plus the object's weak handle (`WeakRefObject`) | Turns into `nullptr` for C++ after the object is destroyed |

- `esl::ABSTRACT` flag (`esl::BY_REF | esl::ABSTRACT`) — interfaces without constructors; allowed
  only with `BY_REF` or `WEAK_REF`.
- Parameter / return traits change ownership of a single call:
  - `esl::ToCpp<T*>` — parameter: C++ takes ownership of an object created in Lua;
  - `esl::ToLua<T*>` — return value: Lua takes ownership of the returned object;
  - `esl::ByValueRef<T>` / `esl::ByValueCopy<T>` — pass a `BY_VALUE` object as reference or copy.

  Traits are given with `ESL_APPLY_TRAITS(R, Args...)` in member bindings or as extra
  `EQSCRIPT_CFUNC` arguments: `EQSCRIPT_CFUNC(S_DbgBox, esl::ToLua<DDBox>, void)`.
- Lua does not call `__eq` when comparing userdata with `nil`, so a destroyed `WEAK_REF` object is
  not `== nil` in Lua. Bind a helper that checks the pointer in C++ (see the note in
  `esl_bind.h`).

## Calling Lua from C++

```cpp
using UpdateFunc = esl::runtime::FunctionCall<bool, float>;

esl::LuaFunctionRef updateFn = *state.GetGlobal<esl::LuaFunctionRef>("OnUpdate");
auto result = UpdateFunc::Invoke(updateFn, deltaTime);
if (!result)
	MsgError("OnUpdate failed: %s\n", result.errorMessage.ToCString());
else if (*result)
	...
```

- `FunctionCall<R, Args...>::Invoke(funcRef, args...)` runs the function in protected mode and
  returns `ResultWithValue<R>`; errors never propagate to C++ as Lua errors.
- `state.CallFunction<R>(name, args...)` calls a global function by name.
- Store Lua callbacks as `LuaFunctionRef` members; release them (`Release()` or `= nullptr`) before
  the Lua state is destroyed.

## Events — `esl_event.h`

`esl::LuaEvent` is a list of Lua handlers on a C++ object:

```cpp
class CVehicle
{
public:
	ESL_DECLARE_EVENT(OnCollision);		// esl::LuaEvent m_eslEventOnCollision
};
// member list: EQSCRIPT_BIND_EVENT(OnCollision)

// vehicle is CVehicle*
ESL_DECLARE_EVENT_CALL(vehicle, OnCollision, const Vector3D&);
ESL_CALL_EVENT(OnCollision, hitPos);
```

```lua
handle = vehicle.OnCollision:AddHandler(function(pos) ... end)
vehicle.OnCollision:RemoveHandler(handle)	-- or let `handle` be collected
```

`AddHandler` returns a `LuaEventHandle` (`REF_PTR`); the handler is removed when the handle is
released. Call `esl::LuaEvent::Bind(state)` once per state (done by `eslSys`).

## Engine Lua state — `eslSys` (`sys_esl.h`)

`sysLib` owns one global Lua state:

- `eslSys::GetScriptState()` — creates the state on first use and returns it. The state may be
  used only from the thread that created it (asserted).
- `eslSys::DestroyScriptState()` — runs a full GC and closes the state.
- `eslSysInit(state)` — registers all engine bindings and runs `scripts/lua/engine.lua`; called by
  the application, not by the engine. `eslSysTerm()` removes Lua console commands, input bindings
  and ImGui handlers created by scripts.
- Errors: `LUA_CHECK_CALL(result, fmt, ...)` logs a failed `ResultWithValue` with the caller
  location and sets the error flag (`GetLuaErrorState`, `ClearLuaErrorState`); `GetLastLuaError()`
  returns and resets the last runtime error.
- `eslSys::GetOrCreateGlobalTable(L, name)` — global namespace table for bindings.

State setup:

- Memory goes through PPMem (`PPDReAlloc`), tagged `LuaString`, `LuaTable`, `LuaFunction`, ...
- Opened libraries: base, `package`, `table`, `coroutine`, `string`, `math`, `debug`; `io` and `os`
  are opened only for the debugger. `loadfile` is replaced by a version that reads through
  `g_fileSystem`.
- Tracy Lua zones are registered; with `TRACY_ENABLE` a call / return hook profiles Lua functions.
- Debugger (non-retail): with environment variable `LOCAL_LUA_DEBUGGER_VSCODE=1` the
  [Local Lua Debugger](https://marketplace.visualstudio.com/items?itemName=ismoh-games.second-local-lua-debugger-vscode)
  is started from `LOCAL_LUA_DEBUGGER_FILEPATH`; Lua runtime errors break in the Lua debugger, and
  so do C++ asserts when no native debugger is attached.
- On an unhandled native exception the Lua stack trace is printed.
- Non-retail: console input that fails as a console command is executed as Lua; `=expr` is run as
  `ConsolePrint(expr)`.

## Engine bindings

`eslSysInit` registers, in order:

| File | Lua globals and classes |
|---|---|
| `sys_esl_core.cpp` | `core` (`GetConfig`), `localize`, `StringId`, `StringId24`; `Msg*`, `MsgBox*`, `print`; `console` (find / create / execute commands and ConVars), `cmdline`, `ConVar`, `ConCommand`; `fileSystem`, `fnmPath*` path helpers |
| `sys_esl_keyvalues.cpp` | `KeyValues`, `KVSection`, `KVPairValue`, `KV_GetValue*`, `KV_GetVector*D`, `KV_PrintSection` |
| `sys_esl_math.cpp` | `Vector2D/3D/4D`, `IVector2D`, `MColor`, `Quaternion`, `Matrix3x3/4x4`, `Transform3D`, `Plane`, `BoundingBox`, `Rectangle`, `IRectangle`, random generators; `f_*` scalar helpers, `q*` quaternion functions, `AngleVectors`, `VectorAngles` |
| `sys_esl_math_utils.cpp` | `Spline3D`, 2D intersection tests, `AngleDiff`, `AnglesDiff` |
| `sys_esl_engine.cpp` | `debugoverlay` and `Dbg*` debug shapes, `eqImGui` (ImGui handlers and debug menus), `input`, `sounds` (`CSoundEmitterSystem`, `EmitParams`), `EqStateMgr`, `host` (frame time, video modes, text input), `buildInfo` |
| `sys_esl_matsystem.cpp` | `matSystem`, `IMaterial` |
| `sys_esl_equi.cpp` | `equi` (UI manager), `equi::*` controls, `equi_cast`, `fonts`, `wstring` |
| `sys_esl_movie.cpp` | `CMoviePlayer`, `CLuaMoviePlayer` |
| `sys_esl_animating.cpp` | `AnimatingEGF` |

Add a new module as a `bool eslSysXxxInit(const esl::ScriptState& state)` function and call it from
`eslSysInit` with `ESL_SYS_INIT` (or `ESL_SYS_INIT_EXT` for a function defined outside `sysLib`).

## Notes

- `LUA_SET_GLOBAL_CONST` / `LUA_SET_GLOBAL_ENUMCONST_3` in `sys_esl.h` are deprecated; use
  `state.SetGlobal` or `RegisterClassStatic`.
- `ESL_TRACE` in `esl.h` enables verbose logging of the binder (`ESL_VERBOSE_LOG`).
