"""RPC endpoint compatible with the engine `RPCEndpoint` / `RPCProtocol`.

Messages of the calls lane are binary KeyValues roots (`id`, `protocol`, `method`, `params`,
`result`, `error`, `payload`), payloads go as chunks on the payload lane. The endpoint is used
from one thread: `update` receives, runs the methods, resolves the futures and sends the payload
chunks; the future callbacks run in `update` too. See eq2engine/docs/network_rpc_overview.md.
"""

import collections
import logging
import struct
import time

from . import kv
from . import transport as net

log = logging.getLogger("eqrpc")

# ERPCError; methods reply with their own positive codes
ERROR_UNKNOWN_METHOD = -1		# no such protocol or method on the peer
ERROR_DISCONNECTED = -2			# the connection closed before the reply
ERROR_CANCELLED = -3			# the payload stream was cancelled, or the protocol removed
ERROR_PAYLOAD_TOO_LARGE = -4	# the peer refused the payload size
ERROR_NOT_CONNECTED = -5		# no connection to send the call to
ERROR_SEND_FAILED = -6			# the message is larger than the transport allows
ERROR_METHOD_FAILED = -7		# the method raised an exception

LANE_CALLS = 0
LANE_PAYLOAD = 1

PAYLOAD_LAST = 1 << 0
PAYLOAD_CANCEL = 1 << 1
PAYLOAD_CHUNK_SIZE = 64 * 1024
PAYLOAD_QUEUE_LIMIT = 4 * PAYLOAD_CHUNK_SIZE
DEFAULT_MAX_PAYLOAD = 256 * 1024 * 1024

RPC_PROTOCOL_NAME = "rpc"
RPC_PROTOCOL_VERSION = 1

_CHUNK_HEADER = struct.Struct("<IB3x")		# streamId, flags, reserved


class RPCError(Exception):
	def __init__(self, code, message):
		super().__init__("RPC error %d: %s" % (code, message))
		self.code = code
		self.message = message


class Response:
	"""A reply: `result` is the "result" section, `payload` a `memoryview` or None."""

	__slots__ = ("result", "payload")

	def __init__(self, result, payload):
		self.result = result
		self.payload = payload


class Future:
	"""The reply of a call: resolved with a `Response` or failed with an `RPCError`."""

	def __init__(self, endpoint):
		self._endpoint = endpoint
		self._callbacks = []
		self.done = False
		self.result = None		# Response
		self.error = None		# RPCError

	def then(self, on_result=None, on_error=None):
		"""Callbacks of the reply, called in `Endpoint.update`, or at once when it is done."""
		if self.done:
			self._call(on_result, on_error)
		else:
			self._callbacks.append((on_result, on_error))
		return self

	def wait(self, timeout=None):
		"""Updates the endpoint until the reply; returns the `Response` or raises `RPCError`.

		For scripts and tests; not from a method or a callback (they run inside `update`).
		"""
		if not self.done and self._endpoint is not None:
			if self._endpoint._updating:
				raise RuntimeError("Future.wait is called inside Endpoint.update")
			deadline = None if timeout is None else time.monotonic() + timeout
			while not self.done:
				self._endpoint.update()
				if self.done:
					break
				if deadline is not None and time.monotonic() > deadline:
					raise TimeoutError("RPC call timed out")
				time.sleep(0.001)

		if self.error is not None:
			raise self.error
		return self.result

	def _call(self, on_result, on_error):
		try:
			if self.error is None:
				if on_result:
					on_result(self.result)
			elif on_error:
				on_error(self.error)
		except Exception:
			log.exception("RPC: future callback failed")

	def _resolve(self, result, error):
		if self.done:
			return
		self.done = True
		self.result = result
		self.error = error
		callbacks, self._callbacks = self._callbacks, []
		for on_result, on_error in callbacks:
			self._call(on_result, on_error)

	@staticmethod
	def failure(code, message):
		future = Future(None)
		future._resolve(None, RPCError(code, message))
		return future


