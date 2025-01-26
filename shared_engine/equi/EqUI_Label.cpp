//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "EqUI_Label.h"

#include "EqUI_Manager.h"
#include "font/FontLayoutBuilders.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

namespace equi
{

// drawn rectangle
IAARectangle Label::GetClientScissorRectangle(int depth, const RenderContextAbstract& context) const
{
	IAARectangle clientRect = GetClientRectangle();

	// apply line height offset
	{
		FontStyleParam style;
		GetCalcFontStyle(style);

		const float lineHeight = GetFont()->GetLineHeight(style);
		clientRect.leftTop.y -= lineHeight * 0.5f;
	}

	if (m_clipTransform)
		clientRect = TransformScissorRectangle(clientRect, context.transformStack[depth]);

	if (!m_parent || !m_parent->IsClipsChilds())
	{
		return clientRect;
	}

	const IAARectangle parentRect = m_parent->GetClientScissorRectangle(depth - 1, context);
	return ClipScissorRectangle(clientRect, parentRect);
}

void Label::DrawSelf( const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder)
{
	CRectangleTextLayoutBuilder rectLayout;
	rectLayout.SetRectangle(AARectangle(rect));

	IEqFont* font = GetFont();
	FontStyleParam style;
	GetCalcFontStyle(style);

	style.layoutBuilder = &rectLayout;

	IVector2D pos = rect.GetLeftTop() + IVector2D(0, font->GetLineHeight(style)*0.5f);

	// draw label
	font->SetupRenderText(m_labelText, pos, style, rendPassRecorder);
}

};

DECLARE_EQUI_CONTROL(label, Label)
