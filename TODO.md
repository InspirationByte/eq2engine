# TODO

Issues noticed while writing the documentation in `docs/`. None of them has been fixed yet; each
needs review by the code owner before changing.

## Possible bugs

- [ ] `core/e2core/Localize.cpp:213` — `RemoveTokensFile` calls `m_tokens.remove(it)` and then
  `++it` on the removed iterator (use after free). `Map::remove` returns the next iterator; the
  loop should continue from it.
- [ ] `core/e2core/Localize.cpp:302` — `AddToken(const char*, const wchar_t*)` passes the wide
  string to `LocalizeConvertSymbols` through `(char*)`. The function parses it as a narrow string
  and stops on the second byte of the first character, so `\n` / `\t` are not converted for this
  overload.
- [ ] `core/e2core/Localize.cpp:302`, `:314` — both `AddToken` overloads cast away `const` and
  modify the caller's string in place (undefined behavior for literals).
- [ ] `core/e2core/Localize.cpp:160` — when `RegionalSettings` is missing, the message says
  "force english" but `m_language` stays empty and the `$LOCALIZE$` search path is already removed.
- [ ] `public/materialsystem1/IMaterialSystem.h:235` — template `SetupDrawDefaultUP(topology,
  const VERT* verts, ...)` passes `&verts` (address of the local pointer) instead of `verts`.
- [ ] `public/render/IDebugOverlay.h:62-63` — `DDNodeBase::nameHash` and `frameindex` are not
  initialized. Without `Name()` the primitive may be treated as named by
  `CDebugOverlay::CheckNodeLifetime` (`nameHash != 0`) and replaced by an unrelated primitive.
- [ ] `framework/core/platform/eqthread.h:101`, `:162` — `CScopedMutex` and `CScopedWriteLocker`
  with `blocking = false` unlock in the destructor even if the lock was not acquired.
- [ ] `framework/core/platform/eqatomic.h:62` — `Atomic::CompareExchangeWeak` takes `oldVal` by
  value, so the updated expected value is not returned to the caller.
- [ ] `framework/core/platform/eqjobmanager.cpp:260` — `CEqJobManager::AllJobsCompleted` dequeues a
  job and enqueues it back to check the queue, which changes the queue order (and is racy with
  workers).
- [ ] `framework/ds/Map.h:168`, `:499` — `clear()` and `remove()` read `item->next` after calling
  `item->~Item()`. Works in practice, but is undefined behavior; read `next` before destroying.

- [ ] `shared/font/FontCache.cpp:253` — `GetFamily` returns the default family for unknown names,
  so `GetFont(..., defaultIfNotFound = false)` never returns `nullptr` once a default font exists.

## Code quality

- [ ] `shared/font/FontCache.cpp:47` — font description loading uses the deprecated
  `KV_LoadFromFile`.
- [ ] `shared/studio/StudioGeom.cpp:708` — `.skin` loading uses the deprecated `KV_LoadFromFile`.
- [ ] `tools/fcompress/fcompress_main.cpp:390`, `:520` — config and KeyValues conversion use the
  deprecated `KV_LoadFromFile` / `KV_GetValue*`.
- [ ] `public/egf/*.h`, `shared/egf`, tools — headers and comments still call EGF "Equilibrium
  Geometry Format" / "Equilibrium Graphics File"; the format is now named Entity Geometry Format.

- [ ] `framework/core/ConCommand.h:17` — `CMD_ARG_CONCAT` uses an undefined `i` and appends the
  separator once after the loop instead of between arguments. Appears unused.
- [ ] `core/e2core/profiler_core.cpp:158` — `ptrace_stop` description is "Performance trace start"
  (copied from `ptrace_start`).
- [ ] `framework/core/platform/eqthread.h:222`, `:237` — the usage comment mentions a non-existent
  `StartWorker()` and its worker example calls `StartThread` instead of `StartWorkerThread`.
- [ ] `public/core/IEqParallelJobs.h:27` — comment mentions `JOB_FLAG_DELETE`, which does not exist
  (the job is created with `DeleteOnFinish`).
- [ ] `framework/math/Utility.cpp:36`, `:50` — `PointToScreen` returns `true` when the point is
  **behind** the camera; the name suggests the opposite. Consider renaming the result or
  documenting it in `Utility.h`.
- [ ] `core/e2core/Localize.cpp:113` — `Init` lower-cases `m_language` again after
  `SetLanguageName` already did it.
- [ ] `core/e2core/Localize.cpp` — style does not match `docs/codestyle.md` (C-style casts,
  `_FindToken` reserved identifier, missing `override`, `pszDefaultToken = 0`, mixed `if(` /
  `if (`). Only relevant if the file is refactored.
- [ ] `shared/scripting/esl_bind.h:338`, `:341` — `EQSCRIPT_BIND_VAR_EX_SET_NAMED` is defined twice
  (identical definitions).
- [ ] `shared/sys/scripting/sys_esl_core.h:31-81` — the `ENABLE_MULTIPLAYER` block still binds
  `Networking::Buffer` / `CNetworkThread` with the old OOLUA macros (`OOLUA_PROXY`), which do not
  exist in ESL; it would not compile with `ENABLE_MULTIPLAYER`.
- [ ] `shared/sys/scripting/sys_esl_engine.h:31` — the Lua class of `DDOrientedBox` is named
  `"DDOrientedBox"`, while the other debug shapes use `Dbg*` names (`DbgBox`, `DbgSphere`, ...)
  and its constructor function is `DbgOriBox`. Renaming changes the script API.

## Features

- [ ] `tools/egfca` — generate `.skin` files (additional model skins) from the compile script, so
  they do not have to be written by hand.

## Build scripts

- [ ] `shared/premake5.lua:189` — `sysLib` lists `gamesys/**`, which does not exist in `shared/`.
- [ ] `samples/GPUDriven/premake5.lua:26` — uses `egfLib`, which is defined only when
  `ENABLE_TOOLS` is on.
- [ ] `utils/.premake_modules/usage` — the custom `usage` / `uses` module conflicts with the
  built-in usages of newer premake5 releases (they work differently), so the build is pinned to
  the premake5 binaries in `utils/`. Resolve the conflict (rename the module API or migrate to
  the built-in usages) to allow updating premake5.

## Documentation to verify

- [ ] `docs/keyvalues.md` — V3 array elements separated by whitespace (`[bottom hcenter]`) and the
  one-line `embed1 { embeddedFlags flag1 flag3; }` form in V2 were not verified against the parser.
- [ ] `docs/public_filesystem.md` — whether `CFileSystemFind::GetPath()` returns a full path or only
  the entry name.
- [ ] `docs/framework_math.md` — `QuadTree.h` description (Barnes-Hut style) is inferred from method
  names.
