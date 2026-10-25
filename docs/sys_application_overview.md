# System application overview — `shared/sys`

`sysLib` is the application framework for games and samples: the entry point, SDL window and event
loop, engine systems start-up and shutdown, the frame loop, app states, the developer console,
input routing, configs and screenshots. An application links `sysLib` (premake property
`gameapp` in the workspace, see [architecture.md](architecture.md)) and implements only its states.

| File | Contents |
|---|---|
| `sys_main.cpp` | Entry points (`WinMain`, `main`, Android), core and file system init, SDL message boxes |
| `sys_window.cpp/.h` | Window creation, SDL init, game loop (`sysHostInit`, `sysHostGameLoop`, `sysHostTerminate`) |
| `sys_host.cpp/.h` | `g_pHost` (`CGameHost`) — systems, frame, video modes, cursor, input events |
| `sys_state.cpp/.h` | `CAppStateBase`, `eqAppStateMng` — app states |
| `sys_in_console.cpp/.h` | `g_consoleInput` (`CEqConsoleInput`) — developer console, log, ImGui frame |
| `sys_in_joystick.cpp/.h` | `CEqGameControllerSDL` — game controllers |
| `cfgloader.cpp/.h` | `WriteCfgFile`, `writecfg` |
| `sys_version.cpp/.h` | `ENGINE_NAME`, `ENGINE_VERSION`, `COMPILE_*`, `GetEngineBuildNumber()` |
| `imgui/imgui_impl_sys.*` | ImGui platform backend for SDL |
| `scripting/` | Engine Lua state and bindings ([scripting_esl_overview.md](scripting_esl_overview.md)) |

`sysLib` also compiles `shared/input` ([input_overview.md](input_overview.md)) and uses
`renderUtilLib`, `equiLib`, `movieLib`, `scriptLib`, SDL2 and the ImGui backend.

## Start-up and shutdown

```
WinMain / main
 ├─ CEqConsoleInput::SpewInit()                log capture for the console
 ├─ g_eqCore->Init(appName, commandLine)       E2.CONFIG, core interfaces (public_core.md)
 └─ sysMain()
     ├─ g_fileSystem->Init()                   (public_filesystem.md)
     ├─ sysHostInit()
     │   ├─ SDL_Init(events, game controller, haptic)
     │   ├─ g_pHost->InitSystems()
     │   │   ├─ g_parallelJobs->Init()
     │   │   ├─ LoadModules()                  eqMatSystem module, LoadShaderLibrary("eqBaseShaders")
     │   │   ├─ eqAppStateMng::InitAppStates() ← application
     │   │   ├─ -user_cfg option, cfg/config_default.cfg
     │   │   ├─ window (vid_mode, vid_fullscreen, vid_screen), command line (+cmd value)
     │   │   ├─ g_matSystem->Init()            RHI on the SDL window
     │   │   ├─ g_fontCache, g_localizer (+GAME_VERSION, BUILD_NUMBER tokens),
     │   │   │  debugoverlay, equi::Manager, g_inputCommandBinder, g_consoleInput (ImGui)
     │   │   └─ REGISTER_INTERNAL_SHADERS()    material shaders compiled into the application
     │   └─ CEqGameControllerSDL::Init()
     ├─ sysHostGameLoop()
     └─ sysHostTerminate() → g_pHost->ShutdownSystems()
         ├─ eqAppStateMng::ChangeState(nullptr), g_parallelJobs->Wait()
         ├─ WriteCfgFile(user_cfg)             bindings + CV_ARCHIVE ConVars
         ├─ eqAppStateMng::ShutdownAppStates() ← application
         └─ systems shutdown, eqMatSystem unloaded, window destroyed
```

`InitAppStates` runs **before** the window and the material system exist: register states,
shader overrides and debug menus there, and create GPU resources in a state's `OnEnter` (the first
state is entered on the first frame).

On Android, `main` also initializes JNI, extracts `E2.CONFIG` from the APK and mounts the OBB file
system before `sysMain`.

## Application contract — `eqAppStateMng`

The application provides these functions (see `samples/GPUDriven/states.cpp`,
`samples/Demo_RockPaperScissors/states.cpp`):

