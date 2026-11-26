"""blink.mesh: write_mesh / read_mesh and merge_meshes (numpy, no Blender)."""

import unittest

try:
	import numpy
except ImportError:
	numpy = None

from blink import Mesh, merge_meshes, read_mesh, write_mesh
from eqrpc import Section


def _section(params):
	section = Section()
	for batch in params.get("batch", []):
		key = section.add("batch")
		for name, value in batch.items():
			key.set(name, value)
	return section


@unittest.skipIf(numpy is None, "numpy is not installed")
class MeshTests(unittest.TestCase):
	def _triangle(self, x, material, colors=None):
		positions = numpy.array([[x, 0, 0], [x + 1, 0, 0], [x, 1, 0]], dtype=numpy.float32)
		normals = numpy.array([[0, 0, 1]] * 3, dtype=numpy.float32)
		texcoords = numpy.array([[0, 0], [1, 0], [0, 1]], dtype=numpy.float32)
		return Mesh(positions, normals, texcoords, numpy.array([0, 1, 2], dtype=numpy.int32), [(material, 0, 3)], colors)

	def test_round_trip(self):
		colors = numpy.array([[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]], dtype=numpy.uint8)
		mesh = self._triangle(0.0, "a", colors)
		params, payload = write_mesh(mesh)
		self.assertEqual(params["batch"], [{"material": "a", "firstIndex": 0, "indexCount": 3}])

		back = read_mesh(_section(params), payload)
		numpy.testing.assert_allclose(back.positions, mesh.positions, atol=1e-6)
		numpy.testing.assert_allclose(back.texcoords, mesh.texcoords, atol=1e-6)
		numpy.testing.assert_array_equal(back.indices, mesh.indices)
		numpy.testing.assert_array_equal(back.colors, colors)
		self.assertEqual(back.batches, [("a", 0, 3)])

	def test_empty_has_no_batch_key(self):
		empty = Mesh(numpy.zeros((0, 3), numpy.float32), numpy.zeros((0, 3), numpy.float32),
			numpy.zeros((0, 2), numpy.float32), numpy.zeros(0, numpy.int32), [])
		params, _ = write_mesh(empty)
		self.assertNotIn("batch", params)

	def test_merge(self):
		merged = merge_meshes([self._triangle(0.0, "a"), self._triangle(5.0, "b"),
			self._triangle(10.0, "a", numpy.full((3, 4), 9, numpy.uint8))])
		self.assertEqual(merged.batches, [("a", 0, 6), ("b", 6, 3)])
		self.assertEqual(len(merged.positions), 9)
		numpy.testing.assert_array_equal(merged.indices, [0, 1, 2, 6, 7, 8, 3, 4, 5])
		self.assertEqual(merged.colors.shape, (9, 4))
		self.assertEqual(int(merged.colors[0, 0]), 0)
		self.assertEqual(int(merged.colors[6, 0]), 9)


if __name__ == "__main__":
	unittest.main()
