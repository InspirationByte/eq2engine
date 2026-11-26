"""BLINK: a live link of an engine application with Blender over eqrpc.

The engine application runs `CBlinkServer` (shared/blink) on 127.0.0.1:<port>; Blender is the client.
The core has the connection (`client.Client`: blink.Hello, reconnect), the engine <-> Blender
coordinates (`coords`) and the mesh of `BlinkMesh` (`mesh`); the application adds its protocols.
`blink.blender` is the Blender part (bpy): the BLINK menu, the preferences, auto-connect. See
eq2engine/docs/blink_overview.md.

Only `client`, `coords` and `mesh` are imported here, so the package works without Blender.
"""

from . import coords
from .client import (
	Client, DEFAULT_PORT, DEFAULT_RECONNECT_INTERVAL,
	STATE_DISCONNECTED, STATE_CONNECTING, STATE_WAITING, STATE_CONNECTED, STATE_FAILED,
)
from .mesh import Mesh, read_mesh

__all__ = [
	"coords", "Client", "DEFAULT_PORT", "DEFAULT_RECONNECT_INTERVAL",
	"STATE_DISCONNECTED", "STATE_CONNECTING", "STATE_WAITING", "STATE_CONNECTED", "STATE_FAILED",
	"Mesh", "read_mesh",
]
