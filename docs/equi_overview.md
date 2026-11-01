# EqUI overview — `shared/equi`

EqUI is the engine's retained-mode game UI: menus, HUD, loading screens. Layouts are trees of
controls described in KeyValues `.res` files and changed at runtime from C++ or Lua. It is built
as `equiLib` and drawn with the material system and the font system. Debug and tool UI uses ImGui
instead (`g_imGuiHost`, see [sys_application_overview.md](sys_application_overview.md)).

| File | Contents |
|---|---|
| `EqUI_Manager.h/.cpp` | `equi::Manager` (`CUIManager`) — root panel, panels, control factories, focus, rendering, input |
| `IEqUI_Control.h/.cpp` | `equi::IUIControl` — base control: KeyValues parsing, layout, fonts, children, events, rendering |
| `equi_defs.h` | Anchors (`UI_BORDER_*`), alignment (`UI_ALIGN_*`), scaling modes (`UI_SCALING_*`), event flags (`UI_EVENT_*`) |
| `EqUI_Panel.h/.cpp` | `Panel` (top-level panel or window), `Container`, `HudElement` |
| `EqUI_Label.h/.cpp` | `Label` — text |
| `EqUI_Button.h/.cpp` | `Button` — clickable button |
| `EqUI_Image.h/.cpp` | `Image` — material or atlas image |
| `EqUI_Mask.h/.cpp` | `Mask` — draws its children through a mask texture |
| `EqUI_ProgressBar.h/.cpp` | `ProgressBar` |
| `shared/sys/scripting/sys_esl_equi.cpp` | Lua bindings (`equi`, `equi_cast`, `fonts`) |

## Manager and panels

`CGameHost` initializes the manager after the font cache (`equi::Manager->Init()`), renders it
every frame and passes mouse and keyboard events to it (see
[sys_application_overview.md](sys_application_overview.md)).

- **Root panel:** `GetRootPanel()` is an invisible panel (`equi_root`) that covers the view frame.
  Every top-level panel is its child.
- **Panels:** `AddPanel(panel)` adds a top-level panel to the root. Panel names must be unique
  (`FindPanel(name)` is case-insensitive). `DestroyPanel(panel)` removes and deletes it.
  `BringToTop(panel)` / `GetTopPanel()` change and return the drawing order.
- **View frame:** the host sets it to the window or the state viewport size each frame
  (`SetViewFrame`). `GetScreenSize()` returns its size.
- **Focus:** `SetFocus(control)` / `GetFocus()` — the keyboard focus. Clicking a control focuses
  it; clicking the root panel clears the focus. `GetMouseOver()` is the control under the cursor.
- **Default font:** `GetDefaultFont()` — `"default"` at size 22, used when no control in the
  parent chain has a font.

```cpp
equi::Panel* panel = equi::Manager->Create<equi::Panel>();	// factory by class name
panel->SetName("PauseMenu");
equi::Manager->AddPanel(panel);

KVSection uiKvs;
if (KV_LoadFromFile("resources/ui_pausemenu.res", SP_MOD, uiKvs))
	panel->InitFromKeyValues(uiKvs);

// later
equi::Manager->DestroyPanel(panel);
```

### Control factories

Controls are created by class name (case-insensitive) through registered factories:

| Name | Class | Purpose |
|---|---|---|
| `panel` | `Panel` | Top-level panel or window, draws a filled rectangle (`color`) |
| `container` | `Container` | Group without drawing; can include children from a file |
| `HudElement` | `HudElement` | Group without drawing, used in HUD layouts |
| `label` | `Label` | Text |
| `button` | `Button` | Button with a label, raises `click` |
| `image` | `Image` | Material or atlas image |
| `mask` | `Mask` | Masked children |
| `progressBar` | `ProgressBar` | Horizontal fill bar |

`CreateElement(name)` returns a new control; `Create<T>()` creates by `T::Classname()` and casts.
To add a control type, derive from `IUIControl` (or another control), add `EQUI_CLASS(Class, Base)`
in the class body, implement `DrawSelf`, define the factory with
`DECLARE_EQUI_CONTROL(name, Class)` in its `.cpp` and register it with
`EQUI_REGISTER_CONTROL(name)` after `equi::Manager->Init()`. On non-MSVC compilers, declare the
factory with `EQUI_CONTROL_DECLARATION(name)` and register with `EQUI_REGISTER_CONTROL2(name)`.

Type checks use class names: `control->As<equi::Image>()` / `equi::DynamicCast<T>(control)`
return `nullptr` when the class name differs (an exact match, not inheritance).

## Layout files

A layout file has one root section (`panel` for panels), with nested `child <class> "<name>"`
sections:

