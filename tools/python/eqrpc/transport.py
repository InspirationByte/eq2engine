"""TCP transport compatible with the engine `CNetTransportTCP`.

Non-blocking sockets polled from one thread, no thread of its own. The stream is a sequence of
frames, a frame is one message of one lane:

    struct NetTCPFrameHeader { uint32 size; uint16 lane; uint16 flags; }   // little-endian

Each lane has its own send queue; the next frame is taken from the lane of the lowest priority
value that has one (by the sent bytes over the weight among equal priorities).
"""

import collections
import errno
import select
import socket
import struct

DEFAULT_MAX_MESSAGE = 1024 * 1024

# ENetDisconnectReason
DISCONNECT_REMOTE = 1
DISCONNECT_CONNECT_FAILED = 2
DISCONNECT_ERROR = 3
DISCONNECT_PROTOCOL = 4

# event types
EVENT_CONNECTED = "connected"
EVENT_DISCONNECTED = "disconnected"
EVENT_MESSAGE = "message"

_FRAME_HEADER = struct.Struct("<IHH")
_RECV_LIMIT = 1024 * 1024		# bytes received per connection per poll
_WOULD_BLOCK = {errno.EWOULDBLOCK, errno.EAGAIN, errno.EINPROGRESS, getattr(errno, "WSAEWOULDBLOCK", 10035)}


def _would_block(error):
	return isinstance(error, BlockingIOError) or error.errno in _WOULD_BLOCK


class Event:
	__slots__ = ("type", "conn", "lane", "reason", "message")

	def __init__(self, type, conn, lane=0, reason=0, message=None):
		self.type = type
		self.conn = conn
		self.lane = lane
		self.reason = reason
		self.message = message		# bytes of EVENT_MESSAGE

	def __repr__(self):
		return "Event(%s, conn=%d, lane=%d, reason=%d)" % (self.type, self.conn, self.lane, self.reason)


class _Lane:
	__slots__ = ("priority", "weight", "queue", "sent_bytes", "queued_bytes")

	def __init__(self, priority=0, weight=1):
		self.priority = priority
		self.weight = max(1, weight)
		self.queue = collections.deque()
		self.sent_bytes = 0
		self.queued_bytes = 0


class _Connection:
	def __init__(self, conn_id, sock, connecting):
		self.id = conn_id
		self.sock = sock
		self.connecting = connecting
		self.closing = False
		self.lanes = [_Lane()]
		self.send_frame = None		# memoryview of the frame being sent
		self.send_lane = None
		self.recv_buffer = bytearray()


def parse_address(address, default_host):
	"""'host:port' to (host, port); an empty host is `default_host`."""
	host, sep, port = address.rpartition(":")
	if not sep:
		raise ValueError("address '%s' has no port" % address)
	return (host or default_host, int(port))


