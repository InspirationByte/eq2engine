#pragma once

struct Transform3D
{
	Quaternion		orientation{ qidentity };
	Vector3D		origin{ vec3_zero };
	Vector3D		scale{ 1.0f };
};

// TODO: transform methods
