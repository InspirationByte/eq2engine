# EQ2 Engine code style

Reference file: `core/e2core/Localize.cpp` / `Localize.h` (with `public/core/ILocalize.h`).
The reference shows the general layout; where it disagrees with this document, this document wins.

## Scope

- These rules are mandatory for **new files**.
- Third-party sources under `src_dependency/` are not subject to this code style. Keep their
  original style when changing them.
- When editing existing files, match the style of the surrounding code. Do not reformat, rename or
  "fix" existing code (style or suspected bugs) unless the user explicitly asks for it.

## Files

- Every source file starts with the standard header block:

  ```cpp
  //////////////////////////////////////////////////////////////////////////////////
  // Copyright (C) Inspiration Byte
  // 2009-2025
  //////////////////////////////////////////////////////////////////////////////////
  // Description: <one line describing the file>
  //////////////////////////////////////////////////////////////////////////////////
  ```

- Headers use `#pragma once` (no `#ifndef` include guards).
- Every `.cpp` file includes `core/core_common.h`.
- Include order in `.cpp` files:
  1. system and third-party headers with angle brackets (`#include <...>`) — always before
     `core_common.h`. Headers from `src_dependency/` are always included with angle brackets:
     `#include <lua.hpp>`, not `#include "lua.hpp"`;
  2. `#include "core/core_common.h"`;
  3. engine headers by path from the source roots (`core/...`, `utils/...`, `ds/...`);
  4. the file's own header last.
- Public interfaces live under `public/` (`I`-prefixed abstract classes); implementations live next
  to their module (`C`-prefixed classes).
- In `.cpp` files, console variable and command declarations (`DECLARE_CVAR*`, `DECLARE_CMD*`,
  see `framework_core.md`) go at the top, after the includes and before function code: file-local
  helpers and other functions are placed below them.
- Logical sections inside a file are separated with a dashed comment line:

  ```cpp
  //-------------------------------------------------------------------------------------------------
  ```

## Formatting

- Indent with tabs.
- Allman braces: opening brace on its own line for functions, classes, `if`, `for`, `while`,
  `switch`, `case` blocks.
- Single-statement bodies may omit braces. If either branch of an `if`/`else` has braces, give
  both branches braces.
- One space after control keywords and none inside parentheses: `if (x)`, `for (...)`,
  `while (...)`, `switch (x)`, `Foo(a, b)` — not `if(x)` or `Foo( a, b )`.
- Constructor initializer lists put each member on its own line with a leading comma:

  ```cpp
  CLocToken::CLocToken(int fileIdx, const char* tok, const char* text)
  	: m_token(tok)
  	, m_fileIdx(fileIdx)
  ```

- In class declarations, align return types, names and members into columns with tabs.
  Trivial one-line accessors may be defined inline in the header.
- Lambdas passed as arguments open their brace on the same line: `[&](int line, ...) {`.
- No trailing whitespace; no more than one consecutive blank line.

## Naming

| Kind | Convention | Example |
|---|---|---|
| Interface | `I` + PascalCase | `ILocalize`, `ILocToken` |
| Implementation class | `C` + PascalCase | `CLocalize`, `CLocToken` |
| Engine container/value type | PascalCase, no prefix | `Array`, `Map`, `EqString` |
| Methods, free functions | PascalCase | `AddTokensFile`, `ReplaceStrFmt` |
| Data-structure methods (`framework/ds`) | camelCase | `numElem()`, `append()`, `atEnd()` |
| Member variables | `m_` + camelCase | `m_languageFilePrefixes` |
| Global variables / singletons | `g_` + camelCase | `g_localizer`, `g_fileSystem` |
| File/class static variables | `s_` + camelCase | `s_defaultValue` |
| Local variables, parameters | camelCase | `fileIdx`, `langFileIdx` |
| Enum types | `E` + PascalCase | `EMyState` |
| Macros, enum constants | UPPER_SNAKE_CASE | `CV_ARCHIVE`, `DEVMSG_LOCALE` |
| Template parameters | UPPER_SNAKE_CASE (preferred); single letter for simple cases | `typename STORAGE_TYPE`, `int SIZE`, `typename T` |
| Type aliases (`using`) | PascalCase | `ITexturePtr`, `ColorTargetList` |
| Callbacks | camelCase + `Cb` suffix | `languageChangeCb` |

- The only valid prefixes are the scope prefixes `m_`, `s_` and `g_`. Type-based Hungarian
  prefixes (`sz`, `psz`, `p`, `i`, `n`, `f`, `b`, `m_p`, `m_n`, ...) are obsolete and must not be
  used in new code: `const char* fileName`, not `const char* pszFileName`; `m_items`, not `m_pItems`.
