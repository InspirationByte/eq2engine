"""The eqrpc names blink uses.

In eq2engine/tools/python eqrpc is a top-level package next to blink; in a Blender addon both are
copied into the addon package (`<addon>.eqrpc`, `<addon>.blink`), where the relative import finds it.
"""

try:
	from ..eqrpc import Endpoint, Protocol, PayloadReader, ERROR_DISCONNECTED
except ImportError:		# blink is a top-level package
	from eqrpc import Endpoint, Protocol, PayloadReader, ERROR_DISCONNECTED

__all__ = ["Endpoint", "Protocol", "PayloadReader", "ERROR_DISCONNECTED"]