class DeferredReply:
	"""A reply sent later, after the method returned; valid while its connection is open, sent once."""

	def __init__(self, endpoint=None, conn_id=0, call_id=0):
		self._endpoint = endpoint
		self._conn_id = conn_id
		self._call_id = call_id

	@property
	def valid(self):
		conn = self._endpoint._connections.get(self._conn_id) if self._endpoint else None
		return conn is not None and self._call_id in conn.deferred

	def reply(self, result=None, payload=None):
		if self._take():
			self._endpoint._send_reply(self._conn_id, self._call_id, _section("result", result), 0, None, payload)

	def reply_error(self, code, message):
		if self._take():
			self._endpoint._send_reply(self._conn_id, self._call_id, None, code, message, None)

	def _take(self):
		if not self.valid:
			return False
		self._endpoint._connections[self._conn_id].deferred.discard(self._call_id)
		return True


class CallContext:
	"""The call being handled by a method.

	A call the method neither replies nor defers gets the value the method returned (a `Section`,
	a `dict` of the JSON view or None) as its result when the method returns.
	"""

	def __init__(self, endpoint, conn_id, call_id):
		self.endpoint = endpoint
		self.connection_id = conn_id
		self._call_id = call_id
		self._answered = False

	@property
	def is_notify(self):
		return self._call_id == 0

	def reply(self, result=None, payload=None):
		if self._answered or self.is_notify:
			return
		self._answered = True
		self.endpoint._send_reply(self.connection_id, self._call_id, _section("result", result), 0, None, payload)

	def reply_error(self, code, message):
		if self._answered or self.is_notify:
			return
		self._answered = True
		self.endpoint._send_reply(self.connection_id, self._call_id, None, code, message, None)

	def defer(self):
		conn = self.endpoint._connections.get(self.connection_id)
		if self._answered or self.is_notify or conn is None:
			return DeferredReply()
		self._answered = True
		conn.deferred.add(self._call_id)
		return DeferredReply(self.endpoint, self.connection_id, self._call_id)


class Protocol:
	"""A named set of methods bound to one endpoint.

	A method is `func(ctx, params, payload)`: `params` is a `Section`, `payload` a `memoryview` or
	None when the message has none.
	"""

	def __init__(self, name, version=1):
		self.name = name
		self.version = version
		self.endpoint = None
		self._methods = {}

	def method(self, name):
		"""Decorator: `@protocol.method("Name")`."""
		def register(func):
			self._methods[name] = func
			return func
		return register

	def add_method(self, name, func):
		self._methods[name] = func
		return self

	def remote_call(self, method, params=None, payload=None, conn=0):
		"""Calls the peer; client: to its connection, server: `conn` is required. Returns a `Future`.

		`params` is a `Section`, a `dict` of the JSON view or None; `payload` any bytes-like object.
		"""
		if self.endpoint is None:
			return Future.failure(ERROR_NOT_CONNECTED, "protocol is not added to an endpoint")
		return self.endpoint._call(self, conn, method, params, payload)

	def remote_notify(self, method, params=None, payload=None, conn=0):
		"""Notifies the peer; client: its connection, server: all connections when `conn` is 0."""
		if self.endpoint is not None:
			self.endpoint._notify(self, conn, method, params, payload)


class _InStream:
	__slots__ = ("message", "data", "filled", "size", "complete")

	def __init__(self):
		self.message = None		# root Section, when it came
		self.data = bytearray()
		self.filled = 0
		self.size = -1
		self.complete = False


class _OutStream:
	__slots__ = ("id", "data", "offset", "call_id")

	def __init__(self, stream_id, data, call_id):
		self.id = stream_id
		self.data = data
		self.offset = 0
		self.call_id = call_id


class _Connection:
	def __init__(self, conn_id):
		self.id = conn_id
		self.connected = False
		self.call_id_counter = 0
		self.stream_id_counter = 0
		self.pending = {}			# call id -> (Future, Protocol)
		self.deferred = set()
		self.in_streams = {}
		self.dropped = set()
		self.out_streams = collections.deque()
		self.peer_protocols = {}


def _section(name, obj):
	"""A section with the name from a `Section`, a `dict` of the JSON view or None (not changing it)."""
	src = kv.Section.from_json(obj)
	section = kv.Section(name)
	section.values = list(src.values)
	section.keys = list(src.keys)
	return section


