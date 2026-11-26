"""The Blender part of BLINK: the menu, the preferences and auto-connect of a `blink.Client`.

An addon makes one `Addon` and calls its `register` / `unregister`:

    ADDON = blink.blender.Addon(__package__, LINK, "leveleditor_blink", title="BLINK",
                                draw_status=..., draw_commands=...)

It adds:

- the addon preferences: Server host, Server port, Auto-connect, Reconnect interval (s);
- the operators `<ns>.connect` (from the menu it asks for the host and the port) and `<ns>.disconnect`;
- the top bar menu "<title>: <state>" after Help and the View3D sidebar panel <title>: the state,
  the server address, the message, Connect / Disconnect, then the application lines and commands;
- the timer that updates the client and redraws the menu and the panel when `ui_state` changes;
- auto-connect after register and after a file is opened (not in background Blender); a new file
  disconnects first (load_pre), the objects of the link are gone with the old one.
"""

import logging

import bpy

from ..client import (
	DEFAULT_PORT, STATE_DISCONNECTED, STATE_CONNECTING, STATE_WAITING, STATE_CONNECTED, STATE_FAILED,
)
from . import mesh

log = logging.getLogger("blink")

UPDATE_INTERVAL = 0.03

_STATE_ICONS = {
	STATE_DISCONNECTED: "UNLINKED",
	STATE_CONNECTING: "SORTTIME",
	STATE_WAITING: "SORTTIME",
	STATE_CONNECTED: "LINKED",
	STATE_FAILED: "ERROR",
}


def redraw_ui():
	"""The top bar menu and the View3D panel show the link state."""
	for window in bpy.context.window_manager.windows:
		for area in window.screen.areas:
			if area.type in ("TOPBAR", "VIEW_3D"):
				area.tag_redraw()


