"""BLINK mesh: the triangles with material batches of the engine `BlinkMesh` (numpy).

The KeyValues result has `batch` sections (`material`, `firstIndex`, `indexCount`), the payload the
vertex arrays: positions, normals, texcoords, indices, each an int32 count and the items.
"""

from . import coords
from .rpc import PayloadReader


class Mesh:
	"""Arrays in Blender coordinates (see `coords.mesh_to_blender`) and (material, first, count) batches."""
	__slots__ = ("positions", "normals", "texcoords", "indices", "batches")

	def __init__(self, positions, normals, texcoords, indices, batches):
		self.positions = positions
		self.normals = normals
		self.texcoords = texcoords
		self.indices = indices
		self.batches = batches


def read_mesh(result, payload):
	"""The Mesh of a reply written by `BlinkMesh::Write`."""
	import numpy

	reader = PayloadReader(payload)
	positions = reader.read_array(numpy.float32, 3)
	normals = reader.read_array(numpy.float32, 3)
	texcoords = reader.read_array(numpy.float32, 2)
	indices = reader.read_array(numpy.int32)
	positions, normals, texcoords, indices = coords.mesh_to_blender(positions, normals, texcoords, indices)

	batches = [
		(batch.value("material", "error"), batch.value("firstIndex", 0), batch.value("indexCount", 0))
		for batch in result.find_all("batch")
	]
	return Mesh(positions, normals, texcoords, indices, batches)
