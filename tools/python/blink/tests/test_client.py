"""Tests of the BLINK client against a Python server with blink.Hello (no Blender needed)."""

import logging
import time
import unittest

from eqrpc import Endpoint, Protocol

import blink

logging.getLogger("eqrpc").setLevel(logging.CRITICAL)
logging.getLogger("blink").setLevel(logging.CRITICAL)


class Server:
	"""A CBlinkServer stand-in: blink.Hello and an application protocol."""

	def __init__(self, port=0):
		self.endpoint = Endpoint(Endpoint.SERVER)
		protocol = Protocol("blink")
		protocol.add_method("Hello", self._on_hello)
		self.endpoint.add_protocol(protocol)
		self.endpoint.add_protocol(Protocol("app"))
		assert self.endpoint.listen("127.0.0.1:%d" % port)
		self.port = self.endpoint.transport.listen_port

	def _on_hello(self, ctx, params, payload):
		ctx.reply({"app": "TestApp", "appVersion": "1.2"})

	def close(self):
		self.endpoint.close()


class AppClient(blink.Client):
	def __init__(self):
		super().__init__("Test addon", "0.1", "Python", "3", server_title="the test app")
		self.ready = 0
		self.closed = 0

	def create_protocols(self):
		return [Protocol("app")]

	def on_ready(self):
		self.ready += 1

	def on_closed(self):
		self.closed += 1


def _pump(condition, *updatables, timeout=5.0):
	end = time.monotonic() + timeout
	while time.monotonic() < end:
		for item in updatables:
			item.update()
		if condition():
			return True
		time.sleep(0.005)
	return False


class ClientTests(unittest.TestCase):
	def test_hello(self):
		server = Server()
		client = AppClient()
		try:
			client.connect("127.0.0.1:%d" % server.port)
			self.assertEqual(client.state, blink.STATE_CONNECTING)
			self.assertTrue(_pump(lambda: client.state == blink.STATE_CONNECTED, server.endpoint, client))
			self.assertEqual(client.ready, 1)
			self.assertEqual((client.server_app, client.server_version), ("TestApp", "1.2"))
			self.assertEqual(client.endpoint.peer_protocol_version("app"), 1)

			client.disconnect()
			self.assertEqual(client.state, blink.STATE_DISCONNECTED)
			self.assertFalse(client.is_active)
			self.assertEqual(client.closed, 1)
		finally:
			client.disconnect()
			server.close()

	def test_waits_and_reconnects(self):
		# no server yet: the client waits and tries again
		probe = Server()
		port = probe.port
		probe.close()

		client = AppClient()
		try:
			client.connect("127.0.0.1:%d" % port, reconnect_interval=0.2)
			self.assertTrue(_pump(lambda: client.state == blink.STATE_WAITING, client))
			self.assertEqual(client.status_text(), "Waiting for the test app")
			self.assertTrue(client.is_active)

			server = Server(port)
			try:
				self.assertTrue(_pump(lambda: client.state == blink.STATE_CONNECTED, server.endpoint, client))
				self.assertEqual(client.ready, 1)

				# the server is restarted: the client waits, then connects again
				server.close()
				self.assertTrue(_pump(lambda: client.state == blink.STATE_WAITING, client))
				self.assertIn("the test app closed the link", client.message)
				server = Server(port)
				self.assertTrue(_pump(lambda: client.state == blink.STATE_CONNECTED, server.endpoint, client))
				self.assertEqual(client.ready, 2)
			finally:
				server.close()
		finally:
			client.disconnect()


if __name__ == "__main__":
	unittest.main()
