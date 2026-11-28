"""BLINK mesh: the triangles with material batches of the engine `BlinkMesh` (numpy).

The KeyValues result has `batch` sections (`material`, `firstIndex`, `indexCount`), the payload the
vertex arrays: positions, normals, texcoords, indices, colors (ubyte4 per vertex or none), each
an int32 count and the items. `read_mesh` reads a received one, `write_mesh` makes one to send.
"""

from . import coords
from .rpc import PayloadReader, PayloadWriter


class Mesh:
	"""Arrays in Blender coordinates (see `coords.mesh_to_blender`) and (material, first, count) batches.

	colors: None or a (n, 4) uint8 array, a color of each vertex (the application gives it a meaning).
	"""
	__slots__ = ("positions", "normals", "texcoords", "indices", "batches", "colors")

	def __init__(self, positions, normals, texcoords, indices, batches, colors=None):
		self.positions = positions
		self.normals = normals
		self.texcoords = texcoords
		self.indices = indices
		self.batches = batches
		self.colors = colors


def read_mesh(result, payload):
	"""The Mesh of a reply written by `BlinkMesh::Write`."""
	import numpy

	reader = PayloadReader(payload)
	positions = reader.read_array(numpy.float32, 3)
	normals = reader.read_array(numpy.float32, 3)
	texcoords = reader.read_array(numpy.float32, 2)
	indices = reader.read_array(numpy.int32)
	colors = reader.read_array(numpy.uint8, 4) if reader.remaining else None
	if colors is not None and len(colors) != len(positions):
		colors = None
	positions, normals, texcoords, indices = coords.mesh_to_blender(positions, normals, texcoords, indices)

	batches = [
		(batch.value("material", "error"), batch.value("firstIndex", 0), batch.value("indexCount", 0))
		for batch in result.find_all("batch")
	]
	return Mesh(positions, normals, texcoords, indices, batches, colors)


def write_mesh(mesh):
	"""The params (`batch` sections) and the payload bytes of a Mesh in Blender coordinates, for
	`BlinkMesh::Read`; the batches must follow each other and cover the indices."""
	import numpy

	positions, normals, texcoords, indices = coords.mesh_to_engine(mesh.positions, mesh.normals, mesh.texcoords, mesh.indices)
	writer = PayloadWriter()
	writer.write_array(positions)
	writer.write_array(normals)
	writer.write_array(texcoords)
	writer.write_array(numpy.ascontiguousarray(indices, dtype=numpy.int32))
	colors = mesh.colors if mesh.colors is not None else numpy.zeros((0, 4), dtype=numpy.uint8)
	writer.write_array(numpy.ascontiguousarray(colors, dtype=numpy.uint8).reshape(-1, 4))

	params = {}
	if mesh.batches:		# an empty list would be one empty key
		params["batch"] = [{"material": name, "firstIndex": int(first), "indexCount": int(count)} for name, first, count in mesh.batches]
	return params, bytes(writer.data)


def merge_meshes(meshes):
	"""One Mesh of several: the vertices one after another, a batch per material name (the triangles
	of that material of each mesh in turn); colors are zeros for the meshes without them."""
	import numpy

	meshes = list(meshes)
	bases = numpy.cumsum([0] + [len(mesh.positions) for mesh in meshes])
	with_colors = any(mesh.colors is not None for mesh in meshes)

	names = []
	for mesh in meshes:
		for name, _, _ in mesh.batches:
			if name not in names:
				names.append(name)

	indices = []
	batches = []
	first = 0
	for name in names:
		count = 0
		for mesh, base in zip(meshes, bases):
			for batch_name, batch_first, batch_count in mesh.batches:
				if batch_name == name:
					indices.append(numpy.asarray(mesh.indices[batch_first:batch_first + batch_count], dtype=numpy.int32) + int(base))
					count += batch_count
		batches.append((name, first, count))
		first += count

	def stack(arrays, components, dtype):
		return numpy.concatenate(arrays).astype(dtype) if arrays else numpy.zeros((0, components), dtype)

	colors = None
	if with_colors:
		colors = stack([mesh.colors if mesh.colors is not None else numpy.zeros((len(mesh.positions), 4), numpy.uint8)
			for mesh in meshes], 4, numpy.uint8)

	return Mesh(stack([mesh.positions for mesh in meshes], 3, numpy.float32), stack([mesh.normals for mesh in meshes], 3, numpy.float32),
		stack([mesh.texcoords for mesh in meshes], 2, numpy.float32),
		numpy.concatenate(indices).astype(numpy.int32) if indices else numpy.zeros(0, numpy.int32), batches, colors)
