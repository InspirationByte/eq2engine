"""Blender meshes of the BLINK meshes."""

import bpy
import numpy

from .. import coords


def material(name):
	"""The Blender material named by an engine material, created when missing."""
	result = bpy.data.materials.get(name)
	if result is None:
		result = bpy.data.materials.new(name)
	return result


def build_mesh(mesh, data):
	"""Fills a bpy mesh from a `blink.Mesh`: a material slot per batch material, the UVs, the normals."""
	num_vertices = len(data.positions)
	num_loops = len(data.indices)
	num_triangles = num_loops // 3

	mesh.clear_geometry()
	mesh.materials.clear()

	mesh.vertices.add(num_vertices)
	mesh.vertices.foreach_set("co", data.positions.ravel())
	mesh.loops.add(num_loops)
	mesh.loops.foreach_set("vertex_index", data.indices)
	mesh.polygons.add(num_triangles)
	mesh.polygons.foreach_set("loop_start", numpy.arange(0, num_loops, 3, dtype=numpy.int32))

	material_indices = numpy.zeros(num_triangles, dtype=numpy.int32)
	slots = {}
	for name, first_index, index_count in data.batches:
		if name not in slots:
			slots[name] = len(mesh.materials)
			mesh.materials.append(material(name))
		first = first_index // 3
		material_indices[first:first + index_count // 3] = slots[name]
	mesh.polygons.foreach_set("material_index", material_indices)

	uv_layer = mesh.uv_layers.new(name="UVMap")
	uv_layer.data.foreach_set("uv", data.texcoords[data.indices].ravel())

	mesh.update(calc_edges=True)
	mesh.normals_split_custom_set_from_vertices(data.normals)


def box_mesh(mesh, bbox_min, bbox_max):
	"""A box of engine bounds (for the things without a mesh)."""
	x = (bbox_min[0], bbox_max[0])
	y = (bbox_min[1], bbox_max[1])
	z = (bbox_min[2], bbox_max[2])
	corners = [coords.to_blender((x[i & 1], y[(i >> 1) & 1], z[(i >> 2) & 1])) for i in range(8)]
	faces = [(0, 2, 3, 1), (4, 5, 7, 6), (0, 1, 5, 4), (2, 6, 7, 3), (0, 4, 6, 2), (1, 3, 7, 5)]
	mesh.clear_geometry()
	mesh.from_pydata(corners, [], faces)
	mesh.update()
