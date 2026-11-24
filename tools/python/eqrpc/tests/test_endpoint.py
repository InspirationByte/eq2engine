import array
import logging
import time
import unittest

import eqrpc
from eqrpc import Endpoint, Protocol, PayloadReader, PayloadWriter, RPCError

logging.getLogger("eqrpc").setLevel(logging.CRITICAL)


class Link:
	"""A Python server and client over TCP on 127.0.0.1."""

	def __init__(self, server_protocols=(), client_protocols=()):
		self.server = Endpoint(Endpoint.SERVER)
		self.client = Endpoint(Endpoint.CLIENT)
		for protocol in server_protocols:
			self.server.add_protocol(protocol)
		for protocol in client_protocols:
			self.client.add_protocol(protocol)
		assert self.server.listen("127.0.0.1:0")
		assert self.client.connect("127.0.0.1:%d" % self.server.transport.listen_port)
		self.pump_until(lambda: self.client.is_connected and self.server.connection_count == 1)

	def update(self):
		self.server.update()
		self.client.update()

	def pump_until(self, condition, timeout=5.0):
		deadline = time.monotonic() + timeout
		while not condition():
			if time.monotonic() > deadline:
				raise TimeoutError("condition was not met")
			self.update()
			time.sleep(0.001)

	def wait(self, future, timeout=5.0):
		self.pump_until(lambda: future.done, timeout)
		return future.wait()

	def close(self):
		self.client.close()
		self.server.close()


