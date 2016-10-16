//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Panel.h"
#include "EQUI_Manager.h"
#include "materialsystem/IMaterialSystem.h"

//-------------------------------------------------------------------
// Base control
//-------------------------------------------------------------------

IEqUIControl::IEqUIControl() : m_visible(false), m_enabled(true), m_parent(NULL)
{

}

IEqUIControl::~IEqUIControl()
{
	ClearChilds(true);
}

void IEqUIControl::SetSize(const IVector2D &size)
{
	m_size = size;
}

void IEqUIControl::SetPosition(const IVector2D &pos)
{
	m_position = pos;
}

void IEqUIControl::SetRectangle(const IRectangle& rect)
{
	m_position = rect.vleftTop;
	m_size = rect.vrightBottom - m_position;
}

bool IEqUIControl::IsVisible() const
{
	if(m_parent)
		return m_parent->IsVisible() && m_visible;

	return m_visible;
}

bool IEqUIControl::IsEnabled() const
{
	if(m_parent)
		return m_parent->IsEnabled() && m_enabled;

	return m_enabled;
}

const IVector2D& IEqUIControl::GetSize() const
{
	return m_size;
}

const IVector2D& IEqUIControl::GetPosition() const
{
	return m_position;
}

// clipping rectangle, size position
IRectangle IEqUIControl::GetRectangle() const
{
	return IRectangle(m_position, m_position + m_size);
}

//-------------------------------------------------------------------
// Panels
//-------------------------------------------------------------------

CEqUI_Panel::CEqUI_Panel() : m_mouseOver(NULL)
{
	m_position = IVector2D(0);
	m_size = IVector2D(32,32);

	m_color = ColorRGBA(0,0,0, 0.5f);
	m_selColor = ColorRGBA(0.25f);
}

CEqUI_Panel::~CEqUI_Panel()
{
	Destroy();
}

void CEqUI_Panel::InitFromKeyValues( kvkeybase_t* pSection )
{
	// TODO: initialize scheme of GUIs
}

void CEqUI_Panel::Destroy()
{
	ClearChilds(true);
}

void IEqUIControl::AddChild(IEqUIControl* pControl)
{
	m_childs.addFirst(pControl);
	pControl->m_parent = this;
}

void IEqUIControl::RemoveChild(IEqUIControl* pControl, bool destroy)
{
	if(m_childs.goToFirst())
	{
		do
		{
			if(m_childs.getCurrent() == pControl)
			{
				pControl->m_parent = NULL;

				if(destroy)
					delete pControl;

				m_childs.removeCurrent();
				return;
			}
		}
		while(m_childs.goToNext());
	}
}

// returns child control
IEqUIControl* IEqUIControl::FindChild(const char* pszName)
{
	DkLinkedListIterator<IEqUIControl*> iter(m_childs);

	if(iter.goToFirst())
	{
		do
		{
			if(!strcmp(iter.getCurrent()->GetName(), pszName))
				return iter.getCurrent();
		}
		while(iter.goToNext());
	}

	return NULL;
}

void IEqUIControl::ClearChilds(bool destroy)
{
	if(m_childs.goToFirst())
	{
		do
		{
			m_childs.getCurrent()->m_parent = NULL;

			if(destroy)
				delete m_childs.getCurrent();

			m_childs.setCurrent(NULL);
		}
		while(m_childs.goToNext());
	}

	m_childs.clear();
}

void CEqUI_Panel::SetColor(const ColorRGBA &color)
{
	m_color = color;
}

void CEqUI_Panel::GetColor(ColorRGBA &color) const
{
	color = m_color;
}

void CEqUI_Panel::SetSelectionColor(const ColorRGBA &color)
{
	m_selColor = color;
}

void CEqUI_Panel::GetSelectionColor(ColorRGBA &color) const
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

// rendering function
void CEqUI_Panel::Render()
{
	if(!m_visible)
		return;

	// draw self if it has parent
	if( m_parent )
	{
		IRectangle myRect(m_position, m_position+m_size);
		g_pShaderAPI->SetScissorRectangle( myRect );

		DrawAlphaFilledRectangle(myRect, m_color, ColorRGBA(m_color.xyz(), 1.0f) );
	}

	// do it recursively
	if(m_childs.goToFirst())
	{
		do
		{
			m_childs.getCurrent()->Render();
		}
		while(m_childs.goToNext());
	}
}

bool CEqUI_Panel::ProcessMouseEvents(float x, float y, int nMouseButtons, int flags)
{
	if(!m_visible)
		return false;

	IRectangle rct = GetRectangle();

	if( !rct.IsInRectangle(IVector2D(x,y)) )
		return false;

	if(flags & UIEVENT_MOUSE_MOVE)
	{
		return true;
	}

	return true;
}

bool CEqUI_Panel::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	if(!m_visible)
		return false;

	return true;
}

bool CEqUI_Panel::ProcessCommand(const char* pszCommand)
{
	/*
	if(m_pParent && m_pParent->ProcessCommand(pszCommand))
		return true;

	return ProcessCommandExecute(pszCommand);
	*/

	// TODO: make it better

	return false;
}

bool CEqUI_Panel::ProcessCommandExecute( const char* pszCommand )
{
	// no commands

	return false;
}
