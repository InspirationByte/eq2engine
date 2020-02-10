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

#include "materialsystem/MeshBuilder.h"

#include "in_keys_ident.h"

//-------------------------------------------------------------------
// Base control
//-------------------------------------------------------------------

namespace equi
{

//-------------------------------------------------------------------
// Panels
//-------------------------------------------------------------------

Panel::Panel() : m_windowControls(false), m_labelCtrl(NULL), m_closeButton(NULL), m_grabbed(false), m_screenOverlay(false)
{
	m_position = IVector2D(0);
	m_size = IVector2D(32,32);

	m_color = ColorRGBA(0.7,0.7,0.7,0.9f);
	m_selColor = ColorRGBA(0.25f);
}

Panel::~Panel()
{

}

void Panel::InitFromKeyValues( kvkeybase_t* sec, bool noClear )
{
	// initialize from scheme
	kvkeybase_t* mainSec = sec->FindKeyBase("panel");

	if (mainSec == nullptr)
		mainSec = sec->FindKeyBase("child");

	if (mainSec == nullptr)
		mainSec = sec;

	BaseClass::InitFromKeyValues(mainSec, noClear);

	m_windowControls = KV_GetValueBool(mainSec->FindKeyBase("window"), 0, m_windowControls);
	m_visible = !m_windowControls;
	m_visible = KV_GetValueBool(mainSec->FindKeyBase("visible"), 0, m_visible);
	m_screenOverlay = KV_GetValueBool(mainSec->FindKeyBase("screenoverlay"), 0, m_screenOverlay);
	m_color = KV_GetVector4D(mainSec->FindKeyBase("color"), 0, m_color);
	m_grabbed = false;
	m_closeButton = nullptr;
	m_labelCtrl = nullptr;

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
			m_labelCtrl->SetLabel( GetLabel() );
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

void Panel::CenterOnScreen()
{
	const IRectangle& screenRect = Manager->GetViewFrame();

	IVector2D panelSize = GetRectangle().GetSize();

	IVector2D screenCenter = screenRect.GetCenter();
	
	SetPosition(screenCenter - panelSize/2);
}

void DrawWindowRectangle(const Rectangle_t &rect, const ColorRGBA &color1, const ColorRGBA &color2)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(NULL,0,0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false,false);

	materials->BindMaterial(materials->GetDefaultMaterial());

	Vector2D r0[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -1) };
	Vector2D r1[] = { MAKEQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vector2D r2[] = { MAKEQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -1) };
	Vector2D r3[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -1) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		// put main rectangle
		meshBuilder.Color4fv(color1);
		meshBuilder.Quad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop());

		// put borders
		meshBuilder.Color4fv(color2);
		meshBuilder.Quad2(r0[0], r0[1], r0[2], r0[3]);
		meshBuilder.Quad2(r1[0], r1[1], r1[2], r1[3]);
		meshBuilder.Quad2(r2[0], r2[1], r2[2], r2[3]);
		meshBuilder.Quad2(r3[0], r3[1], r3[2], r3[3]);
	meshBuilder.End();
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
	DrawWindowRectangle(rect, m_color, ColorRGBA(m_color.xyz()*0.25f, 1.0f) );
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

class HudDummyElement : public IUIControl
{
public:
	EQUI_CLASS(HudDummyElement, IUIControl)

	HudDummyElement() : IUIControl() {}
	~HudDummyElement() {}

	// events
	bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags)	{return true;}
	bool			ProcessKeyboardEvents(int nKeyButtons, int flags)					{return true;}

	void			DrawSelf( const IRectangle& rect ) {}
};

};

DECLARE_EQUI_CONTROL(panel, Panel)
DECLARE_EQUI_CONTROL(HudElement, HudDummyElement)
DECLARE_EQUI_CONTROL(Container, HudDummyElement)