- Do not use identifiers starting with an underscore followed by an uppercase letter (`_FindToken`)
  — they are reserved in C++. Use a descriptive private name (`FindTokenInternal`) instead.

## Language usage

- Use `nullptr`, never `NULL` or `0` for pointers (including default arguments).
- Implementations of virtual interface methods must be marked `override`.
- C-style casts (`(char*)ptr`, `(int)value`) are not allowed. Use `static_cast`,
  `reinterpret_cast` or `const_cast`. Never cast away `const` to modify data the caller passed as
  `const`.
- File-local helpers are `static` free functions in the `.cpp`.
- `auto` is fine for iterators and obvious types; spell out the type otherwise.
- Use `using` aliases (not `typedef`) to shorten and generalize types:
  - ref-counted pointer of a type next to its declaration, `Ptr` suffix:
    `using ITexturePtr = CRefPtr<ITexture>;`
  - long or repeated container types: `using ColorTargetList = FixedArray<ColorTargetDesc, MAX_RENDERTARGETS>;`
  - function and callback signatures: `using CommandFilterFn = bool (*)(const ConCommandBase* cmd, ArrayCRef<EqString> args);`
  - inside templates and classes for dependent or base types: `using Node = ListNode<T>;`,
    `using Data = FutureImpl::FutureData<T>;`
  Aliases are PascalCase; upper case is used only for template parameters, not for aliases.
  Place shared aliases in the header next to the aliased type; keep local
  ones in the narrowest scope (class or function).
- Use `const` for locals that are not modified after initialization.
- Initialize members in the class declaration with brace initializers: `int m_fileIdx{ -1 };`.

## Engine facilities

- Do not use standard library containers, strings, smart or weak pointers, or `std::function`
  (`std::vector`, `std::map`, `std::unordered_map`, `std::list`, `std::string`,
  `std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`, ...). Use the engine equivalents from
  `framework/ds` (see `framework_ds.md`):

  | Instead of | Use |
  |---|---|
  | `std::vector`, `std::array` | `Array`, `FixedArray`, `BufferedArray` |
  | `std::span` | `ArrayRef`, `ArrayCRef` |
  | `std::list` | `List`, `FixedList` |
  | `std::map`, `std::multimap`, `std::unordered_map` | `Map` |
  | `std::string`, `std::wstring` | `EqString`, `EqWString` |
  | `std::string_view` | `EqStringRef`, `EqWStringRef` |
  | `std::shared_ptr`, `std::unique_ptr` | `RefCountedObject` + `CRefPtr` |
  | `std::weak_ptr` | `WeakRefObject` + `CWeakPtr` |
  | `std::function` | `EqFunction` |
  | `std::promise`, `std::future` | `Promise`, `Future` |
  | `std::sort`, `std::find` | `arraySort`, `arrayFindIndex` |

  Language-support parts of `std` (`std::move`, `std::forward`, type traits,
  `std::initializer_list`) are fine.
- Pass the memory-tracking source location to containers and allocating constructors:
  `Array<int> m_items{ PP_SL };`. `PPSourceLine::Empty()` disables allocation tracking — do not
  use it as a substitute for `PP_SL` (see `framework_ds.md`).
- All allocations go through PPMem (`core/ppmem.h`) so they are tracked:
  - `PPNew T(...)` instead of `new T(...)` (`delete` stays as is);
  - `PPAlloc(size)` instead of `malloc`, `PPReAlloc(ptr, size)` instead of `realloc`,
    `PPFree(ptr)` instead of `free`;
  - aligned memory: `PPAlignedAlloc` / `PPAlignedFree`.
- Use `defer { ... };` for scope-exit cleanup instead of manual cleanup on every return path.
- Logging: `Msg`, `MsgWarning`, `MsgError`, and `DevMsg(DEVMSG_<CATEGORY>, ...)` for verbose
  developer output. Messages end with `\n`.
- Programming errors are reported with `ASSERT`, `ASSERT_MSG` and `ASSERT_FAIL`; recover gracefully
  after them when possible.
- Do not modify a container while iterating it unless its API supports it: use the iterator
  returned by `remove` (`it = map.remove(it);`) instead of incrementing an erased iterator.

## Comments

- Comments explain intent or non-obvious behavior (`// only delete tokens which has been added by
  the files`), not what the next line literally does.
- Use `//` comments in code. Keep comment density similar to the surrounding code.
