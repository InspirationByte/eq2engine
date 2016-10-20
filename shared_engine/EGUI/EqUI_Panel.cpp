//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Panel.h"
#include "EqUI_Button.h"
#include "EqUI_Label.h"

#include "EqUI_Manager.h"

#include "materialsystem/IMaterialSystem.h"

#include "in_keys_ident.h"

//-------------------------------------------------------------------
// Base control
//-------------------------------------------------------------------

namespace equi
{

//-------------------------------------------------------------------
// Panels
//-------------------------------------------------------------------

Panel::Panel() : m_windowControls(false), m_labelCtrl(NULL), m_closeButton(NULL), m_grabbed(false)
{
	m_position = IVector2D(0);
	m_size = IVector2D(32,32);

	m_color = ColorRGBA(0.7,0.7,0.7,0.9f);
	m_selColor = ColorRGBA(0.25f);
}

Panel::~Panel()
{

}

void Panel::InitFromKeyValues( kvkeybase_t* sec )
{
	// initialize from scheme
	kvkeybase_t* mainSec = sec->FindKeyBase("panel");

	if(mainSec == NULL)
		mainSec = sec->FindKeyBase("child");

	BaseClass::InitFromKeyValues(mainSec);

	m_windowControls = KV_GetValueBool(mainSec->FindKeyBase("window"), 0, false);
	m_visible = KV_GetValueBool(mainSec->FindKeyBase("visible"), 0, !m_windowControls);

	if(m_windowControls)
	{
		kvkeybase_t winRes;
		KV_LoadFromFile("resources/WindowControls.res", -1, &winRes);

		// create additional controls
		for(int i = 0; i < winRes.keys.numElem(); i++)
		{
			kvkeybase_t* csec = winRes.keys[i];
			if(!csec->IsSection())
				continue;

			if(!stricmp(csec->GetName(), "child"))
			{
				const char* controlClass = KV_GetValueString(csec, 0, "");

				IUIControl* control = equi::Manager->CreateElement( controlClass );

				if(!control)
					continue;

				control->InitFromKeyValues(csec);
				AddChild( control );
			}
		}

		m_labelCtrl = (equi::Label*)FindChild("WindowLabel");
		m_closeButton = (equi::Button*)FindChild("WindowCloseBtn");

		if(m_labelCtrl)
			m_labelCtrl->SetLabel( m_label.c_str() );
	}
}

void Panel::Hide()
{
	if(equi::Manager->GetTopPanel() == this)
		equi::Manager->SetFocus( NULL );

	BaseClass::Hide();
}

void Panel::SetColor(const ColorRGBA &color)
{
	m_color = color;
}

void Panel::GetColor(ColorRGBA &color) const
{
	color = m_color;
}

void Panel::SetSelectionColor(const ColorRGBA &color)
{
	m_selColor = color;
}

void Panel::GetSelectionColor(ColorRGBA &color) const
{
	color = m_selColor;
}

void DrawAlphaFilledRectangle(const IRectangle &rect, const ColorRGBA &color1, const ColorRGBA &color2)
{
	Vertex2D_t tmprect[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, 0) };

	// Cancel textures
	g_pShaderAPI->Reset(STATE_RESET_TEX);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), NULL, color1, &blending);

	// Set color

	// Draw 4 solid rectangles
	Vertex2D_t r0[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -1) };
	Vertex2D_t r1[] = { MAKETEXQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vertex2D_t r2[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vertex2D_t r3[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -1) };

	// Set alpha,rasterizer and depth parameters
	//g_pShaderAPI->SetBlendingStateFromParams(NULL);
	//g_pShaderAPI->ChangeRasterStateEx(CULL_FRONT,FILL_SOLID);

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r0,elementsOf(r0), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r1,elementsOf(r1), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r2,elementsOf(r2), NULL, color2, &blending);
	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,r3,elementsOf(r3), NULL, color2, &blending);
}

void Panel::Render()
{
	// move window controls
	if(m_closeButton)
	{
		IVector2D closePos = IVector2D(m_size.x-m_closeButton->GetSize().x - 5, 5);

		m_closeButton->SetPosition(closePos);
	}

	BaseClass::Render();
}

void Panel::DrawSelf(const IRectangle& rect)
{
	DrawAlphaFilledRectangle(rect, m_color, ColorRGBA(m_color.xyz()*0.25f, 1.0f) );
}

bool Panel::ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags)
{
	if(nMouseButtons == MOU_B1)
	{
		m_grabbed = (flags & UIEVENT_UP) ? false : true;
	}

	if((flags & UIEVENT_MOUSE_OUT) && m_grabbed)
		m_grabbed = false;

	if(m_grabbed && (flags & UIEVENT_MOUSE_MOVE))
	{
		m_position += mouseDelta;
	}

	return true;
}

};

DECLARE_EQUI_CONTROL(panel, Panel)