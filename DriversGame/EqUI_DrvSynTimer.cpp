//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI timer
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_DrvSynTimer.h"

#include "EqUI/EqUI_Manager.h"

#include "IFont.h"
#include "FontLayoutBuilders.h"

#include "MeshBuilder.h"

#include "utils/strtools.h"

namespace equi
{

DrvSynTimerElement::DrvSynTimerElement() 
	: IUIControl() 
{
	m_timeDisplayValue = 0.0f;
	m_type = TIMER_TYPE_DIGITS;
}

void DrvSynTimerElement::InitFromKeyValues(kvkeybase_t* sec, bool noClear)
{
	BaseClass::InitFromKeyValues(sec, noClear);

	m_type = (ETimerType)KV_GetValueInt(sec->FindKeyBase("type"));
}

void DrvSynTimerElement::DrawSelf(const IRectangle& rect)
{
	if (m_type == TIMER_TYPE_DIGITS)
	{
		IEqFont* font = GetFont();

		int secs, mins, millisecs;

		secs = m_timeDisplayValue;
		mins = secs / 60;

		millisecs = m_timeDisplayValue * 100.0f - secs * 100;

		secs -= mins * 60;

		// setup font
		eqFontStyleParam_t style;
		GetCalcFontStyle(style);
		style.align = TEXT_ALIGN_HCENTER;
		style.styleFlag |= TEXT_STYLE_FROM_CAP;

		// clock displayed centered
		Vector2D timeDisplayTextPos(rect.GetCenter().x, rect.GetCenter().y);

		wchar_t* str = varargs_w(L"%.2i:%.2i", mins, secs);

		float minSecWidth = font->GetStringWidth(str, style);
		font->RenderText(str, timeDisplayTextPos, style);

		style.align = 0;
		style.scale *= 0.5f;

		Vector2D millisDisplayTextPos = timeDisplayTextPos + Vector2D(floor(minSecWidth*0.5f) + 10, 0.0f);

		font->RenderText(varargs_w(L"'%02d", millisecs), millisDisplayTextPos, style);
	}
	else if (m_type == TIMER_TYPE_CLOCKFACE)
	{
		BlendStateParam_t blending;
		blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
		blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

		g_pShaderAPI->SetTexture(NULL, 0, 0);
		materials->SetBlendingStates(blending);
		materials->SetRasterizerStates(CULL_NONE, FILL_SOLID);
		materials->SetDepthStates(false, false);

		materials->BindMaterial(materials->GetDefaultMaterial());

		int secs = ceil(m_timeDisplayValue);

		Vector2D center = rect.GetCenter();
		float arrowSize = rect.GetSize().y * 0.5f;

		// setup angle
		float arrowAngle = fabs(float(secs) / 60.0f * 360.0f);

		Matrix2x2 rotation = rotate2( - DEG2RAD(arrowAngle));

		Vector2D vertices[] = {
			Vector2D(-3.0f, arrowSize*0.25f),
			Vector2D(3.0f, arrowSize*0.25f),
			Vector2D(0.0f, -arrowSize)
		};

		// draw arrow poly

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());
		meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
			meshBuilder.Color4f(0.0f, 0.0f, 0.0f, 1.0f);

			meshBuilder.Triangle2(center + rotation*vertices[0], center + rotation*vertices[1], center + rotation*vertices[2]);
			
		meshBuilder.End();
	}
}

}

DECLARE_EQUI_CONTROL(timer, DrvSynTimerElement)