class EndpointTests(unittest.TestCase):
	def setUp(self):
		self.links = []

	def tearDown(self):
		for link in self.links:
			link.close()

	def link(self, server_protocols=(), client_protocols=()):
		link = Link(server_protocols, client_protocols)
		self.links.append(link)
		return link

	def test_call_and_reply(self):
		math = Protocol("math", 2)

		@math.method("Add")
		def add(ctx, params, payload):
			return {"sum": params.value("a") + params.value("b")}

		client_math = Protocol("math", 2)
		link = self.link([math], [client_math])
		response = link.wait(client_math.remote_call("Add", {"a": 2, "b": 3}))
		self.assertEqual(response.result.value("sum"), 5)
		self.assertIsNone(response.payload)
		self.assertEqual(link.client.peer_protocol_version("math"), 2)
		self.assertEqual(link.client.peer_protocol_version("rpc"), 1)

	def test_then_and_ping(self):
		link = self.link()
		results = []
		link.client.ping().then(lambda response: results.append(response))
		link.pump_until(lambda: results)
		self.assertEqual(results[0].result.keys, [])

	def test_notify_and_broadcast(self):
		server_events = Protocol("events")
		client_events = Protocol("events")
		got_server, got_client = [], []
		server_events.add_method("Ping", lambda ctx, params, payload: got_server.append((ctx.is_notify, params.value("n"))))
		client_events.add_method("Tick", lambda ctx, params, payload: got_client.append(params.value("n")))

		link = self.link([server_events], [client_events])
		client_events.remote_notify("Ping", {"n": 1})
		link.pump_until(lambda: got_server)
		self.assertEqual(got_server, [(True, 1)])

		server_events.remote_notify("Tick", {"n": 7})
		link.pump_until(lambda: got_client)
		self.assertEqual(got_client, [7])

	def test_errors(self):
		test = Protocol("test")
		test.add_method("Fail", lambda ctx, params, payload: ctx.reply_error(5, "bad"))
		test.add_method("Raise", lambda ctx, params, payload: 1 / 0)
		test.add_method("Empty", lambda ctx, params, payload: None)
		client_test = Protocol("test")
		link = self.link([test], [client_test])

		with self.assertRaises(RPCError) as error:
			link.wait(client_test.remote_call("Missing"))
		self.assertEqual(error.exception.code, eqrpc.ERROR_UNKNOWN_METHOD)

		with self.assertRaises(RPCError) as error:
			link.wait(client_test.remote_call("Fail"))
		self.assertEqual((error.exception.code, error.exception.message), (5, "bad"))

		with self.assertRaises(RPCError) as error:
			link.wait(client_test.remote_call("Raise"))
		self.assertEqual(error.exception.code, eqrpc.ERROR_METHOD_FAILED)

		self.assertEqual(link.wait(client_test.remote_call("Empty")).result.keys, [])

	def test_deferred_reply(self):
		test = Protocol("test")
		deferred = []
		test.add_method("Later", lambda ctx, params, payload: deferred.append(ctx.defer()))
		client_test = Protocol("test")
		link = self.link([test], [client_test])

		future = client_test.remote_call("Later")
		link.pump_until(lambda: deferred)
		for _ in range(10):
			link.update()
		self.assertFalse(future.done)

		reply = deferred[0]
		self.assertTrue(reply.valid)
		reply.reply({"ok": True})
		reply.reply({"ok": False})		# a second reply is dropped
		self.assertFalse(reply.valid)
		self.assertIs(link.wait(future).result.value("ok"), True)

	def test_payload_both_ways(self):
		test = Protocol("test")

		@test.method("Reverse")
		def reverse(ctx, params, payload):
			ctx.reply({"size": len(payload)}, payload=bytes(payload)[::-1])

		client_test = Protocol("test")
		link = self.link([test], [client_test])

		data = bytes(range(256)) * 1000		# 256000 bytes: several chunks
		response = link.wait(client_test.remote_call("Reverse", payload=data))
		self.assertEqual(response.result.value("size"), len(data))
		self.assertEqual(bytes(response.payload), data[::-1])

		response = link.wait(client_test.remote_call("Reverse", payload=b""))
		self.assertEqual(response.result.value("size"), 0)
		self.assertEqual(len(response.payload), 0)

	def test_payload_layout(self):
		test = Protocol("test")

		@test.method("Mesh")
		def mesh(ctx, params, payload):
			reader = PayloadReader(payload)
			positions = reader.read_array("f", 3)
			indices = reader.read_array("I")
			return {"positions": len(positions) // 3, "last": positions[-1], "indices": list(indices)}

		client_test = Protocol("test")
		link = self.link([test], [client_test])

		writer = PayloadWriter()
		writer.write_array(array.array("f", [0, 0, 0, 1, 0, 0, 0, 1, 2.5]), components=3)
		writer.write_array(array.array("I", [0, 1, 2]))
		self.assertEqual(len(writer.data), 4 + 36 + 4 + 12)

		result = link.wait(client_test.remote_call("Mesh", payload=writer)).result
		self.assertEqual(result.value("positions"), 3)
		self.assertEqual(result.value("last"), 2.5)
		self.assertEqual(result.find("indices").values, [0, 1, 2])

	def test_payload_too_large(self):
		test = Protocol("test")
		test.add_method("Take", lambda ctx, params, payload: None)
		client_test = Protocol("test")
		link = self.link([test], [client_test])
		link.server.max_payload_size = 1000

		with self.assertRaises(RPCError) as error:
			link.wait(client_test.remote_call("Take", payload=bytes(200000)))
		self.assertEqual(error.exception.code, eqrpc.ERROR_PAYLOAD_TOO_LARGE)

		# the connection keeps working
		self.assertIsNotNone(link.wait(client_test.remote_call("Take", payload=bytes(100))))

	def test_disconnect_fails_calls(self):
		test = Protocol("test")
		test.add_method("Never", lambda ctx, params, payload: ctx.defer())
		client_test = Protocol("test")
		link = self.link([test], [client_test])

		events = []
		link.client.on_connection = lambda conn, connected: events.append(connected)
		future = client_test.remote_call("Never")
		link.pump_until(lambda: link.server.connection_count == 1 and link.server._connections[next(iter(link.server._connections))].deferred)
		link.server.close()
		link.pump_until(lambda: future.done)
		self.assertEqual(future.error.code, eqrpc.ERROR_DISCONNECTED)
		self.assertEqual(events, [False])
		self.assertFalse(link.client.is_connected)

	def test_remove_protocol_fails_calls(self):
		test = Protocol("test")
		test.add_method("Never", lambda ctx, params, payload: ctx.defer())
		client_test = Protocol("test")
		link = self.link([test], [client_test])

		future = client_test.remote_call("Never")
		link.client.remove_protocol(client_test)
		self.assertEqual(future.error.code, eqrpc.ERROR_CANCELLED)
		self.assertEqual(client_test.remote_call("Never").error.code, eqrpc.ERROR_NOT_CONNECTED)

	def test_connect_failed(self):
		client = Endpoint(Endpoint.CLIENT)
		events = []
		client.on_connection = lambda conn, connected: events.append(connected)
		self.assertTrue(client.connect("127.0.0.1:1"))
		future = client.ping()
		with self.assertRaises(RPCError) as error:
			future.wait(timeout=10)
		self.assertEqual(error.exception.code, eqrpc.ERROR_DISCONNECTED)
		self.assertEqual(events, [])
		client.close()


if __name__ == "__main__":
	unittest.main()
