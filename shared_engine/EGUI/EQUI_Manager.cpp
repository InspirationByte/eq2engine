//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#include "EQUI_Manager.h"
#include "core_base_header.h"
#include "in_keys_ident.h"

//-----
// include all needed elements here
#include "EqUI_Panel.h"
//#include "EqUI_Label.h"
//#include "EqUI_Button.h"
//#include "EqUI_Slider.h"
//#include "EqUI_TextureFrame.h"
//-----

DECLARE_CMD(equi_showpanel, "Shows panel", CV_CHEAT)
{
	if(args->numElem() == 0)
		return;

	CEqPanel* pPanel = g_pEqUIManager->FindPanel( args->ptr()[0].GetData() );

	if( g_pEqUIManager->GetRootPanel() && (g_pEqUIManager->GetRootPanel() != pPanel) )
	{
		g_pEqUIManager->GetRootPanel()->ClearControls( false );

		g_pEqUIManager->GetRootPanel()->AddControl( pPanel );
	}
}

DECLARE_CMD(equi_listpanels, "Dumps panel names to console", CV_CHEAT)
{
	g_pEqUIManager->DumpPanelsToConsole();
}

CEqUI_Manager::CEqUI_Manager()
{
	m_pRootPanel = NULL;
}

CEqUI_Manager::~CEqUI_Manager()
{
	Shutdown();
}

void CEqUI_Manager::Init()
{
	m_pRootPanel = NULL;
}

void CEqUI_Manager::Shutdown()
{
	m_pRootPanel = NULL;

	for(int i = 0; i < m_AllocatedPanels.numElem(); i++)
	{
		delete m_AllocatedPanels[i];
	}

	m_AllocatedPanels.clear();
}

CEqPanel* CEqUI_Manager::GetRootPanel()
{
	return m_pRootPanel;
}

void CEqUI_Manager::SetRootPanel(CEqPanel* pPanel)
{
	m_pRootPanel = pPanel;
}

// the element loader
CEqPanel* CEqUI_Manager::CreateElement( const char* pszTypeName )
{
	EqUI_Type_e resolvedType = EqUI_ResolveElementTypeString( pszTypeName );

	if(resolvedType == EQUI_INVALID)
	{
		MsgError("EqUI: unknown element type '%s'!!!\n", pszTypeName);
		return NULL;
	}

	CEqPanel* pElement = NULL;

	switch(resolvedType)
	{
		case EQUI_PANEL:
			pElement = new CEqPanel();
		case EQUI_BUTTON:
			pElement = new CEqPanel();	// TODO: button
	}

	if(pElement != NULL)
		m_AllocatedPanels.append( pElement );

	return pElement;
}

void CEqUI_Manager::DestroyElement( CEqPanel* pPanel )
{
	if(!pPanel)
		return;

	pPanel->Shutdown();

	delete pPanel;

	m_AllocatedPanels.remove(pPanel);

	if(m_pRootPanel == pPanel)
		m_pRootPanel = NULL;
}

CEqPanel* CEqUI_Manager::FindPanel( const char* pszPanelName )
{
	for(int i = 0; i < m_AllocatedPanels.numElem(); i++)
	{
		if(strlen(m_AllocatedPanels[i]->GetName()) > 0)
		{
			if(!stricmp(pszPanelName, m_AllocatedPanels[i]->GetName()))
				return m_AllocatedPanels[i];
		}
	}

	return NULL;
}

void CEqUI_Manager::DumpPanelsToConsole()
{
	for(int i = 0; i < m_AllocatedPanels.numElem(); i++)
	{
		if(strlen(m_AllocatedPanels[i]->GetName()) > 0)
		{
			Msg("%s\n", m_AllocatedPanels[i]->GetName());
		}
	}
}

void CEqUI_Manager::SetViewFrame(const IRectangle& rect)
{
	m_viewFrameRect = rect;
}

IRectangle	CEqUI_Manager::GetViewFrame()
{
	return m_viewFrameRect;
}

void CEqUI_Manager::Render()
{
	if(!m_pRootPanel)
		return;

	m_pRootPanel->SetRect( m_viewFrameRect );

	m_pRootPanel->Render();
}

bool CEqUI_Manager::ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags)
{
	if(!m_pRootPanel)
		return true;

	if(!m_pRootPanel->IsVisible())
		return true;

	return m_pRootPanel->ProcessMouseEvents(x, y, nMouseButtons, nMouseFlags);
}

bool  CEqUI_Manager::ProcessKeyboardEvents(int nKeyButtons, int nKeyFlags)
{
	if(!m_pRootPanel)
		return true;

	if(!m_pRootPanel->IsVisible())
		return true;

	if(nKeyButtons == KEY_TILDE ||
		nKeyButtons == KEY_ESCAPE)
		return true;

	return m_pRootPanel->ProcessKeyboardEvents(nKeyButtons, nKeyFlags);
}

static CEqUI_Manager s_eqUIManager;
CEqUI_Manager*	g_pEqUIManager = &s_eqUIManager;