```cpp
namespace eqAppStateMng
{
const char* GetAppNameTitle() { return "My Game"; }

CAppStateBase* GetAppStateByType(int stateType) { return s_appStates[stateType]; }

bool InitAppStates()
{
	s_appStates[APP_STATE_GAME] = g_State_Game;
	g_audioSystem->Init();
	// shader overrides, ImGui debug menus, ...
	SetCurrentStateType(APP_STATE_GAME);
	return true;
}

void ShutdownAppStates() { ... }

bool IsPauseAllowed() { return true; }	// pause when the window loses focus
void SignalPause() {}					// called every frame while paused
}
```

`GetCurrentStateType()` returns `APP_STATE_NONE` (`0`) when there is no state, so a state of type
`0` cannot be told apart from "no state" by type.

## App states — `CAppStateBase`

A state is one mode of the application (menu, game, editor). Derive from `CAppStateBase`:

| Method | Called |
|---|---|
| `GetType()` | State type id (required) |
| `OnEnter(from)` / `OnLeave(to)` | When the state becomes current / stops being current |
| `PreUpdate(dt)` | Before `Update` |
| `Update(dt)` | Every frame (required); return `false` to switch to the next state |
| `HandleKeyPress`, `HandleMouseClick`, `HandleMouseMove`, `HandleMouseWheel`, `HandleJoyAxis` | Input not consumed by console / ImGui / EqUI |
| `GetMouseCursorProperties(visible, centered)` | Cursor visibility and relative (centered) mouse mode |
| `GetTimescale()` | Time scale for this state's `dt` |

State management (`eqAppStateMng`):

- `SetCurrentState(state)` / `SetCurrentStateType(type)` — switch at the start of the next frame
  (`OnLeave` / `OnEnter` are called from `UpdateStates`).
- `ChangeState(state)` / `ChangeStateType(type)` — switch immediately.
- `ScheduleNextState(state)` / `ScheduleNextStateType(type)` — set the state to switch to when the
  current state's `Update` returns `false` (`CAppStateBase::SetNextState(state, force)` does the
  same from inside a state).
- `GetCurrentState()` (returns the pending state if a switch is scheduled), `GetCurrentStateType()`,
  `GetNextState()`, `GetNextStateType()`.
- When there is no current state, the game loop exits.
- Events: `g_onBeginFrame`, `g_onEndFrame`, `g_onPreUpdateState`, `g_onPostUpdateState`,
  `g_onEnterState`, `g_onLeaveState` (`Event<>`, see [framework_ds.md](framework_ds.md)).

Lua scripts control states through `EqStateMgr` (`ChangeStateType`, `SetCurrentStateType`, ...).

## Game loop and frame

`sysHostGameLoop` polls SDL events and calls `g_pHost->Frame()` until quit (`exit` / `quit`
commands, window close, `HostQuitToDesktop()`). While the window is out of focus and
`IsPauseAllowed()` returns `true`, frames are skipped and `SignalPause()` is called. After every
frame the loop sleeps `sys_sleep` ms (default 1).

`CGameHost::Frame()`:

1. Measures time. With `sys_maxfps` set, the frame is skipped until enough time passed; the frame
   time is clamped to 0.001–0.1 s.
2. `g_onBeginFrame`, controller button repeats, cursor state.
3. `BeginScene`: `g_matSystem->BeginFrame`, `r_wireframe` / `r_overdraw`, console / ImGui frame
   start.
4. `eqAppStateMng::UpdateStates(dt × state timescale × sys_timescale)` — the current state
   renders and queues its command buffers.
5. EqUI render, full-screen edge AA, `g_matSystem->SubmitQueuedCommandsAwaitable()`.
6. Debug overlay with engine statistics (FPS, draw / dispatch counts, GPU and system memory),
   `r_showFPS`, `r_showFPSGraph`, input debug drawing.
7. `EndScene`: screenshot (without console / ImGui), console and ImGui drawing,
   `g_matSystem->EndFrame()`.
8. Pending fullscreen toggle, `g_onEndFrame`.

`GetFrameTime()` returns the current frame time; `GetWindowSize()`, `GetDefaultFont()`,
`GetWindowHandle()`.

## Window and video modes

- ConVars: `vid_mode` (`"1280x720"`), `vid_fullscreen`, `vid_screen`, `vid_bpp`,
  `r_antialiasing`.
