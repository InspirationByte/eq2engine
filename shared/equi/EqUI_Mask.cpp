//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"
#include "utils/TextureAtlas.h"

#include "EqUI_Mask.h"
#include "EqUI_Manager.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

DECLARE_CVAR(equi_debugMaskClear, "0", nullptr, 0);

namespace equi
{

Mask::Mask() : IUIControl()
{

}

Mask::~Mask()
{
}

void Mask::InitMask(const char* fileName)
{
	m_uvRegion = AARectangle(0.0f, 0.0f, 1.0f, 1.0f);

	const EqString childsRTName = "_ui_" + m_name + "_masked";

	m_maskedChilds = g_renderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name(childsRTName)
		.Size(4, 4)
		.Format(FORMAT_RGBA8)
		.End()
	);

	KVSection matParams;
	matParams.SetName("UIOverlayMask");
	matParams.SetKey("BaseTexture", childsRTName);
	matParams.SetKey("MaskTexture", fileName);

	m_maskMaterial = g_matSystem->CreateMaterial("_ui_" + m_name + "_mask", &matParams);
	g_matSystem->QueueLoading(m_maskMaterial);
}

void Mask::Parse(const KVSection& sec)
{
	BaseClass::Parse(sec);

	EqStringRef maskImageName;
	sec.Get("image").GetValues(maskImageName);
	sec.Get("color").GetValues(m_color);

	InitMask(maskImageName);
}

AARectangle Mask::GetUVRegion() const
{
	return AARectangle(m_uvRegion.leftTop * m_size, m_uvRegion.rightBottom * m_size);
}

void Mask::SetUVRegion(const AARectangle& rect)
{
	const Vector2D invSize = 1.0f / m_size;
	m_uvRegion = AARectangle(rect.leftTop * invSize, rect.rightBottom * invSize);
}

void Mask::DrawSelf( const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder)
{
	m_renderRect = rect;

	TextureExtent texSize(rect.GetSize());
	g_renderAPI->ResizeRenderTarget(m_maskedChilds, texSize);
}

void Mask::RenderChilds(int depth, RenderContextAbstract& context)
{
	ASSERT_MSG(m_selfVisible, "Mask %s must be selfVisible", m_name.ToCString());
	if (!m_selfVisible)
	{
		BaseClass::RenderChilds(depth, context);
		return;
	}

	Matrix4x4 actualProj;
	g_matSystem->GetMatrix(MATRIXMODE_PROJECTION, actualProj);

	const Matrix4x4 actualTransform = context.transformStack.back();
	const Vector2D projSize = m_renderRect.GetSize();
	
	Matrix4x4 offsetTransform = actualTransform * translate(-(float)m_renderRect.leftTop.x, -(float)m_renderRect.leftTop.y, 0.0f);
	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, projection2DScreen(projSize.x, projSize.y));
	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, offsetTransform);
	
	// render childs into texture first
	IGPURenderPassRecorderPtr maskRenderPass = g_renderAPI->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(m_maskedChilds, equi_debugMaskClear.GetBool(), color_red)
		.End()
	);
	
	{
		RenderContextAbstract maskContext(maskRenderPass, context.transformStack);
		maskContext.transformStack.back() = offsetTransform;
		BaseClass::RenderChilds(depth, maskContext);
		context.transformStack.back() = actualTransform;
	}

	g_matSystem->QueueCommandBuffer(maskRenderPass->End());
	
	// restore
	g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, actualProj);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, actualTransform);

	// draw child elements masked
	{
		AARectangle uvRect = m_uvRegion;

		// draw all rectangles with just single draw call
		CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

		RenderDrawCmd drawCmd;
		drawCmd.SetMaterial(m_maskMaterial);

		MatSysDefaultRenderPass defaultRenderPass;
		defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
		defaultRenderPass.cullMode = CULL_NONE;

		meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		meshBuilder.Color4fv(m_color);
		meshBuilder.TexturedQuad2(
			m_renderRect.GetLeftBottom(), m_renderRect.GetLeftTop(), m_renderRect.GetRightBottom(), m_renderRect.GetRightTop(),
			uvRect.GetLeftBottom(), uvRect.GetLeftTop(), uvRect.GetRightBottom(), uvRect.GetRightTop());
		if (meshBuilder.End(drawCmd))
			g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(context.rendPassRecorder, &defaultRenderPass));
	}
}

};

DECLARE_EQUI_CONTROL(mask, Mask)
