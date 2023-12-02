#include "core/core_common.h"
#include "StudioVertex.h"
#include "materialsystem1/renderers/ShaderAPI_defs.h"

static EGFHwVertex::VertexStreamId s_defaultVertexStreamMappingDesc[] = {
	EGFHwVertex::VERT_POS_UV,
	EGFHwVertex::VERT_TBN,
	EGFHwVertex::VERT_BONEWEIGHT,
	EGFHwVertex::VERT_COLOR,
};
ArrayCRef<EGFHwVertex::VertexStreamId> g_defaultVertexStreamMapping = { s_defaultVertexStreamMappingDesc ,  elementsOf(s_defaultVertexStreamMappingDesc) };

EGFHwVertex::PositionUV::PositionUV(const studioVertexPosUv_t& initFrom)
{
	pos = Vector4D(initFrom.point, 1.0f);
	texcoord = initFrom.texCoord;
}

EGFHwVertex::TBN::TBN(const studioVertexTBN_t& initFrom)
{
	tangent = initFrom.tangent;
	binormal = initFrom.binormal;
	normal = initFrom.normal;
}

EGFHwVertex::BoneWeights::BoneWeights()
{
	memset(boneWeights, 0, sizeof(boneWeights));
	for (int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; i++)
		boneIndices[i] = -1;
}

EGFHwVertex::BoneWeights::BoneWeights(const studioBoneWeight_t& initFrom) : BoneWeights()
{
	ASSERT(initFrom.numweights <= MAX_MODEL_VERTEX_WEIGHTS);
	for (int i = 0; i < min(initFrom.numweights, MAX_MODEL_VERTEX_WEIGHTS); i++)
	{
		boneIndices[i] = initFrom.bones[i];
		boneWeights[i] = initFrom.weight[i];
	}
}

EGFHwVertex::Color::Color(const studioVertexColor_t& initFrom)
{
	color = initFrom.color;
}

const VertexLayoutDesc& EGFHwVertex::PositionUV::GetVertexLayoutDesc()
{
	static const VertexLayoutDesc g_EGFVertexUvFormat = Builder<VertexLayoutDesc>()
		.UserId(EGFHwVertex::VERT_POS_UV)
		.Stride(sizeof(EGFHwVertex::PositionUV))
		.Attribute(VERTEXATTRIB_POSITION, "position", 0, offsetOf(EGFHwVertex::PositionUV, pos), ATTRIBUTEFORMAT_HALF, 4)
		.Attribute(VERTEXATTRIB_TEXCOORD, "texCoord", 1, offsetOf(EGFHwVertex::PositionUV, texcoord), ATTRIBUTEFORMAT_HALF, 2)
		.End();
	return g_EGFVertexUvFormat;
}

const VertexLayoutDesc& EGFHwVertex::TBN::GetVertexLayoutDesc()
{
	static const VertexLayoutDesc g_EGFTBNFormat = Builder<VertexLayoutDesc>()
		.UserId(EGFHwVertex::VERT_TBN)
		.Stride(sizeof(EGFHwVertex::TBN))
		.Attribute(VERTEXATTRIB_TEXCOORD, "tangent", 2, offsetOf(EGFHwVertex::TBN, tangent), ATTRIBUTEFORMAT_HALF, 4)
		.Attribute(VERTEXATTRIB_TEXCOORD, "binormal", 3, offsetOf(EGFHwVertex::TBN, binormal), ATTRIBUTEFORMAT_HALF, 4)
		.Attribute(VERTEXATTRIB_TEXCOORD, "normal", 4, offsetOf(EGFHwVertex::TBN, normal), ATTRIBUTEFORMAT_HALF, 4)
		.End();
	return g_EGFTBNFormat;
}

const VertexLayoutDesc& EGFHwVertex::BoneWeights::GetVertexLayoutDesc()
{
	static const VertexLayoutDesc g_EGFBoneWeightsFormat = Builder<VertexLayoutDesc>()
		.UserId(EGFHwVertex::VERT_BONEWEIGHT)
		.Stride(sizeof(EGFHwVertex::BoneWeights))
		.Attribute(VERTEXATTRIB_TEXCOORD, "boneId", 5, offsetOf(EGFHwVertex::BoneWeights, boneIndices), ATTRIBUTEFORMAT_UINT8, 4)
		.Attribute(VERTEXATTRIB_TEXCOORD, "boneWt", 6, offsetOf(EGFHwVertex::BoneWeights, boneWeights), ATTRIBUTEFORMAT_HALF, 4)
		.End();
	return g_EGFBoneWeightsFormat;
}

const VertexLayoutDesc& EGFHwVertex::Color::GetVertexLayoutDesc()
{
	static const VertexLayoutDesc g_EGFColorFormat = Builder<VertexLayoutDesc>()
		.UserId(EGFHwVertex::VERT_COLOR)
		.Stride(sizeof(EGFHwVertex::Color))
		.Attribute(VERTEXATTRIB_TEXCOORD, "color", 7, offsetOf(EGFHwVertex::Color, color), ATTRIBUTEFORMAT_UINT8, 4)
		.End();
	return g_EGFColorFormat;
}
