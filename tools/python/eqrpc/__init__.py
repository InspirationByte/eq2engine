"""eqrpc: the engine RPC protocol in Python (standard library only).

Protocols with methods, calls with future replies, notifies and binary payload streams over the TCP
frames of the engine `CNetTransportTCP`; messages are binary KeyValues (`kv.Section`). See
eq2engine/docs/network_rpc_overview.md.

    endpoint = eqrpc.Endpoint(eqrpc.Endpoint.CLIENT)
    defs = eqrpc.Protocol("defs")
    endpoint.add_protocol(defs)
    endpoint.connect("127.0.0.1:27100")

    @defs.method("Changed")
    def on_changed(ctx, params, payload): ...

    defs.remote_call("SetMesh", {"def": 12}, payload=data).then(lambda response: ...)
    endpoint.update()   # from the main thread (in Blender: bpy.app.timers)
"""

from .kv import Section, KVError
from .payload import PayloadReader, PayloadWriter
from .transport import TcpTransport
from .endpoint import (
	Endpoint, Protocol, CallContext, DeferredReply, Future, Response, RPCError,
	ERROR_UNKNOWN_METHOD, ERROR_DISCONNECTED, ERROR_CANCELLED, ERROR_PAYLOAD_TOO_LARGE,
	ERROR_NOT_CONNECTED, ERROR_SEND_FAILED, ERROR_METHOD_FAILED,
)

__all__ = [
	"Section", "KVError", "PayloadReader", "PayloadWriter", "TcpTransport",
	"Endpoint", "Protocol", "CallContext", "DeferredReply", "Future", "Response", "RPCError",
	"ERROR_UNKNOWN_METHOD", "ERROR_DISCONNECTED", "ERROR_CANCELLED", "ERROR_PAYLOAD_TOO_LARGE",
	"ERROR_NOT_CONNECTED", "ERROR_SEND_FAILED", "ERROR_METHOD_FAILED",
]
