//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Panel.h"
#include "EQUI_Manager.h"
#include "materialsystem/imaterialsystem.h"

CEqPanel::CEqPanel()
{
	m_vPosition = IVector2D(0);
	m_vSize = IVector2D(32,32);

	m_bVisible = true;
	m_bEnabled = true;
	m_szName = "";

	m_pParent = NULL;

	m_vColor = ColorRGBA(0,0,0, 0.5f);
	m_vSelColor = ColorRGBA(0.25f);

	m_nCurrentElement = NULL;
}

void CEqPanel::InitFromKeyValues( kvkeybase_t* pSection )
{
	// TODO: initialize scheme of GUIs
}

void CEqPanel::Shutdown()
{
	ClearControls();
}

void CEqPanel::AddControl(CEqPanel* pControl)
{
	m_ChildPanels.addFirst(pControl);
	pControl->SetParent(this);
}

void CEqPanel::RemoveControl(CEqPanel* pControl)
{
	if(m_ChildPanels.goToFirst())
	{
		do
		{
			if(m_ChildPanels.getCurrent() == pControl)
			{
				pControl->SetParent(NULL);

				g_pEqUIManager->DestroyElement(pControl);

				m_ChildPanels.removeCurrent();
				return;
			}
		}
		while(m_ChildPanels.goToNext());
	}
}

// returns child control 
CEqPanel* CEqPanel::GetControl(const char* pszName)
{
	DkLinkedListIterator<CEqPanel*> iter(m_ChildPanels);

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

void CEqPanel::ClearControls(bool bFree)
{
	if(m_ChildPanels.goToFirst())
	{
		do
		{
			m_ChildPanels.getCurrent()->SetParent(NULL);

			if(bFree)
				g_pEqUIManager->DestroyElement(m_ChildPanels.getCurrent());

			m_ChildPanels.setCurrent(NULL);
		}
		while(m_ChildPanels.goToNext());
	}

	m_ChildPanels.clear();
}

char* CEqPanel::GetName()
{
	return (char*)m_szName.GetData();
}

void CEqPanel::SetName(const char* pszName)
{
	m_szName = pszName;
}

void CEqPanel::SetColor(const ColorRGBA &color)
{
	m_vColor = color;
}

void CEqPanel::GetColor(ColorRGBA &color)
{
	color = m_vColor;
}

void CEqPanel::SetSelectionColor(const ColorRGBA &color)
{
	m_vSelColor = color;
}

void CEqPanel::GetSelectionColor(ColorRGBA &color)
{
	color = m_vSelColor;
}

void CEqPanel::Show()
{
	m_bVisible = true;
}

void CEqPanel::Hide()
{
	m_bVisible = false;
}

void CEqPanel::SetVisible(bool bVisible)
{
	m_bVisible = bVisible;
}

bool CEqPanel::IsVisible()
{
	return m_bVisible;
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
void CEqPanel::Render()
{
	if(!m_bVisible)
		return;

	// draw self if it has parent
	if(m_pParent)
	{
		IRectangle myRect(m_vPosition, m_vPosition+m_vSize);
		DrawAlphaFilledRectangle(myRect, m_vColor, ColorRGBA(m_vColor.xyz(), 1.0f) );
	}

	// do it recursively
}

void CEqPanel::Enable()
{
	m_bEnabled = true;
}

void CEqPanel::Disable()
{
	m_bEnabled = false;
}

bool CEqPanel::IsEnabled()
{
	return m_bEnabled;
}

bool CEqPanel::ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags)
{

	return false;
}

bool CEqPanel::ProcessKeyboardEvents(int nKeyButtons, int nKeyFlags)
{
	return true;
}

void CEqPanel::SetSize(const IVector2D &size)
{
	m_vSize = size;
}

void CEqPanel::SetPosition(const IVector2D &pos)
{
	m_vPosition = pos;
}

void CEqPanel::SetRect(const IRectangle& rect)
{
	m_vPosition = rect.vleftTop;
	m_vSize = rect.vrightBottom - m_vPosition;
}

void CEqPanel::GetSize(IVector2D& size)
{
	size = m_vSize;
}

void CEqPanel::GetPosition(IVector2D& pos)
{
	pos = m_vPosition;
}

// clipping rectangle, size position
IRectangle	CEqPanel::GetRectangle()
{
	return IRectangle(m_vPosition, m_vPosition + m_vSize);
}

bool CEqPanel::ProcessCommand(const char* pszCommand)
{
	/*
	if(m_pParent && m_pParent->ProcessCommand(pszCommand))
		return true;

	return ProcessCommandExecute(pszCommand);
	*/

	// TODO: make it better

	return false;
}

bool CEqPanel::ProcessCommandExecute( const char* pszCommand )
{
	// no commands

	return false;
}

void CEqPanel::SetParent(CEqPanel* pParent)
{
	m_pParent = pParent;
}

CEqPanel* CEqPanel::GetParent()
{
	return m_pParent;
}