- `SetFullscreenMode(screenSize)`, `SetWindowedMode()`, `ToggleFullscreen()` (also Right Alt +
  Enter), `ApplyVideoMode()`, `GetVideoModes(list)`, `IsWindowed()`; commands
  `sys_set_fullscreen`, `sys_set_windowed`, `sys_vmode_list`.
- `SetWindowTitle(title)`; non-retail builds append the renderer, configuration, platform and build
  number.
- Text input for on-screen keyboards: `RequestTextInput()`, `ReleaseTextInput()`,
  `IsTextInputShown()`.
- Cursor: controlled by the current state (`GetMouseCursorProperties`); it is always shown and
  never centered while the console or EqUI windows are open. `SetCursorPosition(x, y)`.

## Input routing

SDL events are translated to engine key codes (see [input_overview.md](input_overview.md)) and
dispatched by `CGameHost`:

| Event | Order |
|---|---|
| Keyboard | console → EqUI → state `HandleKeyPress` |
| Mouse buttons | console / ImGui → EqUI → (touch emulation with `m_clicks_to_touch`) → state `HandleMouseClick` |
| Mouse movement | console / ImGui position → EqUI → `g_inputCommandBinder->OnVectorEvent(MOU_MOVE)` → state `HandleMouseMove` |
| Mouse wheel | console → binder (`MOU_MOVE`) → state `HandleMouseWheel` |
| Controller buttons / axes | binder → state `HandleKeyPress` / `HandleJoyAxis` |
| Touch | binder touch zones |
| Text input | console |

Keyboard keys and mouse buttons reach command bindings only if the state forwards them to
`g_inputCommandBinder->OnPressEvent` (see [input_overview.md](input_overview.md)). Input is ignored
for 0.25 s after the window regains focus.

## Developer console — `g_consoleInput`

- Opened by the `+con_toggle` command (bind it to a key, e.g. `bind GRAVE +con_toggle` in
  `cfg/config_default.cfg`), or `con_show`, `con_show_full`, `con_hide`; `clear`, `help`.
- Executes console commands (see [framework_core.md](framework_core.md)) with autocompletion,
  variants, history and a log view. With the Lua state initialized, input that fails as a command
  is executed as Lua (`SetAlternateHandler`, see [scripting_esl_overview.md](scripting_esl_overview.md)).
- ConVars: `con_size`, `con_suggest`, `con_minicon`, `con_minicon_time`.
- The console owns the ImGui frame (`g_imGuiHost`, `shared/imgui_backend`): ImGui windows registered
  with `g_imGuiHost->AddDebugHandler` are always drawn, and the debug menu bar with
  `AddDebugMenu(path, handler)` entries is shown while the console is open.
- `CEqConsoleInput::SpewInit` captures engine log output for the console log from the start.

## Configuration files

| File | When |
|---|---|
| `cfg/config_default.cfg` | Executed during start-up (after `InitAppStates`) — default bindings and settings |
| User config (`user_cfg` ConVar, default `cfg/user.cfg`; `-user_cfg <file>` on the command line) | Written on shutdown by `WriteCfgFile`: `unbindall`, all non-custom bindings and `seti <cvar> <value>` for every `CV_ARCHIVE` ConVar. `exec_user_cfg` executes it |
| Command line | `+command value` pairs executed after the configs (`g_cmdLine->ExecuteCommandLine`) |

`writecfg [file]` writes the config manually.

## Other features

- `screenshot [name]` — saves the next frame as JPEG (`screenshots/screenshot_NNNN.jpg` or
  `<name>.jpg`, quality `screenshot_JpegQuality`).
- `sys_timescale` (cheat), `sys_maxfps`, `sys_sleep`.
- `m_sensitivity`, `m_invert`, `m_clicks_to_touch` — mouse settings.
- `__cheats` (protected, invisible) — enables `CV_CHEAT` variables; on by default in non-retail
  builds.
- Engine message boxes (`MsgBox*`, asserts) use SDL message boxes.
- Lua: `host` table (frame time, video modes, text input, window size), `buildInfo`, `EqStateMgr`
  (see [scripting_esl_overview.md](scripting_esl_overview.md)).