class TcpTransport:
	"""Message connections over TCP: `listen`, `connect`, `send`, `poll`."""

	def __init__(self, max_message_size=DEFAULT_MAX_MESSAGE):
		self.max_message_size = max_message_size
		self._listen_sock = None
		self._connections = {}
		self._id_counter = 0
		self._events = []

	def close_all(self):
		"""Closes the listening socket and every connection at once, without events."""
		for conn in list(self._connections.values()):
			self._destroy(conn)
		if self._listen_sock:
			self._listen_sock.close()
			self._listen_sock = None

	# ---- connections

	def listen(self, address):
		"""Accepts connections on 'host:port'; port 0 takes a free port (`listen_port`)."""
		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			sock.bind(parse_address(address, "0.0.0.0"))
			sock.listen(16)
			sock.setblocking(False)
		except OSError:
			sock.close()
			return False
		self._listen_sock = sock
		return True

	@property
	def listen_port(self):
		return self._listen_sock.getsockname()[1] if self._listen_sock else 0

	def connect(self, address):
		"""Starts a connection, returns its id at once (0 for a bad address).

		EVENT_CONNECTED or EVENT_DISCONNECTED with DISCONNECT_CONNECT_FAILED follows in `poll`;
		messages sent before are queued.
		"""
		try:
			target = socket.getaddrinfo(*parse_address(address, "127.0.0.1"), socket.AF_INET, socket.SOCK_STREAM)[0][4]
		except (OSError, ValueError):
			return 0

		sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		sock.setblocking(False)
		sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
		conn = self._add(sock, connecting=True)

		result = sock.connect_ex(target)
		if result != 0 and result not in _WOULD_BLOCK:
			self._events.append(Event(EVENT_DISCONNECTED, conn.id, reason=DISCONNECT_CONNECT_FAILED))
			self._destroy(conn)
		return conn.id

	def close(self, conn_id):
		"""Sends the queued messages and closes; no event comes for a connection closed locally."""
		conn = self._connections.get(conn_id)
		if conn is None:
			return
		if conn.connecting or not self._has_queued(conn):
			self._destroy(conn)
		else:
			conn.closing = True

	def configure_lanes(self, conn_id, lanes):
		"""`lanes` is a list of (priority, weight); call before the first `send` of the connection."""
		conn = self._connections.get(conn_id)
		if conn is None:
			return False
		conn.lanes = [_Lane(priority, weight) for priority, weight in lanes]
		return True

	def is_open(self, conn_id):
		conn = self._connections.get(conn_id)
		return conn is not None and not conn.closing

	# ---- messages

	def send(self, conn_id, lane, data):
		"""Queues a reliable message (any bytes-like object, copied); False when it can't be sent."""
		conn = self._connections.get(conn_id)
		if conn is None or conn.closing or lane < 0 or lane >= len(conn.lanes):
			return False
		if len(data) > self.max_message_size:
			return False

		frame = bytearray(_FRAME_HEADER.pack(len(data), lane, 0))
		frame += data
		target = conn.lanes[lane]
		target.queue.append(frame)
		target.queued_bytes += len(frame)
		return True

	def queued_bytes(self, conn_id, lane):
		conn = self._connections.get(conn_id)
		if conn is None or lane < 0 or lane >= len(conn.lanes):
			return 0
		return conn.lanes[lane].queued_bytes

	def poll(self):
		"""Accepts, connects, receives and sends; returns the list of events since the last poll."""
		self._accept()
		for conn in list(self._connections.values()):
			if conn.id not in self._connections:
				continue
			if conn.connecting and not self._update_connecting(conn):
				continue
			reason = self._receive(conn)
			if reason:
				self._events.append(Event(EVENT_DISCONNECTED, conn.id, reason=reason))
				self._destroy(conn)
				continue
			if not self._flush(conn):
				self._events.append(Event(EVENT_DISCONNECTED, conn.id, reason=DISCONNECT_ERROR))
				self._destroy(conn)
				continue
			if conn.closing and not self._has_queued(conn):
				self._destroy(conn)

		events = self._events
		self._events = []
		return events

	# ---- internals

	def _add(self, sock, connecting):
		self._id_counter += 1
		conn = _Connection(self._id_counter, sock, connecting)
		self._connections[conn.id] = conn
		return conn

	def _destroy(self, conn):
		self._connections.pop(conn.id, None)
		try:
			if not conn.connecting:
				conn.sock.shutdown(socket.SHUT_WR)
		except OSError:
			pass
		conn.sock.close()

	def _accept(self):
		if not self._listen_sock:
			return
		while True:
			try:
				sock, _ = self._listen_sock.accept()
			except OSError:
				return
			sock.setblocking(False)
			sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
			conn = self._add(sock, connecting=False)
			self._events.append(Event(EVENT_CONNECTED, conn.id))

	def _update_connecting(self, conn):
		_, writable, failed = select.select([], [conn.sock], [conn.sock], 0)
		if not writable and not failed:
			return False

		error = conn.sock.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)
		if failed or error:
			self._events.append(Event(EVENT_DISCONNECTED, conn.id, reason=DISCONNECT_CONNECT_FAILED))
			self._destroy(conn)
			return False

		conn.connecting = False
		self._events.append(Event(EVENT_CONNECTED, conn.id))
		return True

	def _receive(self, conn):
		"""Reads the socket and splits the frames; returns the disconnect reason, 0 when fine."""
		received = 0
		while received < _RECV_LIMIT:
			try:
				data = conn.sock.recv(min(65536, _RECV_LIMIT - received))
			except OSError as e:
				if _would_block(e):
					break
				return DISCONNECT_REMOTE if isinstance(e, (ConnectionResetError, ConnectionAbortedError)) else DISCONNECT_ERROR
			if not data:
				reason = self._split_frames(conn)
				return reason or DISCONNECT_REMOTE
			conn.recv_buffer += data
			received += len(data)

		return self._split_frames(conn)

	def _split_frames(self, conn):
		buffer = conn.recv_buffer
		offset = 0
		while len(buffer) - offset >= _FRAME_HEADER.size:
			size, lane, _ = _FRAME_HEADER.unpack_from(buffer, offset)
			if size > self.max_message_size:
				return DISCONNECT_PROTOCOL
			end = offset + _FRAME_HEADER.size + size
			if end > len(buffer):
				break
			message = bytes(buffer[offset + _FRAME_HEADER.size:end])
			self._events.append(Event(EVENT_MESSAGE, conn.id, lane=lane, message=message))
			offset = end
		if offset:
			del buffer[:offset]
		return 0

	def _has_queued(self, conn):
		return conn.send_frame is not None or any(lane.queue for lane in conn.lanes)

	def _pick_lane(self, conn):
		best = None
		for lane in conn.lanes:
			if not lane.queue:
				continue
			if best is None or lane.priority < best.priority or \
				(lane.priority == best.priority and lane.sent_bytes * best.weight < best.sent_bytes * lane.weight):
				best = lane
		return best

	def _flush(self, conn):
		"""Writes the queued frames while the socket takes them; False on a socket error."""
		while True:
			if conn.send_frame is None:
				lane = self._pick_lane(conn)
				if lane is None:
					return True
				conn.send_frame = memoryview(lane.queue.popleft())
				conn.send_lane = lane

			try:
				sent = conn.sock.send(conn.send_frame)
			except OSError as e:
				if _would_block(e):
					return True
				return False

			conn.send_lane.sent_bytes += sent
			conn.send_lane.queued_bytes -= sent
			conn.send_frame = conn.send_frame[sent:]
			if not len(conn.send_frame):
				conn.send_frame = None
				conn.send_lane = None
