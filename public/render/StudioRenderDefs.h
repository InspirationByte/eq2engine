#pragma once

struct RenderBoneTransform
{
	Quaternion	quat{ qidentity };
	Vector4D	origin{0.0f, 0.0f, 0.0f, 1.0f};
};

// must be exactly two regs
assert_sizeof(RenderBoneTransform, sizeof(Vector4D) * 2);

constexpr uint RenderBoneTransformID = MAKECHAR4('B','S','K','N');
