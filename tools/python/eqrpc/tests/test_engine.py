"""Interop with the engine RPC endpoint: runs the disabled `RPC_TESTS.DISABLED_ServePython` test of
the engine `network_tests` executable and talks to it over TCP.

Set EQRPC_NETWORK_TESTS to the path of `network_tests` (for example
eq2engine/build/auto_tests/bin/x64/Debug/network_tests.exe); the tests are skipped without it.
"""

import os
import subprocess
import threading
import unittest

from eqrpc import Endpoint, Protocol, PayloadReader, RPCError
import eqrpc

_EXECUTABLE = os.environ.get("EQRPC_NETWORK_TESTS")


@unittest.skipUnless(_EXECUTABLE, "EQRPC_NETWORK_TESTS is not set")
class EngineTests(unittest.TestCase):
	@classmethod
	def setUpClass(cls):
		cls.process = subprocess.Popen(
			[_EXECUTABLE, "--gtest_also_run_disabled_tests", "--gtest_filter=RPC_TESTS.DISABLED_ServePython"],
			cwd=os.path.dirname(os.path.abspath(_EXECUTABLE)),
			stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace")

		cls.output = []
		port = None
		for line in cls.process.stdout:
			cls.output.append(line)
			if line.startswith("EQRPC_PORT "):
				port = int(line.split()[1])
				break
		if port is None:
			cls.process.wait()
			raise RuntimeError("the engine server did not start:\n" + "".join(cls.output))

		# keep reading the output so the process never blocks on a full pipe
		cls.reader = threading.Thread(target=lambda: cls.output.extend(cls.process.stdout), daemon=True)
		cls.reader.start()

		cls.endpoint = Endpoint(Endpoint.CLIENT)
		cls.protocol = Protocol("interop", 3)
		cls.protocol.add_method("Twice", lambda ctx, params, payload: {"value": params.value("value") * 2})
		cls.endpoint.add_protocol(cls.protocol)
		assert cls.endpoint.connect("127.0.0.1:%d" % port)

	@classmethod
	def tearDownClass(cls):
		cls.protocol.remote_notify("Quit")
		cls.endpoint.ping().wait(timeout=5)
		cls.endpoint.close()
		try:
			cls.process.wait(timeout=10)
		except subprocess.TimeoutExpired:
			cls.process.kill()
		cls.reader.join(timeout=5)
		output = "".join(cls.output)
		assert cls.process.returncode == 0, "the engine server failed:\n" + output

	def call(self, method, params=None, payload=None):
		return self.protocol.remote_call(method, params, payload).wait(timeout=10)

	def test_hello(self):
		self.endpoint.ping().wait(timeout=10)
		self.assertEqual(self.endpoint.peer_protocol_version("interop"), 3)
		self.assertEqual(self.endpoint.peer_protocol_version("rpc"), 1)

	def test_engine_section(self):
		response = self.call("Engine")
		result = response.result
		self.assertEqual(result.value("int"), 42)
		self.assertEqual(result.value("float"), 0.5)
		self.assertIs(result.value("bool"), True)
		self.assertEqual(result.value("string"), "Привет, \"world\"")
		self.assertEqual(result.find("position").values, [1.0, -2.0, 3.5])

		view = result.to_json()
		self.assertEqual(view["model"], [["a"], ["b"]])
		self.assertIsNone(view["empty"])
		self.assertEqual(view["straight"], {"_v": ["main"], "lanes": 2})
		self.assertEqual(view["nested"], {"deeper": {"depth": 2}})
		self.assertEqual(view["value"], {"_kv": {"x": 1}})

		reader = PayloadReader(response.payload)
		positions = reader.read_array("f", 3)
		self.assertEqual(list(positions), [0.0, 0.0, 0.0, 1.0, 2.0, 3.0])
		self.assertEqual(list(reader.read_array("i")), [0, 1, -1])
		self.assertEqual(reader.remaining, 0)

	def test_engine_reads_python(self):
		params = {
			"int": 7, "float": 2.5, "bool": True, "string": "текст",
			"mixed": [1, 2.5, 3],
			"straight": {"_v": ["main"], "lanes": 2},
			"nested": {"deeper": {"depth": 3}},
			"model": [["a"], ["b"], ["c"]],
		}
		result = self.call("Check", params, payload=b"12345").result
		self.assertEqual(result.value("int"), 7)
		self.assertEqual(result.value("float"), 2.5)
		self.assertIs(result.value("bool"), True)
		self.assertEqual(result.value("string"), "текст")
		self.assertEqual(result.find("mixed").values, [1.0, 2.5, 3.0])
		self.assertEqual(result.value("straight"), "main")
		self.assertEqual(result.value("depth"), 3)
		self.assertEqual(result.value("models"), 3)
		self.assertEqual(result.value("payload"), 5)

	def test_echo_round_trip(self):
		section = eqrpc.Section()
		section.add("id", -5)
		section.add("v", 1.25, -2.0)
		section.add("s", "x", "y")
		section.add("b", False)
		section.add("rep", 1).add("k")
		section.add("rep", 2)
		data = bytes(range(256)) * 700		# several chunks both ways

		response = self.call("Echo", section, payload=data)
		echoed = response.result
		self.assertEqual(echoed.name, "result")
		self.assertEqual(echoed.keys, section.keys)
		self.assertEqual(bytes(response.payload), data)

		empty = self.call("Echo", None, payload=b"")
		self.assertEqual(len(empty.payload), 0)

	def test_engine_calls_python(self):
		self.assertEqual(self.call("CallBack", {"value": 21}).result.value("value"), 42)

	def test_errors(self):
		with self.assertRaises(RPCError) as error:
			self.call("Missing")
		self.assertEqual(error.exception.code, eqrpc.ERROR_UNKNOWN_METHOD)
		self.assertIn("interop.Missing", error.exception.message)


if __name__ == "__main__":
	unittest.main()
