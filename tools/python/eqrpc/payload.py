"""Payload layout helpers: the engine `WriteObj` / `ReadObj` layout (little-endian).

An engine array (`ArrayBase` `VSWrite` / `VSRead`) is an `int32` element count and the elements.
`read_array` and `write_array` take a struct format character ("f", "i", "I", "H", ...) or a numpy
dtype; with numpy the read array is a view of the payload, without a copy.
"""

import struct

_INT32 = struct.Struct("<i")


def _itemsize(dtype):
	if isinstance(dtype, str):
		return struct.calcsize("<" + dtype)
	import numpy
	return numpy.dtype(dtype).itemsize


class PayloadReader:
	"""Reads a received payload (`memoryview`) from its start."""

	def __init__(self, data):
		self.view = memoryview(data).cast("B")
		self.offset = 0

	@property
	def remaining(self):
		return len(self.view) - self.offset

	def _take(self, size):
		if size < 0 or size > self.remaining:
			raise ValueError("payload is shorter than its layout (%d bytes needed, %d left)" % (size, self.remaining))
		start = self.offset
		self.offset += size
		return self.view[start:self.offset]

	def read(self, fmt):
		"""Values of a struct format (without the byte order, little-endian is used)."""
		layout = struct.Struct("<" + fmt)
		return layout.unpack(self._take(layout.size))

	def read_int32(self):
		return _INT32.unpack(self._take(4))[0]

	def read_bytes(self, size):
		"""A `memoryview` of the next bytes."""
		return self._take(size)

	def read_array(self, dtype, components=1):
		"""An engine array: the count, then count * components elements of the type.

		With a format character the result is a flat `memoryview` cast to it, with a numpy dtype a
		numpy array view, of shape (count, components) when components > 1.
		"""
		count = self.read_int32()
		if count < 0:
			raise ValueError("payload array has a negative count")
		data = self._take(count * components * _itemsize(dtype))

		if isinstance(dtype, str):
			return data.cast(dtype)

		import numpy
		array = numpy.frombuffer(data, dtype=numpy.dtype(dtype).newbyteorder("<"))
		return array.reshape(count, components) if components > 1 else array


class PayloadWriter:
	"""Builds a payload; `data` is the `bytearray` to send."""

	def __init__(self):
		self.data = bytearray()

	def write(self, fmt, *values):
		"""Values of a struct format (without the byte order, little-endian is used)."""
		self.data += struct.pack("<" + fmt, *values)

	def write_int32(self, value):
		self.data += _INT32.pack(value)

	def write_bytes(self, data):
		self.data += memoryview(data).cast("B")

	def write_array(self, array, components=1):
		"""An engine array of a numpy array or a buffer (`array.array`, `memoryview`).

		The count is the number of elements over `components`: a numpy array of shape (n, 3) or
		a flat buffer of 3 * n floats with components=3 are both n Vector3D.
		"""
		if hasattr(array, "dtype") and hasattr(array, "tobytes"):
			import numpy
			array = numpy.ascontiguousarray(array, dtype=array.dtype.newbyteorder("<"))
			if array.ndim > 1:
				components = 1
			count = len(array) // components
			data = array.tobytes()
		else:
			view = memoryview(array)
			count = len(view) // components
			data = view.cast("B")

		self.write_int32(count)
		self.data += data
