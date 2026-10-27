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
| `sys_viewport.cpp/.h` | `CHostViewport` — state rendering inside an ImGui window |
| `sys_in_console.cpp/.h` | `g_consoleInput` (`CEqConsoleInput`) — developer console, log |
| `sys_in_joystick.cpp/.h` | `CEqGameControllerSDL` — game controllers |
| `cfgloader.cpp/.h` | `WriteCfgFile`, `writecfg` |
| `sys_version.cpp/.h` | `ENGINE_NAME`, `ENGINE_VERSION`, `COMPILE_*`, `GetEngineBuildNumber()` |
| `scripting/` | Engine Lua state and bindings ([scripting_esl_overview.md](scripting_esl_overview.md)) |

`sysLib` also compiles `shared/input` ([input_overview.md](input_overview.md)) and uses
`renderUtilLib`, `equiLib`, `movieLib`, `scriptLib`, SDL2 and `imguiBackendLib` (`g_imGuiHost`,
`shared/imgui_backend`, which also contains the ImGui platform backend for SDL).

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
     │   │   │  debugoverlay, g_imGuiHost (ImGui on the SDL window), equi::Manager,
     │   │   │  g_inputCommandBinder, g_consoleInput
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
| `GetViewport()` | `CHostViewport` the state renders to, `nullptr` for the whole window (see [Host viewport](#host-viewport--chostviewport)) |

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
3. `BeginScene`: `g_matSystem->BeginFrame`, console frame start, ImGui frame start
   (`g_imGuiHost->BeginFrame`, debug menus while the console is open), `r_wireframe` /
   `r_overdraw`.
4. `eqAppStateMng::UpdateStates(dt × state timescale × sys_timescale)` — the current state
   renders and queues its command buffers.
5. EqUI render, full-screen edge AA, `g_matSystem->SubmitQueuedCommandsAwaitable()`.
6. Debug overlay with engine statistics (FPS, draw / dispatch counts, GPU and system memory),
   `r_showFPS`, `r_showFPSGraph`. Screenshot (without console / ImGui), then one render pass with
   the debug overlay, input debug drawing, console (`g_consoleInput->EndFrame`) and ImGui
   (`g_imGuiHost->EndFrame`). With a visible state viewport, EqUI, debug overlays, input debug
   drawing and the console go to the viewport target, ImGui is drawn in a separate backbuffer pass
   and the screenshot is taken after it (see [Host viewport](#host-viewport--chostviewport)).
7. `EndScene`: `g_matSystem->EndFrame()`.
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
| Keyboard | console toggle key → ImGui → console → EqUI → state `HandleKeyPress` |
| Mouse buttons | ImGui → console → EqUI → (touch emulation with `m_clicks_to_touch`) → state `HandleMouseClick` |
| Mouse movement | console / ImGui position → EqUI → `g_inputCommandBinder->OnVectorEvent(MOU_MOVE)` → state `HandleMouseMove` |
| Mouse wheel | ImGui → console → binder (`MOU_MOVE`) → state `HandleMouseWheel` |
| Controller buttons / axes | binder → state `HandleKeyPress` / `HandleJoyAxis` |
| Touch | binder touch zones |
| Text input | ImGui → console |

With a visible state viewport (`GetViewport()`), the console, EqUI and the state receive mouse
buttons, movement and wheel only over the viewport, in viewport coordinates. Keys and text go past
the ImGui check while the viewport window is focused or the console is open, unless an ImGui text
field is active. See [Host viewport](#host-viewport--chostviewport).

Keyboard keys and mouse buttons reach command bindings only if the state forwards them to
`g_inputCommandBinder->OnPressEvent` (see [input_overview.md](input_overview.md)). Input is ignored
for 0.25 s after the window regains focus.

ImGui (`g_imGuiHost->Input*`) receives keys, mouse buttons, wheel and text first. While an ImGui
item is hovered, active or focused, or ImGui wants the mouse / keyboard
(`g_imGuiHost->IsImGuiItemsInFocus()`), the event goes no further; key releases still reach the
console so its modifier state does not stick.

## Host viewport — `CHostViewport`

A state can render into a render target shown in an ImGui window instead of the whole window
(tools with panels around the scene). The state owns a `CHostViewport` and returns it from
`CAppStateBase::GetViewport()`.

```cpp
CHostViewport m_viewport{ "egfman2_scene" };

CHostViewport* GetViewport() override { return &m_viewport; }

// ImGui handler (runs in BeginScene, before Update)
if (ImGui::Begin("Scene"))
	m_viewport.DrawImGui();	// fills the available content region
ImGui::End();

// Update
const IVector2D viewSize = m_viewport.GetSize();
projMat = perspectiveMatrixY(fov, viewSize.x, viewSize.y, zNear, zFar);
g_renderAPI->BeginRenderPass(
	Builder<RenderPassDesc>()
	.ColorTarget(m_viewport.GetColorTarget(), true, clearColor)
	.DepthStencilTarget(m_viewport.GetDepthTarget())
	.DepthClear()
	.End());
```

- `DrawImGui(size)` resizes the color and depth targets (backbuffer formats, `ResizeRenderTarget`)
  to the drawn size and puts an invisible button with the image in the current ImGui window. The
  viewport is *visible* only in a frame where `DrawImGui` was called.
- Viewports are disabled in `Retail` and `Profile` builds (`HOST_VIEWPORT_ENABLED` in
  `sys_viewport.h`): `DrawImGui` does nothing and the viewport is never visible.
- When the viewport is not visible (ImGui window closed, `Retail` or `Profile` build),
  `GetColorTarget`, `GetDepthTarget` and `GetSize` return the backbuffer, the default depth buffer
  and the window size, and the host routes input as without a viewport.
- Input: mouse coordinates passed to the state handlers are relative to the viewport. A button
  pressed over the viewport captures the mouse until all captured buttons are released, so drags
  continue outside the viewport. Mouse movement and wheel reach the state while the mouse is over
  the viewport or captured. Key releases always reach the state, so keys do not stick when focus
  moves away.
- `CGameHost::SetCursorPosition` takes viewport coordinates, and a centered cursor
  (`GetMouseCursorProperties`) is warped to the viewport center.
- `GetScreenRect`, `ToViewport`, `ToWindow` convert between window and viewport coordinates.
- The state viewport is the main viewport: the host draws EqUI (`equi::Manager->Render(target)`,
  view frame of the viewport size), debug overlays (`debugoverlay->Draw` with the viewport targets),
  `r_showFPS`, input debug drawing and the console only into it. ImGui stays in the window
  backbuffer. Full-screen edge AA is skipped: its luma pass needs a storage source texture.
- `screenshot` with a visible viewport saves the whole window with ImGui, since the viewport gets
  to the backbuffer only through ImGui.

## Developer console — `g_consoleInput`

- Opened by the key bound to `toggleconsole` (`bind GRAVE toggleconsole` in
  `cfg/config_default.cfg`). `CGameHost::Key_Event` checks this key before any other input
  handling, so it works while ImGui has focus; with the console open, Shift + the key toggles the
  log view. Also `con_toggle` / `+con_toggle`, `con_show`, `con_show_full`, `con_hide`; `clear`,
  `help`.
- Executes console commands (see [framework_core.md](framework_core.md)) with autocompletion,
  variants, history and a log view. With the Lua state initialized, input that fails as a command
  is executed as Lua (`SetAlternateHandler`, see [scripting_esl_overview.md](scripting_esl_overview.md)).
- ConVars: `con_size`, `con_suggest`, `con_minicon`, `con_minicon_time`.
- ImGui is run by the host (`g_imGuiHost`, `shared/imgui_backend`), not the console: ImGui
  windows registered with `g_imGuiHost->AddDebugHandler` are always drawn, and the debug menu bar
  with `AddDebugMenu(path, handler)` entries is shown while the console is open.
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
