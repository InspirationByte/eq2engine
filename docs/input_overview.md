# Input overview — `shared/input`, `public/input`

Input is part of `sysLib`. Platform events (SDL) are turned into engine key codes by the host
(`shared/sys`, see [sys_application_overview.md](sys_application_overview.md)) and passed to
the input command binder, which maps keys, axes and touch zones to console commands or C++
callbacks.

| File | Contents |
|---|---|
| `public/input/in_keys_ident.h` | Key codes (`KEY_*`, `KEY_JOY_*`, `MOU_*`, joystick axes), key name table and conversion helpers |
| `shared/input/InputCommandBinder.h/.cpp` | `g_inputCommandBinder` (`CInputCommandBinder`) — bindings, vector actions, touch zones, console commands |
| `shared/sys/sys_in_joystick.*` | `CEqGameControllerSDL` — game controllers (SDL Game Controller API) |

## Key codes — `in_keys_ident.h`

With SDL (all current platforms) key codes are one integer range:

| Range | Codes |
|---|---|
| `0 .. SDL_NUM_SCANCODES-1` | Keyboard: `KEY_A`, `KEY_F1`, `KEY_ENTER`, ... (SDL scancodes, layout independent). Right Shift / Ctrl / Alt are reported as the left ones |
| `JOYSTICK_START_KEYS + n` | Controller buttons: `KEY_JOY_A`, `KEY_JOY_START`, `KEY_JOY_DPAD_UP`, ... (up to 32) |
| `JOYSTICK_START_AXES + n` | Controller axes: left / right stick X and Y, triggers (up to 32) |
| `MOU_MOVE`, `MOU_B1`..`MOU_B5`, `MOU_WHUP`, `MOU_WHDN` | Mouse movement, buttons, wheel |

Every bindable code has a name in `s_keyMapList`: `"W"`, `"SPACE"`, `"MOUSE1"`, `"MWHEELUP"`,
`"JOY_A"`, `"JOYAXIS_LX"`, ... Some entries also have a localization token for UI
(`#IN_MOU_B1`, `#IN_JOY_A`); for keyboard keys `Init()` fills the human-readable name from SDL.

```cpp
const int key = KeyStringToKeyCode("SPACE");		// -1 if unknown
const char* name = KeyCodeToString(KEY_SPACE);		// "SPACE", or "UNKNOWN"
const char* hrName = KeyCodeToHumanReadableString(KEY_JOY_A);	// "#IN_JOY_A"
```

## Command bindings

A binding maps a key (optionally with up to two modifier keys, `"CTRL+SHIFT+S"`) to a console
command with arguments. Bindings are created from config files and the console:

```
bind W +forward
bind MOUSE1 +attack
bind CTRL+S "save" quick
bind GRAVE +con_toggle
unbind W
unbindall
```

- **`+` / `-` commands:** if both `+name` and `-name` exist, the binding calls `+name` on press
  and `-name` on release. Otherwise `name` itself is called on press only.
- Arguments after the command are passed to it on every dispatch.
- When a key with modifiers matches, bindings with modifiers win over plain bindings of the same key
  (`CTRL+S` does not also trigger `S`). Releasing a modifier releases its bindings.
- Bindings may be created before the commands exist (configs are executed before all modules are
  loaded); they are resolved after `Init()` and again on first use. Unknown commands are reported
  once.
- `CGameHost::ShutdownSystems` writes all non-custom bindings back to the user config
  (`WriteBindings`: `unbindall` + `bind ...` lines, see
  [sys_application_overview.md](sys_application_overview.md)).

C++ API: `BindCommand(key, command, args)`, `UnbindKey(key)`, `UnbindCommand(name)`,
`UnbindAll()`, `UnbindAll_Joystick()`, `FindBinding(key)`, `FindBindingByCommand(cmd, args)`,
`FindBindingByCommandName(name, args)`, `GetBindingList()`.

Key names of a binding for UI: `UTIL_GetBindingKeyString`, `UTIL_GetBindingKeyStringLocalized`,
`UTIL_GetBindingKeyIds`.

## Named bindings — C++ callbacks

For code that does not need console commands, bind a callback directly. Named bindings are
"custom": they are not saved to the config.

```cpp
g_inputCommandBinder->AddNamedBinding("forward", "W", [](void* userData, const Vector3D& value) {
	CMyState* self = static_cast<CMyState*>(userData);
	bitsSet(self->m_buttons, CAM_FORWARD, value.x > 0);	// 1 on press, 0 on release
}, this);

g_inputCommandBinder->RemoveNamedBinding("forward");	// e.g. in OnLeave
```

