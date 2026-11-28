"""KeyValues sections and their binary form (BKVS), the wire format of the engine RPC messages.

`Section` mirrors the engine `KVSection`: a name, values and nested keys. Values are `str`, `int`,
`float`, `bool` or a nested `Section` (a section value, KVPAIR_SECTION). Numbers are int32 and
float32 on the wire, as `KVPairValue` keeps them.

The JSON view (`to_json` / `from_json`) maps sections to `dict` / `list` values for simple
parameters and for logs; see "JSON view" in eq2engine/docs/network_rpc_overview.md.
"""

import struct

# EKVPairType
STRING = 0
INT = 1
FLOAT = 2
BOOL = 3
SECTION = 4
_TYPE_COUNT = 5

_IDENT = struct.unpack("<i", b"BKVS")[0]
_SECTION_HEADER = struct.Struct("<iiiHH")	# ident, type, keyCount, valueCount, nameLen
_VALUE_HEADER = struct.Struct("<ii")		# type, int value or string length
_FLOAT = struct.Struct("<f")
_FLOAT_VALUE = struct.Struct("<if")		# type, float value
_MAX_DEPTH = 64
_INT_MIN = -(1 << 31)
_INT_MAX = (1 << 31) - 1


class KVError(ValueError):
	"""Broken or truncated binary KeyValues, or a value that the binary form can't keep."""


def value_type(value):
	"""The EKVPairType of a Python value."""
	# bool goes first, it is a subclass of int
	if isinstance(value, bool):
		return BOOL
	if isinstance(value, int):
		return INT
	if isinstance(value, float):
		return FLOAT
	if isinstance(value, str):
		return STRING
	if isinstance(value, Section):
		return SECTION
	raise KVError("unsupported KeyValues value type %s" % type(value).__name__)


class Section:
	"""A KeyValues section: `name`, `values` (a list) and `keys` (a list of sections)."""

	__slots__ = ("name", "values", "keys")

	def __init__(self, name="", *values):
		self.name = name
		self.values = list(values)
		self.keys = []

	# ---- building and searching

	def add(self, name, *values):
		"""Adds a key (repeated names are kept), returns it."""
		key = Section(name, *values)
		self.keys.append(key)
		return key

	def set(self, name, *values):
		"""Replaces the values of the first key with the name or adds the key, returns it."""
		key = self.find(name)
		if key is None:
			return self.add(name, *values)
		key.values = list(values)
		return key

	def find(self, name):
		"""The first key with the name, or None."""
		for key in self.keys:
			if key.name == name:
				return key
		return None

	def find_all(self, name):
		"""All keys with the name, in their order."""
		return [key for key in self.keys if key.name == name]

	def value(self, name, default=None, index=0):
		"""A value of the first key with the name, `default` when there is no key or value."""
		key = self.find(name)
		if key is None or index >= len(key.values):
			return default
		return key.values[index]

	def __eq__(self, other):
		if not isinstance(other, Section):
			return NotImplemented
		return self.name == other.name and self.values == other.values and self.keys == other.keys

	def __repr__(self):
		return "Section(%r, %r)" % (self.name, self.to_json())

	# ---- binary form

	def to_binary(self):
		"""The binary KeyValues bytes of the section, equal to `KeyValues::WriteBinary`."""
		out = bytearray()
		_write_section(out, self)
		return bytes(out)

	@staticmethod
	def from_binary(data):
		"""Parses binary KeyValues, raises KVError on broken or truncated data."""
		view = memoryview(data).cast("B")
		return _read_section(view, 0, 0)[0]

	# ---- JSON view

	def to_json(self):
		"""The JSON view: an object of the keys (with `_v` when the section has values)."""
		return _section_object(self)

	@staticmethod
	def from_json(obj, name=""):
		"""Builds a section from the JSON view: `obj` is an object (`dict`) of keys."""
		if isinstance(obj, Section):
			return obj
		section = Section(name)
		if obj is None:
			return section
		if not isinstance(obj, dict):
			raise KVError("the JSON view of a section is an object, got %s" % type(obj).__name__)
		_fill_from_object(section, obj)
		return section


# ---------------------------------------------------------------------------------------------
# binary

def _check_int(value):
	if value < _INT_MIN or value > _INT_MAX:
		raise KVError("integer %d is out of the int32 range" % value)


def _section_type(values):
	"""The engine reader gives the values the type of their section: it is the type of the first
	value, or FLOAT for numbers with a float among them, so `1, 2.5` is not read as `1, 2`."""
	if not values:
		return STRING
	first = value_type(values[0])
	if first == INT and any(value_type(value) == FLOAT for value in values):
		return FLOAT
	return first


