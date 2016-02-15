//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Panel.h"
#include "EQUI_Manager.h"
#include "materialsystem/imaterialsystem.h"

CEqUI_Panel::CEqUI_Panel() : m_visible(true), m_enabled(true), m_parent(NULL), m_currentElement(NULL)
{
	m_position = IVector2D(0);
	m_size = IVector2D(32,32);

	m_color = ColorRGBA(0,0,0, 0.5f);
	m_selColor = ColorRGBA(0.25f);
}

void CEqUI_Panel::InitFromKeyValues( kvkeybase_t* pSection )
{
	// TODO: initialize scheme of GUIs
}

void CEqUI_Panel::Shutdown()
{
	ClearChilds();
}

void CEqUI_Panel::AddChild(CEqUI_Panel* pControl)
{
	m_childPanels.addFirst(pControl);
	pControl->m_parent = this;
}

void CEqUI_Panel::RemoveChild(CEqUI_Panel* pControl)
{
	if(m_childPanels.goToFirst())
	{
		do
		{
			if(m_childPanels.getCurrent() == pControl)
			{
				pControl->m_parent = NULL;

				g_pEqUIManager->DestroyElement(pControl);

				m_childPanels.removeCurrent();
				return;
			}
		}
		while(m_childPanels.goToNext());
	}
}

// returns child control 
CEqUI_Panel* CEqUI_Panel::GetControl(const char* pszName)
{
	DkLinkedListIterator<CEqUI_Panel*> iter(m_childPanels);

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

void CEqUI_Panel::ClearChilds(bool bFree)
{
	if(m_childPanels.goToFirst())
	{
		do
		{
			m_childPanels.getCurrent()->m_parent = NULL;

			if(bFree)
				g_pEqUIManager->DestroyElement(m_childPanels.getCurrent());

			m_childPanels.setCurrent(NULL);
		}
		while(m_childPanels.goToNext());
	}

	m_childPanels.clear();
}

const char* CEqUI_Panel::GetName() const
{
	return m_name.GetData();
}

void CEqUI_Panel::SetName(const char* pszName)
{
	m_name = pszName;
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

void CEqUI_Panel::Show()
{
	m_visible = true;
}

void CEqUI_Panel::Hide()
{
	m_visible = false;
}

void CEqUI_Panel::SetVisible(bool bVisible)
{
	m_visible = bVisible;
}

bool CEqUI_Panel::IsVisible() const
{
	return m_visible;
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
	if(m_parent)
	{
		IRectangle myRect(m_position, m_position+m_size);
		DrawAlphaFilledRectangle(myRect, m_color, ColorRGBA(m_color.xyz(), 1.0f) );
	}

	// do it recursively
}

void CEqUI_Panel::Enable()
{
	m_enabled = true;
}

void CEqUI_Panel::Disable()
{
	m_enabled = false;
}

bool CEqUI_Panel::IsEnabled() const
{
	return m_enabled;
}

bool CEqUI_Panel::ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags)
{
	
	return false;
}

bool CEqUI_Panel::ProcessKeyboardEvents(int nKeyButtons, int nKeyFlags)
{

	return true;
}

void CEqUI_Panel::SetSize(const IVector2D &size)
{
	m_size = size;
}

void CEqUI_Panel::SetPosition(const IVector2D &pos)
{
	m_position = pos;
}

void CEqUI_Panel::SetRect(const IRectangle& rect)
{
	m_position = rect.vleftTop;
	m_size = rect.vrightBottom - m_position;
}

const IVector2D& CEqUI_Panel::GetSize() const
{
	return m_size;
}

const IVector2D& CEqUI_Panel::GetPosition() const
{
	return m_position;
}

// clipping rectangle, size position
IRectangle	CEqUI_Panel::GetRectangle() const
{
	return IRectangle(m_position, m_position + m_size);
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