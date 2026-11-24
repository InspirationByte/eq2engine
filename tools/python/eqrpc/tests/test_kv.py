import json
import struct
import unittest

from eqrpc import kv
from eqrpc.kv import Section, KVError


def _header(section_type, key_count, value_count, name):
	data = name.encode("utf-8")
	return b"BKVS" + struct.pack("<iiHH", section_type, key_count, value_count, len(data)) + data


def _sample():
	root = Section()
	root.add("id", 9)
	root.add("name", "Привет")
	root.add("position", 1.5, -2.0, 3.25)
	root.add("flags", True, False)
	root.add("empty")
	sub = root.add("straight", "main")
	sub.add("lanes", 2)
	root.add("straight", "side").add("lanes", 1)
	inner = Section()
	inner.add("x", 1)
	root.add("value", inner, "after")
	return root


class BinaryTests(unittest.TestCase):
	def test_round_trip(self):
		root = _sample()
		self.assertEqual(Section.from_binary(root.to_binary()), root)

	def test_layout(self):
		# the bytes KeyValues::WriteBinary writes for: id 9; name "ab"; speed 0.5; on 1(bool) { }
		root = Section()
		root.add("id", 9)
		root.add("name", "ab")
		root.add("speed", 0.5)
		root.add("on", True)

		expected = _header(kv.STRING, 4, 0, "")
		expected += _header(kv.INT, 0, 1, "id") + struct.pack("<ii", kv.INT, 9)
		expected += _header(kv.STRING, 0, 1, "name") + struct.pack("<ii", kv.STRING, 2) + b"ab"
		expected += _header(kv.FLOAT, 0, 1, "speed") + struct.pack("<if", kv.FLOAT, 0.5)
		expected += _header(kv.BOOL, 0, 1, "on") + struct.pack("<ii", kv.BOOL, 1)
		self.assertEqual(root.to_binary(), expected)

	def test_section_value_layout(self):
		root = Section()
		root.add("v", Section("0"))
		expected = _header(kv.STRING, 1, 0, "")
		expected += _header(kv.SECTION, 0, 1, "v") + struct.pack("<ii", kv.SECTION, 0) + _header(kv.STRING, 0, 0, "0")
		self.assertEqual(root.to_binary(), expected)

	def test_truncated(self):
		data = _sample().to_binary()
		for size in range(len(data)):
			with self.assertRaises(KVError, msg="truncated at %d" % size):
				Section.from_binary(data[:size])

	def test_corrupt(self):
		data = bytearray(_sample().to_binary())
		data[0] = ord("X")
		with self.assertRaises(KVError):
			Section.from_binary(bytes(data))

		data = bytearray(_sample().to_binary())
		struct.pack_into("<i", data, 8, -1)		# key count of the root
		with self.assertRaises(KVError):
			Section.from_binary(bytes(data))

		data = bytearray(_sample().to_binary())
		struct.pack_into("<i", data, 16 + 16 + 2, 99)	# value type of "id"
		with self.assertRaises(KVError):
			Section.from_binary(bytes(data))

	def test_depth(self):
		root = Section()
		key = root
		for _ in range(70):
			key = key.add("k")
		with self.assertRaises(KVError):
			Section.from_binary(root.to_binary())

	def test_int_range(self):
		with self.assertRaises(KVError):
			Section("x", 1 << 31).to_binary()

	def test_float32(self):
		value = Section.from_binary(Section("x", 0.1).to_binary()).values[0]
		self.assertAlmostEqual(value, 0.1, places=6)
		self.assertIsInstance(value, float)

	def test_mixed_numbers_section_type(self):
		# the engine reads the values with the section type: int and float numbers make a FLOAT section
		self.assertEqual(struct.unpack_from("<i", Section("x", 1, 2.5).to_binary(), 4)[0], kv.FLOAT)
		self.assertEqual(struct.unpack_from("<i", Section("x", 1, 2).to_binary(), 4)[0], kv.INT)
		self.assertEqual(struct.unpack_from("<i", Section("x", "a", 1).to_binary(), 4)[0], kv.STRING)

	def test_bool_type(self):
		section = Section.from_binary(Section("x", True, 1).to_binary())
		self.assertIs(section.values[0], True)
		self.assertEqual(type(section.values[1]), int)


class AccessTests(unittest.TestCase):
	def test_find_value_set(self):
		root = _sample()
		self.assertEqual(root.value("id"), 9)
		self.assertEqual(root.value("position", index=2), 3.25)
		self.assertEqual(root.value("missing", 5), 5)
		self.assertEqual(root.value("empty", "none"), "none")
		self.assertEqual(len(root.find_all("straight")), 2)

		root.set("id", 10)
		self.assertEqual(root.value("id"), 10)
		self.assertEqual(len(root.find_all("id")), 1)
		root.set("new", "a")
		self.assertEqual(root.value("new"), "a")


class JsonViewTests(unittest.TestCase):
	def check(self, text, obj):
		"""The section of `text` (built by the test) has the view `obj`, and the view builds it back."""
		section = text
		self.assertEqual(section.to_json(), obj)
		self.assertEqual(Section.from_json(obj), section)
		json.dumps(obj)

	def test_one_value(self):
		root = Section()
		root.add("speed", 50)
		self.check(root, {"speed": 50})

	def test_several_values(self):
		root = Section()
		root.add("position", 1, 2, 3)
		self.check(root, {"position": [1, 2, 3]})

	def test_types(self):
		root = Section()
		root.add("i", 1)
		root.add("f", 1.5)
		root.add("b", True)
		root.add("s", "text")
		self.check(root, {"i": 1, "f": 1.5, "b": True, "s": "text"})
		self.assertIs(Section.from_json({"b": False}).value("b"), False)
		self.assertIsInstance(Section.from_json({"f": 2.0}).value("f"), float)

	def test_empty_key(self):
		root = Section()
		root.add("empty")
		self.check(root, {"empty": None})

	def test_section_with_keys(self):
		root = Section()
		root.add("params").add("def", 12)
		self.check(root, {"params": {"def": 12}})

	def test_section_with_values_and_keys(self):
		root = Section()
		root.add("straight", "main").add("lanes", 2)
		self.check(root, {"straight": {"_v": ["main"], "lanes": 2}})

	def test_section_value(self):
		root = Section()
		inner = Section()
		inner.add("x", 1)
		root.add("value", inner, "after")
		self.check(root, {"value": [{"_kv": {"x": 1}}, "after"]})

		root = Section()
		root.add("value", inner)
		self.check(root, {"value": {"_kv": {"x": 1}}})

	def test_repeated_sections(self):
		root = Section()
		root.add("straight", "a").add("lanes", 1)
		root.add("straight", "b").add("lanes", 2)
		self.check(root, {"straight": [{"_v": ["a"], "lanes": 1}, {"_v": ["b"], "lanes": 2}]})

	def test_repeated_values(self):
		root = Section()
		root.add("model", "a")
		root.add("model", "b")
		self.check(root, {"model": [["a"], ["b"]]})

	def test_root_values(self):
		root = Section("", 1, 2)
		self.check(root, {"_v": [1, 2]})

	def test_bad_values(self):
		with self.assertRaises(KVError):
			Section.from_json({"x": [[1], 2]})
		with self.assertRaises(KVError):
			Section.from_json([1, 2])
		with self.assertRaises(KVError):
			Section.from_json({"x": object()})


if __name__ == "__main__":
	unittest.main()