The callback receives `value.x` = 1 on press and 0 on release for buttons, or the axis value for
axes and mouse movement.

## Vector actions — axes

Analog inputs (controller sticks and triggers, mouse movement) are bound to vector actions:

```cpp
g_inputCommandBinder->CreateVectorAction("steering", [](void* userData, const Vector3D& value) {
	static_cast<CCarControls*>(userData)->steer = value.x;	// -1..1 for sticks
}, 1, this);
```

```
bind JOYAXIS_LX ax_steering
bind MOUSE_MOVE ax_camera
```

- The action is registered with an `ax_` prefix; bindings refer to it as `ax_<name>`.
  `CreateVectorAction` with an existing name replaces its callback.
- Controller axes are normalized to -1..1 (`value.x`). Mouse movement sends (dx, dy) scaled by
  `m_sensitivity` and inverted Y unless `m_invert`; the mouse wheel is also sent as `MOU_MOVE`.
- Axis keys bound to a normal console command use the `+`/`-` rule with a threshold of 0.5.
- `RemoveVectorAction(name)`, `GetAxisActionList()`.

## Touch zones

On touch devices screen rectangles act as buttons. They are loaded from
`resources/in_touchzones.res` (KeyValues):

```
zones
{
	accelerate
	{
		bind		"+accel";
		position	0.85 0.8;		// center, normalized screen coordinates
		size		0.2 0.3;
	}
}
```

A finger pressed inside a zone activates its command; lifting that finger deactivates it.
`in_touchZones_reload` reloads the file, `in_touchZones_debug 1` draws the zones.
`m_clicks_to_touch 1` converts mouse clicks to touches for testing on desktop.

## Event flow

The host feeds the binder (`CGameHost` in `shared/sys/sys_host.cpp`):

| Event | Binder call | Notes |
|---|---|---|
| Keyboard keys, mouse buttons | — | Passed to the current app state's `HandleKeyPress` / `HandleMouseClick`; **the state forwards them** with `g_inputCommandBinder->OnPressEvent(key, down)` |
| Controller buttons | `OnPressEvent(JOYSTICK_START_KEYS + button, down)` | Also passed to `HandleKeyPress`. Held buttons repeat after `in_joy_repeatDelayInit`, then every `in_joy_repeatDelay` |
| Controller axes | `OnVectorEvent(JOYSTICK_START_AXES + axis, value)` | Only the controller selected by `in_joy_id` |
| Mouse movement, wheel | `OnVectorEvent(MOU_MOVE, delta)` | |
| Touch | `OnTouchEvent(finger, pos, down)` | |

The console, ImGui and EqUI receive keyboard and mouse events first and consume them while they
are active. A typical state forwards input like this:

```cpp
void CMyState::HandleKeyPress(int key, bool down)
{
	g_inputCommandBinder->OnPressEvent(key, down);
}

void CMyState::HandleMouseClick(int x, int y, int buttons, bool down)
{
	if (g_consoleInput->IsVisible())
		return;
	g_inputCommandBinder->OnPressEvent(buttons, down);
}
```

`GetLastInputDeviceUsed()` returns the device type (`INPUTDEV_KEYBOARD`, `INPUTDEV_MOUSE`,
`INPUTDEV_CONTROLLER`, `INPUTDEV_TOUCHPAD`) of the last released key — use it to show matching
button hints.

## Game controllers

`CEqGameControllerSDL` opens up to `MAX_CONTROLLERS` (4) controllers on connection events and
reports the one selected by `in_joy_id` (default 0). `GetControllers()`, `GetName()`,
`IsConnected()`. ConVars: `in_joy_id`, `in_joy_repeatDelayInit`, `in_joy_repeatDelay`,
`in_joy_rumble`, `in_joy_debug`.

## Console commands and debugging

| Command / ConVar | Purpose |
|---|---|
| `bind <key> <command> [args]`, `unbind <key>`, `unbindall`, `unbindjoystick` | Manage bindings |
| `in_listBindings`, `in_listAxisActions`, `in_listTouchZones` | List bindings, vector actions, touch zones |
| `in_keys_debug 1` | Log key presses and dispatched commands |
| `in_touchZones_debug 1` / `2` | Draw touch zones / also log touches |
| `in_joy_debug 1` | Log controller buttons and axes |

Lua: the `input` table (`AddBinding`, `CreateVectorAction`, `BindAction`, `GetLastInputDeviceUsed`,
key name helpers, ...) and `INPUTDEV_*` constants are bound in
`shared/sys/scripting/sys_esl_engine.cpp` (see [scripting_esl_overview.md](scripting_esl_overview.md)).