```
panel
{
	selfVisible	0;
	position 	0 0;
	size 		640 480;		// layout units: children are scaled from this size

	child file "resources/ui_partial/menu_fonts.res";	// include children and fonts

	child image "MenuTitle"
	{
		path		"ui/ui_menu_title_gradient";
		position	-50 100;
		size		300 30;
		scaling		uniform;
		color		1.0 0.57 0.0 1;

		child label "text"
		{
			align		left vcenter;
			position	100 3;
			size		200 20;
			scaling		uniform;
			label		"#MENU_TITLE";		// '#' = localization token
		}
	}

	child button "Resume"
	{
		label		"Resume";
		position	10 64;
		size		210 40;
		command		engine "echo Resume clicked";
	}
}
```

- `InitFromKeyValues(sec, keepElements)` parses the control and creates its children. With
  `keepElements = false` existing children are deleted first. With `true`, a child with the same
  name is updated in place (or replaced if its class differs), which is how HUD layouts are
  layered over a base layout.
- `Panel::InitFromKeyValues` uses the nested `panel` section if there is one, otherwise the given
  section.
- `child file "<path>"` loads another file (path as is, `SP_MOD`) and adds its fonts and children
  to this control. `Container` also accepts `file "<name>"`, loaded from `resources/<name>`.

### Common properties — `IUIControl::Parse`