def _write_section(out, section):
	name = section.name.encode("utf-8")
	if len(name) > 0xFFFF:
		raise KVError("section name is longer than 65535 bytes")
	if len(section.values) > 0xFFFF:
		raise KVError("section '%s' has more than 65535 values, send them as payload" % section.name)

	section_type = _section_type(section.values)
	out += _SECTION_HEADER.pack(_IDENT, section_type, len(section.keys), len(section.values), len(name))
	out += name

	for value in section.values:
		vtype = value_type(value)
		if vtype == STRING:
			data = value.encode("utf-8")
			out += _VALUE_HEADER.pack(STRING, len(data))
			out += data
		elif vtype == INT:
			_check_int(value)
			out += _VALUE_HEADER.pack(INT, value)
		elif vtype == FLOAT:
			out += _FLOAT_VALUE.pack(FLOAT, value)
		elif vtype == BOOL:
			out += _VALUE_HEADER.pack(BOOL, 1 if value else 0)
		else:
			out += _VALUE_HEADER.pack(SECTION, 0)
			_write_section(out, value)

	for key in section.keys:
		_write_section(out, key)


def _need(view, offset, size):
	if size < 0 or len(view) - offset < size:
		raise KVError("binary KeyValues are truncated or broken")


def _read_section(view, offset, depth):
	if depth > _MAX_DEPTH:
		raise KVError("binary KeyValues are nested too deep")

	_need(view, offset, _SECTION_HEADER.size)
	ident, section_type, key_count, value_count, name_len = _SECTION_HEADER.unpack_from(view, offset)
	offset += _SECTION_HEADER.size

	if ident != _IDENT or key_count < 0 or section_type < 0 or section_type >= _TYPE_COUNT:
		raise KVError("binary KeyValues are broken (bad section header)")

	_need(view, offset, name_len)
	section = Section(bytes(view[offset:offset + name_len]).decode("utf-8", "replace"))
	offset += name_len

	for _ in range(value_count):
		_need(view, offset, _VALUE_HEADER.size)
		vtype, ivalue = _VALUE_HEADER.unpack_from(view, offset)
		offset += _VALUE_HEADER.size

		if vtype == STRING:
			_need(view, offset, ivalue)
			section.values.append(bytes(view[offset:offset + ivalue]).decode("utf-8", "replace"))
			offset += ivalue
		elif vtype == INT:
			section.values.append(ivalue)
		elif vtype == FLOAT:
			section.values.append(_FLOAT.unpack_from(view, offset - 4)[0])
		elif vtype == BOOL:
			# the engine writes a C++ bool into the low byte of the 4-byte field
			section.values.append(view[offset - 4] != 0)
		elif vtype == SECTION:
			value, offset = _read_section(view, offset, depth + 1)
			section.values.append(value)
		else:
			raise KVError("binary KeyValues are broken (bad value type %d)" % vtype)

	for _ in range(key_count):
		key, offset = _read_section(view, offset, depth + 1)
		section.keys.append(key)

	return section, offset


# ---------------------------------------------------------------------------------------------
# JSON view

def _json_value(value):
	return {"_kv": _section_object(value)} if isinstance(value, Section) else value


def _section_object(section):
	obj = {}
	if section.values:
		obj["_v"] = [_json_value(value) for value in section.values]

	# keys are grouped by name in the order of their first appearance
	groups = {}
	for key in section.keys:
		groups.setdefault(key.name, []).append(key)

	for name, keys in groups.items():
		if len(keys) == 1:
			obj[name] = _key_json(keys[0])
		else:
			obj[name] = [_repeated_key_json(key) for key in keys]
	return obj


def _key_json(key):
	if key.keys:
		return _section_object(key)
	if not key.values:
		return None
	if len(key.values) == 1:
		return _json_value(key.values[0])
	return [_json_value(value) for value in key.values]


def _repeated_key_json(key):
	# an element of a repeated key is always an object or an array
	if key.keys:
		return _section_object(key)
	return [_json_value(value) for value in key.values]


def _is_kv_value(obj):
	return isinstance(obj, dict) and len(obj) == 1 and "_kv" in obj


def _from_json_value(obj):
	if _is_kv_value(obj):
		return Section.from_json(obj["_kv"])
	if isinstance(obj, (dict, list)) or obj is None:
		raise KVError("a KeyValues value is a number, a bool, a string or {\"_kv\": {...}}")
	value_type(obj)
	return obj


def _fill_from_object(section, obj):
	for name, item in obj.items():
		if name == "_v":
			section.values.extend(_from_json_value(value) for value in item)
			continue

		if isinstance(item, list) and item and all(isinstance(e, list) or (isinstance(e, dict) and not _is_kv_value(e)) for e in item):
			# repeated key
			for element in item:
				_add_key_from_json(section, name, element)
		else:
			_add_key_from_json(section, name, item)


def _add_key_from_json(section, name, item):
	key = section.add(name)
	if item is None:
		return
	if isinstance(item, list):
		key.values.extend(_from_json_value(value) for value in item)
	elif isinstance(item, dict) and not _is_kv_value(item):
		_fill_from_object(key, item)
	else:
		key.values.append(_from_json_value(item))
