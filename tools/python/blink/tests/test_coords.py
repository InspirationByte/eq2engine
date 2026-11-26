"""Tests of the engine <-> Blender coordinates (no Blender needed)."""

import math
import random
import unittest

try:
	import numpy
except ImportError:		# Blender has it; the mesh test needs it
	numpy = None

from blink import coords


def _engine_axes(x, y, z):
	"""The drawn axes of an editor object with the angles (radians): the columns of rotateXYZ3."""
	sx, cx = math.sin(x), math.cos(x)
	sy, cy = math.sin(y), math.cos(y)
	sz, cz = math.sin(z), math.cos(z)
	rows = [
		(cy * cz, cy * sz, -sy),
		(sx * sy * cz - cx * sz, sx * sy * sz + cx * cz, sx * cy),
		(cx * sy * cz + sx * sz, cx * sy * sz - sx * cz, cx * cy),
	]
	return tuple(tuple(rows[r][c] for r in range(3)) for c in range(3))


def _engine_point(position, axes, local):
	"""rotateVector(local, q) + t: the local axes scaled by the local coordinates."""
	return tuple(position[i] + sum(local[a] * axes[a][i] for a in range(3)) for i in range(3))


def _det3(rows):
	(a, b, c), (d, e, f), (g, h, i) = (row[:3] for row in rows[:3])
	return a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g)


def _blender_point(rows, local):
	return tuple(sum(rows[i][c] * local[c] for c in range(3)) + rows[i][3] for i in range(3))


class CoordsTests(unittest.TestCase):
	def setUp(self):
		random.seed(7)

	def assertVector(self, a, b, places=5):
		for x, y in zip(a, b):
			self.assertAlmostEqual(x, y, places=places)

	def test_point_round_trip(self):
		v = (1.0, 2.0, 3.0)
		self.assertEqual(coords.to_blender(v), (-1.0, -3.0, 2.0))
		self.assertEqual(coords.to_engine(coords.to_blender(v)), v)

	def test_object_matches_engine_transform(self):
		for _ in range(20):
			axes = _engine_axes(*(random.uniform(-math.pi, math.pi) for _ in range(3)))
			position = tuple(random.uniform(-100, 100) for _ in range(3))
			offset = (0.0, random.uniform(0, 2), 0.0)
			rows = coords.object_to_blender(position, axes, offset)

			# a proper rotation in Blender: the reflection of the coordinates cancels out
			self.assertAlmostEqual(_det3(rows), 1.0, places=5)

			# a model point lands where the editor draws it
			local = tuple(random.uniform(-5, 5) for _ in range(3))
			world = _engine_point(tuple(p + o for p, o in zip(position, offset)), axes, local)
			self.assertVector(_blender_point(rows, coords.to_blender(local)), coords.to_blender(world), places=4)

	def test_object_round_trip(self):
		for _ in range(20):
			axes = _engine_axes(*(random.uniform(-math.pi, math.pi) for _ in range(3)))
			position = tuple(random.uniform(-100, 100) for _ in range(3))
			offset = (0.0, 1.5, 0.0)
			back_position, back_axes = coords.object_to_engine(coords.object_to_blender(position, axes, offset), offset)
			self.assertVector(back_position, position, places=4)
			for a, b in zip(back_axes, axes):
				self.assertVector(a, b)

	def test_scale_is_dropped(self):
		axes = _engine_axes(0.3, -0.2, 1.1)
		rows = coords.object_to_blender((1.0, 2.0, 3.0), axes)
		for r in range(3):
			for c in range(3):
				rows[r][c] *= 2.0
		_, back_axes = coords.object_to_engine(rows)
		for a, b in zip(back_axes, axes):
			self.assertVector(a, b)

	@unittest.skipIf(numpy is None, "numpy is not installed")
	def test_mesh_winding_and_uv(self):
		positions = numpy.array([[0, 0, 0], [1, 0, 0], [0, 0, 1]], dtype=numpy.float32)
		normals = numpy.array([[0, 1, 0]] * 3, dtype=numpy.float32)
		texcoords = numpy.array([[0.0, 0.25], [1.0, 0.25], [0.0, 1.0]], dtype=numpy.float32)
		indices = numpy.array([0, 1, 2], dtype=numpy.int32)

		def facing(p, n, i):
			a, b, c = p[i[0]], p[i[1]], p[i[2]]
			return numpy.dot(numpy.cross(b - a, c - a), n[i[0]])

		out_p, out_n, out_t, out_i = coords.mesh_to_blender(positions, normals, texcoords, indices)
		# the triangle faces the same way relative to its normal as in the engine
		self.assertEqual(numpy.sign(facing(out_p, out_n, out_i)), numpy.sign(facing(positions, normals, indices)))
		self.assertEqual(list(out_i), [0, 2, 1])
		self.assertVector(out_t[:, 1], (0.75, 0.75, 0.0))
		self.assertVector(out_n[0], (0.0, 0.0, 1.0))


if __name__ == "__main__":
	unittest.main()
