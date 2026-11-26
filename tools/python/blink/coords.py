"""Engine <-> Blender coordinates. The link protocols use engine coordinates; only the Blender side converts.

The engine is Y up and left handed, Blender is Z up and right handed, 1 unit is 1 m in both:
Blender (x, y, z) is engine (-x, z, -y). The conversion is a reflection, so the triangle winding is
reversed, and the texture V is 1 - v, like the OBJ export of the level editor.

An object rotation is sent as the images of the engine unit axes (axisX, axisY, axisZ), as the
engine draws the object; there is no Euler angle order to agree on.

No bpy here: the functions take and return tuples and numpy arrays (Blender has numpy, the
object functions work without it); the application makes the `mathutils.Matrix` of the rows.
"""


def to_blender(v):
	"""An engine point or direction in Blender coordinates."""
	return (-v[0], -v[2], v[1])


def to_engine(v):
	"""A Blender point or direction in engine coordinates."""
	return (-v[0], v[2], -v[1])


def _add(a, b):
	return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _sub(a, b):
	return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _neg(a):
	return (-a[0], -a[1], -a[2])


def _normalize(a):
	length = (a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) ** 0.5
	if length < 1e-12:
		return a
	return (a[0] / length, a[1] / length, a[2] / length)


def object_to_blender(position, axes, placement_offset=(0.0, 0.0, 0.0)):
	"""4x4 Blender world matrix rows of an engine object.

	position: the object position (without the def placement offset), axes: the engine axisX,
	axisY, axisZ. The Blender basis X, Y, Z are engine -X, -Z, +Y, so the matrix columns are the
	images of those axes converted back.
	"""
	axis_x, axis_y, axis_z = axes
	columns = (to_blender(_neg(axis_x)), to_blender(_neg(axis_z)), to_blender(axis_y))
	origin = to_blender(_add(position, placement_offset))
	return [
		[columns[0][0], columns[1][0], columns[2][0], origin[0]],
		[columns[0][1], columns[1][1], columns[2][1], origin[1]],
		[columns[0][2], columns[1][2], columns[2][2], origin[2]],
		[0.0, 0.0, 0.0, 1.0],
	]


def object_to_engine(rows, placement_offset=(0.0, 0.0, 0.0)):
	"""position and (axisX, axisY, axisZ) of a Blender world matrix (rows); the scale is dropped."""
	column = lambda i: (rows[0][i], rows[1][i], rows[2][i])
	col_x = _normalize(column(0))
	col_y = _normalize(column(1))
	col_z = _normalize(column(2))
	axis_x = _neg(to_engine(col_x))
	axis_z = _neg(to_engine(col_y))
	axis_y = to_engine(col_z)
	position = _sub(to_engine(column(3)), placement_offset)
	return position, (axis_x, axis_y, axis_z)


def mesh_to_blender(positions, normals, texcoords, indices):
	"""Engine mesh arrays (numpy, (n, 3) / (n, 3) / (n, 2) / (m,)) in Blender order.

	Returns new arrays: positions and normals converted, V flipped, every triangle reversed.
	"""
	import numpy

	positions = numpy.asarray(positions, dtype=numpy.float32)
	normals = numpy.asarray(normals, dtype=numpy.float32)
	texcoords = numpy.asarray(texcoords, dtype=numpy.float32)

	out_positions = numpy.empty_like(positions)
	out_positions[:, 0] = -positions[:, 0]
	out_positions[:, 1] = -positions[:, 2]
	out_positions[:, 2] = positions[:, 1]

	out_normals = numpy.empty_like(normals)
	out_normals[:, 0] = -normals[:, 0]
	out_normals[:, 1] = -normals[:, 2]
	out_normals[:, 2] = normals[:, 1]

	out_texcoords = texcoords.copy()
	out_texcoords[:, 1] = 1.0 - texcoords[:, 1]

	triangles = numpy.asarray(indices, dtype=numpy.int32).reshape(-1, 3)
	out_indices = triangles[:, [0, 2, 1]].ravel()
	return out_positions, out_normals, out_texcoords, out_indices
