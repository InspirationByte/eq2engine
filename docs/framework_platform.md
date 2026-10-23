# framework/core/platform — message boxes, timers

Platform abstraction headers in `framework/core/platform/` (`coreLib`). All headers described here
are included by `core/core_common.h`. Use them instead of `std::chrono` and OS message box / timer
APIs in engine code.

Threads, locks, signals, thread-local storage and atomics are described in
[framework_multithreading.md](framework_multithreading.md).

## Message boxes — `messagebox.h`

```cpp
ErrorMsg("Failed to load '%s'", fileName);
WarningMsg("Shader cache is outdated");
InfoMsg("Saved to %s", path);
CrashMsg("Unrecoverable error: %s", reason);
```

- `InfoMsg`, `WarningMsg`, `ErrorMsg`, `CrashMsg` — printf-style; show a system message box with
  the matching icon and title. They only show the message: they do not terminate the application,
  so return or shut down explicitly after a fatal error.
- When no GUI is available the message is printed to the log instead.
- `SetMessageBoxCallback(cb)` replaces how message boxes are shown (e.g. by the application host);
  returns the previous callback. Callback: `int cb(const char* message, const char* title, EMessageBoxType type)`.
- `SetAssertHandler(fn)` replaces the assertion dialog handler used by `ASSERT*` macros; returns
  the previous handler.
- `EMessageBoxType`: `MSGBOX_INFO`, `MSGBOX_WARNING`, `MSGBOX_ERROR`, `MSGBOX_CRASH`,
  `MSGBOX_YESNO`, `MSGBOX_ABORTRETRYINGORE`; results are `EMessageBoxButton` values.

Message boxes block the calling thread. For regular diagnostics use the logger (`Msg`,
`MsgWarning`, `MsgError`, `DevMsg`).

## Timers — `eqtimer.h`

`CEqTimer` — high-resolution timer started at construction.

- `GetTime(reset = false)` — seconds (`double`) since start or last reset.
- `GetTimeMS(reset = false)` — milliseconds (`uint64`).

```cpp
CEqTimer timer;
LoadResources();
Msg("Loaded in %.2f ms\n", timer.GetTime() * 1000.0);

// per-frame delta
const float deltaTime = static_cast<float>(m_frameTimer.GetTime(true));
```
