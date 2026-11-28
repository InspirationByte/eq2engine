"""Blender meshes of the BLINK meshes."""

import hashlib

import bpy
import numpy

from .. import coords
from ..mesh import Mesh


MATERIAL_KEY = "eq_material"	# the custom property with the full engine material name
COLOR_ATTRIBUTE = "Color"	# the byte color corner attribute with the vertex colors

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


WELD_DISTANCE = 1e-4	# m, the vertices closer than this become one (the engine splits them by UVs and normals)
QUAD_ANGLE = 0.0873		# radians (5 degrees), the largest angle between two triangles joined into a quad
QUAD_SHAPE_ANGLE = 0.698	# radians (40 degrees), how far the quad corners may be from right angles


def build_mesh(mesh, data, on_material_created=None, weld_distance=WELD_DISTANCE, quads=False):
	"""Fills a bpy mesh from a `blink.Mesh`: a material slot per batch material, the UVs, the normals.

	The engine vertices are split where the UVs or normals change; here the vertices at one position
	(weld_distance) become one, the UVs and normals stay per face corner, so the mesh is connected
	and looks the same. The triangles that become degenerate are dropped. quads joins the triangle
	pairs that are flat (QUAD_ANGLE) and have the same material, UVs and colors into quads.
	The colors are the `Color` byte color corner attribute (the raw bytes as its sRGB values).
	Returns the geometry_hash of the built mesh: the application keeps it outside the mesh (Blender
	undo restores mesh properties) and compares it to tell whether the mesh was edited since.
	"""
	positions = data.positions
	vertex_indices = data.indices
	if weld_distance > 0 and len(positions):
		keys = numpy.round(positions / weld_distance).astype(numpy.int64)
		_, first, remap = numpy.unique(keys, axis=0, return_index=True, return_inverse=True)
		positions = positions[first]
		vertex_indices = remap.reshape(-1)[data.indices]

	triangles = vertex_indices.reshape(-1, 3)
	keep = (triangles[:, 0] != triangles[:, 1]) & (triangles[:, 1] != triangles[:, 2]) & (triangles[:, 0] != triangles[:, 2])
	corners = data.indices.reshape(-1, 3)[keep].reshape(-1)		# the engine vertex of each face corner

	material_indices = numpy.zeros(len(triangles), dtype=numpy.int32)
	mesh.clear_geometry()
	mesh.materials.clear()
	slots = {}
	for name, first_index, index_count in data.batches:
		if name not in slots:
			slots[name] = len(mesh.materials)
			mesh.materials.append(material(name, on_material_created))
		first = first_index // 3
		material_indices[first:first + index_count // 3] = slots[name]
	material_indices = material_indices[keep]

	num_loops = len(corners)
	mesh.vertices.add(len(positions))
	mesh.vertices.foreach_set("co", positions.reshape(-1))
	mesh.loops.add(num_loops)
	mesh.loops.foreach_set("vertex_index", triangles[keep].reshape(-1).astype(numpy.int32))
	mesh.polygons.add(num_loops // 3)
	mesh.polygons.foreach_set("loop_start", numpy.arange(0, num_loops, 3, dtype=numpy.int32))
	mesh.polygons.foreach_set("material_index", material_indices)

	uv_layer = mesh.uv_layers.new(name="UVMap")
	uv_layer.data.foreach_set("uv", data.texcoords[corners].reshape(-1))

	if data.colors is not None and len(data.colors) == len(data.positions):
		attribute = mesh.color_attributes.new(COLOR_ATTRIBUTE, "BYTE_COLOR", "CORNER")
		attribute.data.foreach_set("color_srgb", (data.colors[corners].astype(numpy.float32) / 255.0).reshape(-1))

	mesh.update(calc_edges=True)
	mesh.normals_split_custom_set(data.normals[corners])

	if quads and num_loops:
		import bmesh
		bm = bmesh.new()
		bm.from_mesh(mesh)
		bmesh.ops.join_triangles(bm, faces=bm.faces[:], angle_face_threshold=QUAD_ANGLE,
			angle_shape_threshold=QUAD_SHAPE_ANGLE, cmp_uvs=True, cmp_vcols=True, cmp_materials=True)
		bm.to_mesh(mesh)
		bm.free()
		mesh.update()

	return geometry_hash(mesh)


def _get(collection, name, count, dtype, components=1):
	array = numpy.empty(count * components, dtype=dtype)
	if count:
		collection.foreach_get(name, array)
	return array.reshape(-1, components) if components > 1 else array


def _colors(mesh):
	attribute = mesh.color_attributes.get(COLOR_ATTRIBUTE)
	if attribute is None or attribute.domain != "CORNER" or attribute.data_type != "BYTE_COLOR":
		return None
	colors = _get(attribute.data, "color_srgb", len(mesh.loops), numpy.float32, 4)
	return numpy.clip(numpy.rint(colors * 255.0), 0, 255).astype(numpy.uint8)


def _check_object_mode(mesh):
	if mesh.is_editmode:		# its UV and attribute data are in the edit mesh, not in the mesh
		raise ValueError("mesh '%s' is in Edit Mode: leave it first" % mesh.name)


def geometry_hash(mesh):
	"""A hash of what read_bpy_mesh sends: the positions, faces, materials, UVs, normals, colors.

	Not for a mesh in Edit Mode (ValueError).
	"""
	_check_object_mode(mesh)
	digest = hashlib.blake2b(digest_size=16)
	digest.update(_get(mesh.vertices, "co", len(mesh.vertices), numpy.float32, 3).tobytes())
	digest.update(_get(mesh.loops, "vertex_index", len(mesh.loops), numpy.int32).tobytes())
	digest.update(_get(mesh.polygons, "loop_start", len(mesh.polygons), numpy.int32).tobytes())
	digest.update(_get(mesh.polygons, "material_index", len(mesh.polygons), numpy.int32).tobytes())
	digest.update(_get(mesh.corner_normals, "vector", len(mesh.loops), numpy.float32, 3).tobytes())
	uv_layer = mesh.uv_layers.active
	if uv_layer is not None:
		digest.update(_get(uv_layer.data, "uv", len(mesh.loops), numpy.float32, 2).tobytes())
	colors = _colors(mesh)
	if colors is not None:
		digest.update(colors.tobytes())
	digest.update("|".join(material_name(mesh, i) for i in range(len(mesh.materials))).encode("utf-8"))
	return digest.hexdigest()


def material_name(mesh, index):
	"""The engine material of a slot: its `eq_material`, else the Blender material name."""
	if 0 <= index < len(mesh.materials) and mesh.materials[index] is not None:
		mat = mesh.materials[index]
		return mat.get(MATERIAL_KEY, mat.name)
	return "error"


def read_bpy_mesh(mesh, matrix=None):
	"""A `blink.Mesh` of a bpy mesh (Blender coordinates), for `blink.write_mesh`.

	The faces are triangulated; a vertex is split per face corner where the UVs, the normals, the
	colors or the material differ, and the triangles are ordered by material, a batch each.
	matrix (a 4x4 mathutils.Matrix): the vertices are moved by it (an object matrix_world).
	Not for a mesh in Edit Mode (ValueError): its UVs and attributes are in the edit mesh.
	"""
	_check_object_mode(mesh)
	mesh.calc_loop_triangles()
	num_triangles = len(mesh.loop_triangles)
	num_loops = len(mesh.loops)

	triangle_loops = _get(mesh.loop_triangles, "loops", num_triangles, numpy.int32, 3)
	triangle_materials = _get(mesh.loop_triangles, "material_index", num_triangles, numpy.int32)
	loop_vertices = _get(mesh.loops, "vertex_index", num_loops, numpy.int32)
	positions = _get(mesh.vertices, "co", len(mesh.vertices), numpy.float32, 3)
	loop_normals = _get(mesh.corner_normals, "vector", num_loops, numpy.float32, 3)
	uv_layer = mesh.uv_layers.active
	if uv_layer is not None:
		loop_uvs = _get(uv_layer.data, "uv", num_loops, numpy.float32, 2)
	else:
		loop_uvs = numpy.zeros((num_loops, 2), numpy.float32)
	loop_colors = _colors(mesh)

	order = numpy.argsort(triangle_materials, kind="stable")
	triangle_materials = triangle_materials[order]
	corner_loops = triangle_loops[order].reshape(-1)

	# a unique row per vertex; the material comes first, so the vertices are in batch order too
	columns = [numpy.repeat(triangle_materials, 3)[:, None], loop_vertices[corner_loops][:, None],
		loop_uvs[corner_loops], loop_normals[corner_loops]]
	if loop_colors is not None:
		columns.append(loop_colors[corner_loops])
	keys = numpy.hstack([column.astype(numpy.float64) for column in columns])
	_, first, inverse = numpy.unique(keys, axis=0, return_index=True, return_inverse=True)
	source_loops = corner_loops[first]

	out_positions = positions[loop_vertices[source_loops]]
	out_normals = loop_normals[source_loops]
	indices = inverse.reshape(-1).astype(numpy.int32)
	if matrix is not None:
		rotation = matrix.to_3x3()
		normal_matrix = numpy.array(rotation.inverted_safe().transposed(), dtype=numpy.float32)
		out_positions = out_positions @ numpy.array(rotation, dtype=numpy.float32).T + numpy.array(matrix.translation, dtype=numpy.float32)
		out_normals = out_normals @ normal_matrix.T
		out_normals = out_normals / numpy.maximum(numpy.linalg.norm(out_normals, axis=1, keepdims=True), 1e-12)
		if rotation.determinant() < 0.0:		# mirrored: the winding is reversed
			indices = indices.reshape(-1, 3)[:, [0, 2, 1]].reshape(-1)

	batches = []
	if num_triangles:
		materials, starts, counts = numpy.unique(triangle_materials, return_index=True, return_counts=True)
		batches = [(material_name(mesh, int(index)), int(start) * 3, int(count) * 3)
			for index, start, count in zip(materials, starts, counts)]

	colors = loop_colors[source_loops].copy() if loop_colors is not None else None
	return Mesh(out_positions.astype(numpy.float32), out_normals.astype(numpy.float32), loop_uvs[source_loops].copy(),
		indices, batches, colors)


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