def _payload_view(payload):
	if payload is None:
		return None
	if hasattr(payload, "data") and isinstance(payload.data, bytearray):
		payload = payload.data		# PayloadWriter
	return memoryview(payload).cast("B")


class Endpoint:
	"""Connections, call ids, futures and payload streams over a transport (TCP by default).

	A client has one connection; a server has several (`remote_call` needs a connection there,
	`remote_notify` without one goes to all of them). Protocols are added before `listen` /
	`connect`: they are announced to the peer by `rpc.Hello`.
	"""

	CLIENT = 0
	SERVER = 1

	def __init__(self, role=CLIENT, transport=None):
		self.role = role
		self.transport = transport if transport is not None else net.TcpTransport()
		self.max_payload_size = DEFAULT_MAX_PAYLOAD
		self.on_connection = None		# func(conn_id, connected)
		self._connections = {}
		self._protocols = {}
		self._client_conn = 0
		self._updating = False

		self._rpc = Protocol(RPC_PROTOCOL_NAME, RPC_PROTOCOL_VERSION)
		self._rpc.add_method("Hello", self._on_hello)
		self._rpc.add_method("Ping", lambda ctx, params, payload: None)
		self._rpc.add_method("CancelStream", self._on_cancel_stream)
		self.add_protocol(self._rpc)

	# ---- connections

	def listen(self, address):
		"""Server: accepts connections on 'host:port'."""
		assert self.role == Endpoint.SERVER, "Endpoint.listen is for the server"
		return self.transport.listen(address)

	def connect(self, address):
		"""Client: connects to 'host:port'; returns at once, calls made before are queued."""
		assert self.role == Endpoint.CLIENT, "Endpoint.connect is for the client"
		if self._client_conn:
			self.disconnect()

		conn_id = self.transport.connect(address)
		if not conn_id:
			return False

		self._client_conn = conn_id
		conn = self._add_connection(conn_id)
		self._send_hello(conn)
		return True

	def disconnect(self, conn=0):
		"""Closes a connection (the client one when `conn` is 0); its pending calls fail."""
		if not conn:
			conn = self._client_conn
		if conn not in self._connections:
			return
		self.transport.close(conn)
		self._remove_connection(conn, True)

	def close(self):
		"""Disconnects everything and closes the transport."""
		for conn_id in list(self._connections):
			self.disconnect(conn_id)
		self.transport.close_all()

	@property
	def is_connected(self):
		"""Client: the connection is established."""
		conn = self._connections.get(self._client_conn)
		return conn is not None and conn.connected

	@property
	def connection_id(self):
		"""Client: its connection id."""
		return self._client_conn

	@property
	def connection_count(self):
		return sum(1 for conn in self._connections.values() if conn.connected)

	def peer_protocol_version(self, name, conn=0):
		"""The version of a protocol the peer announced, 0 when it has none."""
		connection = self._connections.get(conn or self._client_conn)
		return connection.peer_protocols.get(name, 0) if connection else 0

	def ping(self, conn=0):
		return self._rpc.remote_call("Ping", conn=conn)

	# ---- protocols

	def add_protocol(self, protocol):
		assert protocol.endpoint is None, "protocol '%s' is added to an endpoint already" % protocol.name
		self._protocols[protocol.name] = protocol
		protocol.endpoint = self

	def remove_protocol(self, protocol):
		"""Removes the protocol; its pending calls fail with ERROR_CANCELLED."""
		if self._protocols.get(protocol.name) is not protocol:
			return
		del self._protocols[protocol.name]
		protocol.endpoint = None

		failed = []
		for conn in self._connections.values():
			for call_id, (future, call_protocol) in list(conn.pending.items()):
				if call_protocol is protocol:
					del conn.pending[call_id]
					failed.append(future)
		for future in failed:
			future._resolve(None, RPCError(ERROR_CANCELLED, "protocol removed"))

	# ---- update

	def update(self):
		"""Receives, runs the methods, resolves the futures and sends the payload chunks."""
		self._updating = True
		try:
			for event in self.transport.poll():
				self._handle_event(event)
			for conn in list(self._connections.values()):
				self._send_payload_chunks(conn)
		finally:
			self._updating = False

	def _handle_event(self, event):
		if event.type == net.EVENT_CONNECTED:
			if self.role == Endpoint.SERVER:
				conn = self._add_connection(event.conn)
				self._send_hello(conn)
			else:
				conn = self._connections.get(event.conn)
				if conn is None:
					return
			conn.connected = True
			if self.on_connection:
				self.on_connection(event.conn, True)

		elif event.type == net.EVENT_DISCONNECTED:
			if event.reason == net.DISCONNECT_CONNECT_FAILED:
				log.warning("RPC: connection %d failed", event.conn)
			self._remove_connection(event.conn, True)

		elif event.type == net.EVENT_MESSAGE:
			conn = self._connections.get(event.conn)
			if conn is None:
				return
			if event.lane == LANE_CALLS:
				self._receive_message(conn, event.message)
			elif event.lane == LANE_PAYLOAD:
				self._receive_chunk(conn, event.message)

	def _add_connection(self, conn_id):
		conn = _Connection(conn_id)
		self._connections[conn_id] = conn
		self.transport.configure_lanes(conn_id, [(0, 1), (1, 1)])
		return conn

	def _remove_connection(self, conn_id, notify):
		conn = self._connections.pop(conn_id, None)
		if conn is None:
			return
		if conn_id == self._client_conn:
			self._client_conn = 0

		for future, _ in conn.pending.values():
			future._resolve(None, RPCError(ERROR_DISCONNECTED, "disconnected"))
		if notify and conn.connected and self.on_connection:
			self.on_connection(conn_id, False)

	def _find(self, conn_id):
		if not conn_id:
			assert self.role == Endpoint.CLIENT, "a server call needs a connection"
			conn_id = self._client_conn
		return self._connections.get(conn_id)

	# ---- sending

	def _call(self, protocol, conn_id, method, params, payload):
		conn = self._find(conn_id)
		if conn is None:
			return Future.failure(ERROR_NOT_CONNECTED, "not connected")

		conn.call_id_counter += 1
		call_id = conn.call_id_counter

		root = kv.Section()
		root.add("id", call_id)
		root.add("protocol", protocol.name)
		root.add("method", method)
		root.keys.append(_section("params", params))
		has_stream = self._attach_payload(conn, root, payload, call_id)

		if not self._send_root(conn, root):
			# the peer never heard of the stream, it is only dropped here
			if has_stream:
				conn.out_streams.pop()
			return Future.failure(ERROR_SEND_FAILED, "message is too large")

		future = Future(self)
		conn.pending[call_id] = (future, protocol)
		return future

	def _notify(self, protocol, conn_id, method, params, payload):
		root = kv.Section()
		root.add("protocol", protocol.name)
		root.add("method", method)
		root.keys.append(_section("params", params))

		if not conn_id and self.role == Endpoint.SERVER:
			targets = list(self._connections.values())
		else:
			conn = self._find(conn_id)
			targets = [conn] if conn else []

		for conn in targets:
			message = kv.Section()
			message.keys = list(root.keys)
			self._attach_payload(conn, message, payload, 0)
			self._send_root(conn, message)

	def _send_reply(self, conn_id, call_id, result, code, message, payload):
		conn = self._connections.get(conn_id)
		if conn is None:
			return

		root = kv.Section()
		root.add("id", call_id)
		if result is not None:
			root.keys.append(result)
			self._attach_payload(conn, root, payload, 0)
		else:
			error = root.add("error")
			error.add("code", code)
			error.add("message", message or "")
		self._send_root(conn, root)

	def _send_hello(self, conn):
		params = kv.Section("params")
		protocols = params.add("protocols")
		for protocol in self._protocols.values():
			protocols.add(protocol.name, protocol.version)

		root = kv.Section()
		root.add("protocol", RPC_PROTOCOL_NAME)
		root.add("method", "Hello")
		root.keys.append(params)
		self._send_root(conn, root)

	def _attach_payload(self, conn, root, payload, call_id):
		"""Adds the payload section; returns True when an outgoing stream was added."""
		view = _payload_view(payload)
		if view is None:
			return False

		section = root.add("payload")
		if not len(view):
			# an empty payload has no stream and no chunks
			section.add("stream", 0)
			section.add("size", 0)
			return False

		if len(view) > 0x7FFFFFFF:
			raise ValueError("RPC payload must be smaller than 2 GB")

		conn.stream_id_counter += 1
		conn.out_streams.append(_OutStream(conn.stream_id_counter, view, call_id))
		section.add("stream", conn.stream_id_counter)
		section.add("size", len(view))
		return True

	def _send_root(self, conn, root):
		data = root.to_binary()
		if len(data) > self.transport.max_message_size:
			log.error("RPC: message of %d bytes is larger than the transport allows, send the data as payload", len(data))
			return False
		return self.transport.send(conn.id, LANE_CALLS, data)

	def _send_payload_chunks(self, conn):
		chunk_size = min(PAYLOAD_CHUNK_SIZE, self.transport.max_message_size - _CHUNK_HEADER.size)
		while conn.out_streams and self.transport.queued_bytes(conn.id, LANE_PAYLOAD) < PAYLOAD_QUEUE_LIMIT:
			stream = conn.out_streams[0]
			end = min(stream.offset + chunk_size, len(stream.data))
			last = end >= len(stream.data)
			self._send_chunk(conn, stream.id, PAYLOAD_LAST if last else 0, stream.data[stream.offset:end])
			stream.offset = end
			if last:
				conn.out_streams.popleft()

	def _send_chunk(self, conn, stream_id, flags, data):
		chunk = bytearray(_CHUNK_HEADER.pack(stream_id, flags))
		chunk += data
		self.transport.send(conn.id, LANE_PAYLOAD, chunk)

	def _cancel_out_stream(self, conn, stream_id):
		"""Drops the outgoing stream, tells the peer, and fails our call that had it as payload."""
		for stream in conn.out_streams:
			if stream.id != stream_id:
				continue
			conn.out_streams.remove(stream)
			self._send_chunk(conn, stream_id, PAYLOAD_CANCEL, b"")

			pending = conn.pending.pop(stream.call_id, None) if stream.call_id else None
			if pending:
				pending[0]._resolve(None, RPCError(ERROR_CANCELLED, "payload cancelled"))
			return

	def _refuse_payload(self, conn, call_id, stream_id):
		log.error("RPC: payload stream %d of connection %d is too large, refused", stream_id, conn.id)
		if call_id:
			self._send_reply(conn.id, call_id, None, ERROR_PAYLOAD_TOO_LARGE, "payload is too large", None)
		self._rpc.remote_notify("CancelStream", {"stream": stream_id}, conn=conn.id)

	# ---- receiving

	def _broken(self, conn, what):
		log.error("RPC: %s from connection %d, disconnecting", what, conn.id)
		self.disconnect(conn.id)

	def _fail_pending(self, conn, root, code, message):
		pending = conn.pending.pop(root.value("id", 0), None)
		if pending:
			pending[0]._resolve(None, RPCError(code, message))

	def _receive_message(self, conn, data):
		try:
			root = kv.Section.from_binary(data)
		except kv.KVError:
			self._broken(conn, "broken message")
			return

		payload = root.find("payload")
		if payload is None:
			self._process(conn, root, None)
			return

		stream_id = payload.value("stream", 0)
		size = payload.value("size", 0)
		if size == 0:
			self._process(conn, root, memoryview(bytearray()))
			return

		stream = conn.in_streams.setdefault(stream_id, _InStream())
		if not isinstance(size, int) or size < 0 or size > self.max_payload_size or stream.filled > size:
			# chunks still coming for the stream are dropped until its last or cancel chunk
			if not stream.complete:
				conn.dropped.add(stream_id)
			del conn.in_streams[stream_id]

			# a reply with a payload we can't take fails its call, a call is refused
			if root.find("result") is not None:
				self._fail_pending(conn, root, ERROR_PAYLOAD_TOO_LARGE, "reply payload is too large")
				self._refuse_payload(conn, 0, stream_id)
			else:
				self._refuse_payload(conn, root.value("id", 0), stream_id)
			return

		stream.message = root
		stream.size = size
		stream.data.extend(bytes(size - len(stream.data)))
		if not stream.complete:
			return

		# all chunks came before the message
		del conn.in_streams[stream_id]
		if stream.filled != size:
			self._broken(conn, "payload stream %d shorter than announced" % stream_id)
			return
		self._process(conn, root, memoryview(stream.data))

	def _receive_chunk(self, conn, data):
		if len(data) < _CHUNK_HEADER.size:
			self._broken(conn, "broken payload chunk")
			return
		stream_id, flags = _CHUNK_HEADER.unpack_from(data)

		if stream_id in conn.dropped:
			if flags & (PAYLOAD_LAST | PAYLOAD_CANCEL):
				conn.dropped.discard(stream_id)
			return

		stream = conn.in_streams.get(stream_id)
		if flags & PAYLOAD_CANCEL:
			if stream is None:
				return
			# the peer could not send its payload
			del conn.in_streams[stream_id]
			if stream.message is not None and stream.message.find("result") is not None:
				self._fail_pending(conn, stream.message, ERROR_CANCELLED, "reply payload cancelled")
			return

		if stream is None:
			# a chunk may come before its message on backends with independent lanes
			stream = conn.in_streams[stream_id] = _InStream()

		size = len(data) - _CHUNK_HEADER.size
		limit = stream.size if stream.size >= 0 else self.max_payload_size
		if stream.filled + size > limit:
			self._broken(conn, "payload stream %d larger than announced" % stream_id)
			return

		end = stream.filled + size
		if end > len(stream.data):
			stream.data.extend(bytes(end - len(stream.data)))
		stream.data[stream.filled:end] = memoryview(data)[_CHUNK_HEADER.size:]
		stream.filled = end

		if not flags & PAYLOAD_LAST:
			return
		stream.complete = True
		if stream.message is None:
			return

		del conn.in_streams[stream_id]
		if stream.filled != stream.size:
			self._broken(conn, "payload stream %d shorter than announced" % stream_id)
			return
		self._process(conn, stream.message, memoryview(stream.data))

	def _process(self, conn, root, payload):
		if root.find("result") is not None or root.find("error") is not None:
			self._dispatch_reply(conn, root, payload)
		else:
			self._dispatch_call(conn, root, payload)

	def _dispatch_reply(self, conn, root, payload):
		pending = conn.pending.pop(root.value("id", 0), None)
		if pending is None:
			return

		error = root.find("error")
		if error is not None:
			pending[0]._resolve(None, RPCError(error.value("code", 0), error.value("message", "")))
			return

		pending[0]._resolve(Response(root.find("result"), payload), None)

	def _dispatch_call(self, conn, root, payload):
		call_id = root.value("id", 0)
		protocol_name = root.value("protocol", "")
		method_name = root.value("method", "")

		protocol = self._protocols.get(protocol_name)
		method = protocol._methods.get(method_name) if protocol else None
		if method is None:
			if call_id:
				self._send_reply(conn.id, call_id, None, ERROR_UNKNOWN_METHOD, "unknown method %s.%s" % (protocol_name, method_name), None)
			else:
				log.debug("RPC: notify of unknown method %s.%s", protocol_name, method_name)
			return

		ctx = CallContext(self, conn.id, call_id)
		params = root.find("params") or kv.Section("params")
		try:
			result = method(ctx, params, payload)
		except Exception as e:
			log.exception("RPC: method %s.%s failed", protocol_name, method_name)
			ctx.reply_error(ERROR_METHOD_FAILED, "%s.%s: %s" % (protocol_name, method_name, e))
			return

		if not ctx._answered:
			ctx.reply(result)

	# ---- built-in rpc protocol

	def _on_hello(self, ctx, params, payload):
		conn = self._connections.get(ctx.connection_id)
		protocols = params.find("protocols")
		if conn is None or protocols is None:
			return

		conn.peer_protocols = {}
		for key in protocols.keys:
			version = key.values[0] if key.values else 0
			conn.peer_protocols[key.name] = version
			ours = self._protocols.get(key.name)
			if ours is not None and ours.version != version:
				log.warning("RPC: protocol '%s' version %d, the peer has version %s", key.name, ours.version, version)

	def _on_cancel_stream(self, ctx, params, payload):
		conn = self._connections.get(ctx.connection_id)
		if conn is not None:
			self._cancel_out_stream(conn, params.value("stream", 0))
