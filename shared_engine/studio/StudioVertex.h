#pragma once
#include "egf/model.h"

struct VertexLayoutDesc;

// egf model hardware vertex
struct EGFHwVertex
{
	enum VertexStreamId : int
	{
		VERT_POS_UV,
		VERT_TBN,
		VERT_BONEWEIGHT,
		VERT_COLOR,

		// TODO: more UVs ?

		VERT_COUNT,

		EGF_MASK = (1 << VERT_COUNT)-1,
		EGF_FLAG = (1 << 31)
	};

	struct PositionUV
	{
		static const VertexLayoutDesc& GetVertexLayoutDesc();

		PositionUV() = default;
		PositionUV(const studioVertexPosUv_t& initFrom);

		TVec4D<half>	pos;
		TVec2D<half>	texcoord;
	};

	struct TBN
	{
		static const VertexLayoutDesc& GetVertexLayoutDesc();

		TBN() = default;
		TBN(const studioVertexTBN_t& initFrom);

		TVec3D<half>	tangent;
		half			unused1;	// half float types are unsupported with v3d, turn them into v4d
		TVec3D<half>	binormal;
		half			unused2;
		TVec3D<half>	normal;
		half			unused3;
	};

	struct BoneWeights
	{
		static const VertexLayoutDesc& GetVertexLayoutDesc();

		BoneWeights();
		BoneWeights(const studioBoneWeight_t& initFrom);

		uint8			boneIndices[MAX_MODEL_VERTEX_WEIGHTS];
		half			boneWeights[MAX_MODEL_VERTEX_WEIGHTS];
	};

	struct Color
	{
		Color() = default;
		Color(const studioVertexColor_t& initFrom);

		static const VertexLayoutDesc& GetVertexLayoutDesc();
		uint			color{ color_white.pack() };
	};
};
