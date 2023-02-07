//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

void ProgressBar::DrawSelf(const IRectangle& _rect, bool scissorOn)
{
	// setup default material and translucent blending
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	MatTextureProxy(materials->FindGlobalMaterialVar(StringToHashConst("basetexture"))).Set(nullptr);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_NONE, FILL_SOLID);
	materials->SetDepthStates(false, false);

	materials->BindMaterial(materials->GetDefaultMaterial());

	//-------------------

	Rectangle_t rect(_rect);

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

	meshBuilder.Color4f(0.435, 0.435, 0.435, m_color.w * 0.25f);
	meshBuilder.Quad2(rect.GetRightTop(), rect.GetLeftTop(), rect.GetRightBottom(), rect.GetLeftBottom());

	float percentage = m_value;

	if (percentage > 0)
	{
		Rectangle_t fillRect(Vector2D(rect.vleftTop.x, rect.vleftTop.y), 
							 Vector2D(lerp(rect.vleftTop.x, rect.vrightBottom.x, percentage), rect.vrightBottom.y));

		// draw damage bar foreground
		meshBuilder.Color4fv(m_color);
		meshBuilder.Quad2(fillRect.GetRightTop(), fillRect.GetLeftTop(), fillRect.GetRightBottom(), fillRect.GetLeftBottom());
	}

	meshBuilder.End();
}

}

DECLARE_EQUI_CONTROL(progressBar, ProgressBar)