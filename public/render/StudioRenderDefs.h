#pragma once

struct RenderBoneTransform
{
	Quaternion	quat;
	Vector4D	origin;
};

// must be exactly two regs
assert_sizeof(RenderBoneTransform, sizeof(Vector4D) * 2);

constexpr uint RenderBoneTransformID = MAKECHAR4('B','S','K','N');