class Addon:
	"""The BLINK UI of one addon and its client.

	package: the addon `__package__` (the preferences id); ns: the lowercase operator prefix, unique
	per addon; draw_status(layout) adds the state lines of the application, draw_commands(layout)
	its commands (Sync, ...); props are the preferences of the application ({name: bpy property}),
	shown in the menu after the BLINK ones; classes are the bpy classes of the application,
	registered with it.
	"""

	def __init__(self, package, client, ns, title="BLINK", default_port=DEFAULT_PORT,
			draw_status=None, draw_commands=None, props=None, classes=()):
		self.package = package
		self.client = client
		self.ns = ns
		self.title = title
		self.default_port = default_port
		self.draw_status = draw_status
		self.draw_commands = draw_commands
		self.props = dict(props or {})
		self.app_classes = tuple(classes)
		self.classes = self._make_classes()

		# functions (not bound methods): the same callables are added and removed, persistent
		# sets an attribute on them
		addon = self

		def timer():
			return addon._update()

		def auto_connect_timer():
			addon.auto_connect()
			return None		# once

		def draw_topbar(menu, context):
			addon._topbar(menu, context)

		@bpy.app.handlers.persistent
		def load_pre(*args):
			addon.client.disconnect()

		@bpy.app.handlers.persistent
		def load_post(*args):
			addon.auto_connect()

		self._timer = timer
		self._auto_connect_timer = auto_connect_timer
		self._draw_topbar = draw_topbar
		self._load_pre = load_pre
		self._load_post = load_post

	# ---- client

	def preferences(self, context=None):
		return (context or bpy.context).preferences.addons[self.package].preferences

	def connect(self, prefs=None):
		prefs = prefs or self.preferences()
		self.client.connect("%s:%d" % (prefs.host, prefs.port), float(prefs.reconnect_interval))
		if not bpy.app.timers.is_registered(self._timer):
			bpy.app.timers.register(self._timer, first_interval=UPDATE_INTERVAL, persistent=True)
		redraw_ui()

	def disconnect(self):
		self.client.disconnect()
		redraw_ui()

	def auto_connect(self):
		"""Starts the link when Auto-connect is on; not in background Blender (scripts, render)."""
		if bpy.app.background or self.client.is_active:
			return
		prefs = self.preferences()
		if prefs.auto_connect:
			self.connect(prefs)

	def _update(self):
		if not self.client.is_active:
			return None		# unregisters the timer
		shown = self.client.ui_state()
		try:
			self.client.update()
		except Exception:
			log.exception("BLINK: update failed")
		if self.client.ui_state() != shown:
			redraw_ui()
		return UPDATE_INTERVAL

	# ---- UI

	def state_icon(self):
		return _STATE_ICONS.get(self.client.state, "QUESTION")

	def draw_status_lines(self, layout):
		client = self.client
		layout.label(text=client.status_text(), icon=self.state_icon())
		if client.is_active:
			layout.label(text="Server: %s" % client.address)
		if self.draw_status:
			self.draw_status(layout)
		if client.message:
			layout.label(text=client.message, icon="ERROR")

	def _topbar(self, menu, context):
		menu.layout.menu(self.menu_idname, text="%s: %s" % (self.title, self.client.status_text()), icon=self.state_icon())

	def _make_classes(self):
		addon = self
		ns_upper = self.ns.upper()
		self.connect_idname = self.ns + ".connect"
		self.disconnect_idname = self.ns + ".disconnect"
		self.menu_idname = ns_upper + "_MT_menu"

		def _set_interval(prefs, context):
			addon.client.reconnect_interval = float(prefs.reconnect_interval)

		def preferences_draw(prefs, context):
			for name in ("host", "port", "auto_connect", "reconnect_interval", *addon.props):
				prefs.layout.prop(prefs, name)

		preferences = type(ns_upper + "_Preferences", (bpy.types.AddonPreferences,), {
			"bl_idname": self.package,
			"__annotations__": {
				"host": bpy.props.StringProperty(
					name="Server host", default="127.0.0.1",
					description="The computer the engine application runs on"),
				"port": bpy.props.IntProperty(
					name="Server port", default=self.default_port, min=1, max=65535,
					description="The BLINK port the engine application listens on"),
				"auto_connect": bpy.props.BoolProperty(
					name="Auto-connect", default=True,
					description="Connect when Blender starts and after a file is opened; "
						"without the server the link waits and tries again",
					update=lambda prefs, context: addon.auto_connect()),
				"reconnect_interval": bpy.props.IntProperty(
					name="Reconnect interval (s)", default=5, min=1, max=600,
					description="Seconds between the connection attempts while the server is not running or after it closed the link",
					update=_set_interval),
				**self.props,
			},
			"draw": preferences_draw,
		})

		def connect_draw(op, context):
			prefs = addon.preferences(context)
			op.layout.prop(prefs, "host")
			op.layout.prop(prefs, "port")

		def connect_execute(op, context):
			addon.connect(addon.preferences(context))
			return {"FINISHED"}

		connect = type(ns_upper + "_OT_connect", (bpy.types.Operator,), {
			"bl_idname": self.connect_idname,
			"bl_label": "Connect",
			"bl_description": "Connects to the BLINK server of the engine application",
			# from the menu: asks for the host and the port first
			"invoke": lambda op, context, event: context.window_manager.invoke_props_dialog(
				op, title="Connect to %s" % addon.client.server_title, confirm_text="Connect"),
			"draw": connect_draw,
			"execute": connect_execute,
		})

		def disconnect_execute(op, context):
			addon.disconnect()
			return {"FINISHED"}

		disconnect = type(ns_upper + "_OT_disconnect", (bpy.types.Operator,), {
			"bl_idname": self.disconnect_idname,
			"bl_label": "Disconnect",
			"bl_description": "Closes the link; what it made stays in the scene",
			"execute": disconnect_execute,
		})

		def menu_draw(menu, context):
			layout = menu.layout
			addon.draw_status_lines(layout)
			layout.separator()
			if addon.client.is_active:
				layout.operator(addon.disconnect_idname, icon="UNLINKED")
			else:
				layout.operator_context = "INVOKE_DEFAULT"
				layout.operator(addon.connect_idname, text="Connect...", icon="LINKED")
			if addon.draw_commands:
				addon.draw_commands(layout)
			layout.separator()
			prefs = addon.preferences(context)
			for name in ("auto_connect", "reconnect_interval", *addon.props):
				layout.prop(prefs, name)

		menu = type(self.menu_idname, (bpy.types.Menu,), {
			"bl_idname": self.menu_idname,
			"bl_label": self.title,
			"draw": menu_draw,
		})

		def panel_draw(panel, context):
			layout = panel.layout
			addon.draw_status_lines(layout)
			if addon.client.is_active:
				layout.operator(addon.disconnect_idname)
			else:
				prefs = addon.preferences(context)
				layout.prop(prefs, "host")
				layout.prop(prefs, "port")
				layout.operator_context = "EXEC_DEFAULT"
				layout.operator(addon.connect_idname)
			if addon.draw_commands:
				addon.draw_commands(layout)

		panel = type(ns_upper + "_PT_panel", (bpy.types.Panel,), {
			"bl_idname": ns_upper + "_PT_panel",
			"bl_label": self.title,
			"bl_space_type": "VIEW_3D",
			"bl_region_type": "UI",
			"bl_category": self.title,
			"draw": panel_draw,
		})

		return (preferences, connect, disconnect, menu, panel)

	# ---- register

	def register(self):
		for cls in self.classes + self.app_classes:
			bpy.utils.register_class(cls)
		bpy.types.TOPBAR_MT_editor_menus.append(self._draw_topbar)
		bpy.app.handlers.load_pre.append(self._load_pre)
		bpy.app.handlers.load_post.append(self._load_post)
		# the preferences and the window manager are ready after register
		bpy.app.timers.register(self._auto_connect_timer, first_interval=0.5)

	def unregister(self):
		self.client.disconnect()
		bpy.types.TOPBAR_MT_editor_menus.remove(self._draw_topbar)
		for handlers, func in ((bpy.app.handlers.load_pre, self._load_pre), (bpy.app.handlers.load_post, self._load_post)):
			if func in handlers:
				handlers.remove(func)
		for timer in (self._timer, self._auto_connect_timer):
			if bpy.app.timers.is_registered(timer):
				bpy.app.timers.unregister(timer)
		for cls in reversed(self.classes + self.app_classes):
			bpy.utils.unregister_class(cls)


__all__ = ["Addon", "redraw_ui", "mesh"]
