//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI progress bar
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "EqUI_ProgressBar.h"

#include "EqUI_Manager.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

namespace equi
{

ProgressBar::ProgressBar()
	: IUIControl() 
{
	m_value = 0.5f;
	m_color = color_white;
}

void ProgressBar::InitFromKeyValues(KVSection* sec, bool noClear)
{
	BaseClass::InitFromKeyValues(sec, noClear);

	m_color = KV_GetVector4D(sec->FindSection("color"), 0, m_color);
	m_value = KV_GetValueFloat(sec->FindSection("value"), 0, m_value);
}

void ProgressBar::DrawSelf(const IAARectangle& _rect, bool scissorOn)
{
	AARectangle rect(_rect);

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

	RenderDrawCmd drawCmd;
	drawCmd.material = g_matSystem->GetDefaultMaterial();

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	drawCmd.userData = &defaultRenderPass;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

	meshBuilder.Color4f(0.435, 0.435, 0.435, m_color.w * 0.25f);
	meshBuilder.Quad2(rect.GetRightTop(), rect.GetLeftTop(), rect.GetRightBottom(), rect.GetLeftBottom());

	float percentage = m_value;

	if (percentage > 0)
	{
		AARectangle fillRect(Vector2D(rect.leftTop.x, rect.leftTop.y), 
							 Vector2D(lerp(rect.leftTop.x, rect.rightBottom.x, percentage), rect.rightBottom.y));

		// draw damage bar foreground
		meshBuilder.Color4fv(m_color);
		meshBuilder.Quad2(fillRect.GetRightTop(), fillRect.GetLeftTop(), fillRect.GetRightBottom(), fillRect.GetLeftBottom());
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->Draw(drawCmd);
}

}

DECLARE_EQUI_CONTROL(progressBar, ProgressBar)