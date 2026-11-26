"""Blender meshes of the BLINK meshes."""

import bpy
import numpy

from .. import coords


MATERIAL_KEY = "eq_material"	# the custom property with the full engine material name

_materials = {}		# engine name -> Blender material name (a cache, checked on use)


def material(name, on_created=None):
	"""The Blender material of an engine material, created when missing.

	Found by its `eq_material` property, not by the Blender name: Blender names are limited to
	63 bytes and get a .001 suffix when taken, so two engine materials could share one, or one could
	get a second copy. on_created(material) runs for a new one (to fill it).
	"""
	cached = bpy.data.materials.get(_materials.get(name, ""))
	if cached is not None and cached.get(MATERIAL_KEY) == name:
		return cached

	for candidate in bpy.data.materials:
		if candidate.get(MATERIAL_KEY) == name:
			_materials[name] = candidate.name
			return candidate

	result = bpy.data.materials.new(name)
	result[MATERIAL_KEY] = name
	_materials[name] = result.name
	if on_created:
		on_created(result)
	return result


def build_mesh(mesh, data, on_material_created=None):
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
			mesh.materials.append(material(name, on_material_created))
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


def fill_material(mat, texture_path="", color=(0.8, 0.8, 0.8, 1.0)):
	"""A Principled BSDF with the base texture (loaded once per file: check_existing).

	The image alpha is not used (alpha mode None): the engine textures keep other data in it.
	"""
	mat.use_nodes = True
	mat.diffuse_color = color
	nodes = mat.node_tree.nodes
	bsdf = next((node for node in nodes if node.type == "BSDF_PRINCIPLED"), None)
	if bsdf is None or not texture_path:
		return

	try:
		image = bpy.data.images.load(texture_path, check_existing=True)
	except RuntimeError:		# not readable by Blender
		return
	image.alpha_mode = "NONE"

	texture = nodes.new("ShaderNodeTexImage")
	texture.image = image
	texture.location = (bsdf.location[0] - 300, bsdf.location[1])
	mat.node_tree.links.new(texture.outputs["Color"], bsdf.inputs["Base Color"])


def show_object_colors():
	"""The Solid shading of the 3D views shows the object colors (obj.color)."""
	for window in bpy.context.window_manager.windows:
		for area in window.screen.areas:
			if area.type != "VIEW_3D":
				continue
			for space in area.spaces:
				if space.type == "VIEW_3D":
					space.shading.color_type = "OBJECT"