| Key | Meaning |
|---|---|
| `position x y`, `size w h` | Rectangle in layout units, relative to the parent and its alignment |
| `visible 0/1` | Visibility (`Show`, `Hide`); hidden controls are not drawn and not hit-tested |
| `selfVisible 0/1` | Draw the control itself; children are still drawn |
| `label "text"` | UTF-8 text, or `#TOKEN` for a localized string |
| `align ...` | Alignment inside the parent: `left`, `right`, `hcenter`, `top`, `bottom`, `vcenter`, `center` |
| `anchors ...` | Sides kept at the same distance when the parent is resized: `left`, `top`, `right`, `bottom`, `all` |
| `scaling mode` | How the parent's resize scales the control: `none`, `inherit`, `inherit_min`, `inherit_max`, `aspectw`, `aspecth` / `uniform`, `aspect_min`, `aspect_max` |
| `transform { rotate deg; translate x y; elementScale x y; }` | Extra transform around the control center (does not affect hit testing) |
| `clipChilds 0/1`, `clipTransform 0/1` | Clip children to this control's rectangle; apply the transform to the clip rectangle. Default is inherited from the parent |
| `command <args>` | Handler for the `command` event, see [Events](#events) |
| `font "Name" [size] [bold] [italic]` | Font of the control. A single name refers to an entry of `fonts` in this control or a parent |
| `fonts { Name "Font" size; ... }` | Named fonts for children (text properties can be set per entry) |
| `fontScale x y`, `textColor r g b a`, `textAlign ...`, `textCase upper/lower`, `textWeight`, `textMonospace`, `textShadowColor`, `textShadowOffset`, `textShadowWeight` | Text style |

Control-specific keys:

| Control | Keys |
|---|---|
| `panel` | `color`, `window 1` (window with title and close button from `resources/WindowControls.res`, hidden by default), `screenoverlay 1` (always covers the view frame) |
| `image` | `path "material"` with optional `uvLeftTop` / `uvRightBottom`, or `atlas "material" "entry"`; `color`, `flipX`, `flipY` |
| `mask` | `image "mask texture"`, `color` |
| `progressBar` | `value` (0..1), `color` |

### Size and scaling

Positions and sizes are in layout units. When the manager resizes the root panel to the view
frame, the difference between the current and the parsed size (`GetSizeDiff()`,
`GetSizeDiffPerc()`) is used by children:

- **Scaling** multiplies position, size and font scale by the parent's size ratio
  (`CalcScaling()`). `uniform` keeps the aspect ratio by using the vertical ratio, so a layout
  made for 640×480 keeps its proportions on wide screens.
- **Anchors** move or stretch the control by the parent's size difference.
- **Alignment** places the control relative to the parent's client rectangle; with `right` /
  `bottom`, `position` is an offset from that side.

`GetClientRectangle()` returns the final rectangle in screen pixels.

## Controls at runtime

```cpp
equi::IUIControl* title = panel->Get("MenuTitle.text");	// path of child names
title->SetLabel("#MENU_PAUSE");
title->SetTextColor(ColorRGBA(1, 1, 0, 1));

if (equi::ProgressBar* bar = panel->FindChildRecursive("Health")->As<equi::ProgressBar>())
	bar->SetValue(0.75f);
```

- Children: `AddChild`, `RemoveChild(ctrl, destroy)`, `FindChild(name)` (direct children),
  `FindChildRecursive(name)`, `Get("a.b.c")`, `ForEachChild(func)`, `ClearChilds(destroy)`,
  `ClearAll` (children, fonts and event handlers). A control owns its children and deletes them.
- Children added later are drawn on top and tested first for mouse hits.
- Visibility and state: `Show`, `Hide`, `SetVisible`, `SetSelfVisible`, `Enable`. `IsVisible()`
  and `IsEnabled()` include the parents.
- Geometry: `SetPosition`, `SetSize`, `SetRectangle`, `SetAnchors`, `SetAlignment`, `SetScaling`,
  `SetTransform(translate, scale, rotate)`.
- Text: `SetLabel` (UTF-8 or `#TOKEN`), `SetLabelText` (wide string), `SetFontScale`,
  `SetTextColor`, `SetTextAlignment`. Labels support font tags (`TEXT_STYLE_USE_TAGS`).
- `Image::SetMaterial`, `SetAtlasImage`, `SetColor`, `SetUVRegion` (in texture pixels);
  `ProgressBar::SetValue`, `SetColor`; `Panel::SetColor`, `CenterOnScreen`.

### Events

Event handlers are deprecated: they are not used by the game and may be removed or reworked. Do
not build new UI logic on them. Event handlers are named callbacks stored in the control:

- `AddEventHandler(name, callback)` returns an id (`StringId24(name)`); `RemoveEventHandler(id)`,
  `RemoveEventHandlers(name)`.
- `RaiseEvent(name, userData)` / `RaiseEventUid(id, userData)` call the **first** handler with
  that name and return its result (-1 if there is none).
- `Button` raises `click` when the left mouse button is released over it after being pressed on
  it.
- The `command` key in a layout adds a `command` handler with these actions:

  | Command | Action |
  |---|---|
  | `command engine "<console command>"` | Executes a console command |
  | `command showpanel "<panel>"` / `hidepanel "<panel>"` | Shows / hides a top-level panel |
  | `command hideparent` | Hides the parent control |

  Nothing raises `command` at the moment: `Button` raises only `click`, and `RaiseEvent` is not
  bound to Lua. A `command` key has an effect only if C++ code calls `RaiseEvent("command", ...)`
  on the control.

## Rendering

`Render(colorTarget)` draws the root panel in one render pass on the given target (the backbuffer
or the state viewport). Each control:

1. Pushes its transform (`transform` block, rotated and scaled around its center).
2. Sets the scissor rectangle (its client rectangle, clipped by parents with `clipChilds`).
3. Calls `DrawSelf(rect, recorder)` if `selfVisible` (the root panel never draws itself).
4. Renders children, earliest first.

`Mask` renders its children to a render target (`_ui_<name>_masked`) and draws it with the
`UIOverlayMask` shader and the mask texture. It must be `selfVisible`.

## Input

The host passes events in this order: ImGui → console → EqUI → app state (see
[sys_application_overview.md](sys_application_overview.md)).

- EqUI handles input only while a panel with `window 1` is visible (`IsWindowsVisible()`). The
  host also shows the cursor while such a panel is visible.
- Mouse: the manager hit-tests from the root panel, sends `UI_EVENT_MOUSE_IN` / `MOUSE_OUT` when
  the control under the cursor changes, brings the clicked panel to the top and passes the event
  to the control under the cursor (`ProcessMouseEvents`). A window panel can be dragged with the
  left button.
- Keyboard: events go to the focused control (`ProcessKeyboardEvents`).
- An event is consumed when the control returns `true` (the default for all controls).

## Lua

`shared/sys/scripting/sys_esl_equi.cpp` binds EqUI to the engine Lua state (see
[scripting_esl_overview.md](scripting_esl_overview.md)):

- `equi` — the manager: `GetRootPanel`, `CreateElement`, `AddPanel`, `DestroyPanel`, `FindPanel`,
  `BringToTop`, `GetTopPanel`, `GetViewFrame`, `GetScreenSize`, `SetFocus`, `GetFocus`,
  `GetMouseOver`, `IsWindowsVisible`, `GetDefaultFont`, plus `equi.FormatString(fmt, ...)` and
  `equi.IsValid(control)`.
- Controls: `IUIControl` methods and properties (`name`, `label`, `visible`, `position`, `size`,
  `anchors`, `alignment`, `scaling`, `rectangle`, ...), `Panel`, `Container`, `Image`,
  `ProgressBar`. `control:SetLabel(fmt, ...)` formats the localized `fmt` with the extra
  arguments.
- `equi_cast.panel(ctrl)`, `equi_cast.image(ctrl)`, `equi_cast.progressBar(ctrl)`,
  `equi_cast.container(ctrl)` — typed casts, `nil` if the class differs.
- `fonts.LoadFontDescriptionFile(file)`; `UI_BORDER_*`, `UI_ALIGN_*`, `UI_SCALING_*`,
  `TEXT_STYLE_*`, `TEXT_ALIGN_*` constants.

```lua
local kvs = KeyValues.new()
if kvs:LoadFile("resources/ui_filmdirector.res", SP_MOD) then
	layout:ClearAll(true)
	layout:InitFromKeyValues(kvs:GetRoot(), false)
end

local bar = equi_cast.progressBar(layout:FindChildRecursive("Health"))
if bar then
	bar.value = 0.5
end
```

## Console commands and debugging

| Command / ConVar | Purpose |
|---|---|
| `ui_listpanels` | List top-level panels and their visibility |
| `ui_showpanel <name>`, `ui_hidepanel <name>` | Show / hide a panel |
| `equi_debug <depth>` | Draw control rectangles and names at the given tree depth (`-1` for all) |
| `equi_debugMaskClear 1` | Clear mask render targets to red |
