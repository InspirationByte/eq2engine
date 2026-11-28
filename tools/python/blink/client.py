"""BLINK client: the connection to the `CBlinkServer` of an engine application.

Connect -> blink.Hello (names and versions) -> `on_ready`, then the application protocols. When the
server is not running yet or closes the link (it was restarted), the client waits and connects
again every `reconnect_interval` seconds until `disconnect`. No bpy: `update` is called from the
main thread (in Blender by the `blink.blender` timer); the endpoint update runs the received
methods and the reply callbacks.
"""

import logging
import time

from .rpc import Endpoint, Protocol, ERROR_DISCONNECTED

log = logging.getLogger("blink")

DEFAULT_PORT = 27100
DEFAULT_RECONNECT_INTERVAL = 5.0

STATE_DISCONNECTED = "Disconnected"
STATE_CONNECTING = "Connecting"
STATE_WAITING = "Waiting"			# for the server: connects again after reconnect_interval
STATE_CONNECTED = "Connected"		# blink.Hello is answered
STATE_FAILED = "Failed"


class Client:
	"""Subclasses add their protocols in `create_protocols` and start their sync in `on_ready`."""

	def __init__(self, name, version, host="", host_version="", server_title="the server"):
		self.name = name					# of the client (the addon), sent in blink.Hello
		self.version = version
		self.host = host					# the application of the client ("Blender")
		self.host_version = host_version
		self.server_title = server_title	# for the UI: "the level editor"
		self.endpoint = None
		self.protocols = {}					# name -> Protocol of the current endpoint
		self.state = STATE_DISCONNECTED
		self.message = ""
		self.address = ""					# host:port the client connects to, the server listens there
		self.server_app = ""				# of the blink.Hello reply
		self.server_version = ""
		self.reconnect_time = None			# when to connect again after the link was lost
		self.reconnect_interval = DEFAULT_RECONNECT_INTERVAL

	# ---- for subclasses

	def create_protocols(self):
		"""New protocols of the application for each connection attempt (an endpoint)."""
		return []

	def on_ready(self):
		"""blink.Hello is answered: the application protocols can be called."""

	def on_closed(self):
		"""The endpoint is closed: by `disconnect`, before a reconnect or when the link is lost."""

	def on_update(self):
		"""From `update` while connected."""

	def status_text(self):
		"""The state the UI shows; applications add their own (syncing, ...)."""
		if self.state == STATE_WAITING:
			return "Waiting for %s" % self.server_title
		return self.state

	def ui_state(self):
		"""What the UI shows; it is redrawn when this changes."""
		return (self.status_text(), self.message, self.address)

	# ----

	@property
	def is_active(self):
		"""Connected, connecting or waiting to connect again."""
		return self.endpoint is not None

	def report_error(self, what, error):
		"""Logs a failed call and shows it; the failures of a lost link are not shown."""
		if self.endpoint is None or self.state == STATE_WAITING:
			return
		log.warning("BLINK: %s failed: %s", what, error)
		self.message = "%s: %s" % (what, error.message)

	def fail(self, what, error):
		self.report_error(what, error)
		self.state = STATE_FAILED

	def connect(self, address, reconnect_interval=DEFAULT_RECONNECT_INTERVAL):
		self.disconnect()
		self.address = address
		self.reconnect_interval = reconnect_interval
		self.state = STATE_CONNECTING
		self._open()

	def disconnect(self):
		self._close_endpoint()
		self.reconnect_time = None
		self.state = STATE_DISCONNECTED
		self.message = ""

	def update(self):
		if self.reconnect_time is not None and time.monotonic() >= self.reconnect_time:
			self._open()
		if self.endpoint is None:
			return
		self.endpoint.update()
		if self.endpoint is not None and self.state != STATE_WAITING:		# a callback may close it
			self.on_update()

	# ----

	def _open(self):
		self._close_endpoint()
		self.reconnect_time = None

		self.endpoint = Endpoint(Endpoint.CLIENT)
		self.protocols = {"blink": Protocol("blink")}
		for protocol in self.create_protocols():
			self.protocols[protocol.name] = protocol

		# rpc.Hello of the endpoint lists the protocols added before connect
		for protocol in self.protocols.values():
			self.endpoint.add_protocol(protocol)
		self.endpoint.on_connection = self._on_connection
		self.endpoint.connect(self.address)

		params = {"client": self.name, "clientVersion": self.version, "host": self.host, "hostVersion": self.host_version}
		self.protocols["blink"].remote_call("Hello", params).then(self._on_hello, self._on_hello_failed)

	def _close_endpoint(self):
		endpoint = self.endpoint
		self.endpoint = None		# set first: the failed calls of close() see the link closed
		self.protocols = {}
		if endpoint is not None:
			endpoint.close()
			self.on_closed()

	def _wait_server(self, message):
		"""The server is not running or closed the link: connect again later.

		From the endpoint update (a callback), so the endpoint is closed by the next `_open`.
		"""
		self.state = STATE_WAITING
		self.message = message
		self.reconnect_time = time.monotonic() + self.reconnect_interval

	def _on_connection(self, conn, connected):
		if connected:
			self.message = ""
		elif self.state != STATE_WAITING:
			self._wait_server("%s closed the link" % self.server_title)

	def _on_hello(self, response):
		result = response.result
		self.server_app = result.value("app", "")
		self.server_version = result.value("appVersion", "")
		self.state = STATE_CONNECTED
		self.message = ""
		self.on_ready()

	def _on_hello_failed(self, error):
		if self.endpoint is None:		# disconnect
			return
		if error.code == ERROR_DISCONNECTED:
			if self.state != STATE_WAITING:
				self._wait_server("nothing listens at %s" % self.address)
			return
		self.fail("blink.Hello", error)
