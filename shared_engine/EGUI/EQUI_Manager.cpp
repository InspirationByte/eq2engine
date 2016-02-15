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

	CEqUI_Panel* pPanel = g_pEqUIManager->FindPanel( args->ptr()[0].GetData() );

	if( g_pEqUIManager->GetRootPanel() && (g_pEqUIManager->GetRootPanel() != pPanel) )
	{
		g_pEqUIManager->GetRootPanel()->ClearChilds( false );

		g_pEqUIManager->GetRootPanel()->AddChild( pPanel );
	}
}

DECLARE_CMD(equi_listpanels, "Dumps panel names to console", CV_CHEAT)
{
	g_pEqUIManager->DumpPanelsToConsole();
}

CEqUI_Manager::CEqUI_Manager()
{
	m_rootPanel = NULL;
}

CEqUI_Manager::~CEqUI_Manager()
{
	Shutdown();
}

void CEqUI_Manager::Init()
{
	m_rootPanel = NULL;
}

void CEqUI_Manager::Shutdown()
{
	m_rootPanel = NULL;

	for(int i = 0; i < m_allocatedPanels.numElem(); i++)
	{
		delete m_allocatedPanels[i];
	}

	m_allocatedPanels.clear();
}

CEqUI_Panel* CEqUI_Manager::GetRootPanel()
{
	return m_rootPanel;
}

void CEqUI_Manager::SetRootPanel(CEqUI_Panel* pPanel)
{
	m_rootPanel = pPanel;
}

// the element loader
CEqUI_Panel* CEqUI_Manager::CreateElement( const char* pszTypeName )
{
	EqUI_Type_e resolvedType = EqUI_ResolveElementTypeString( pszTypeName );

	if(resolvedType == EQUI_INVALID)
	{
		MsgError("EqUI: unknown element type '%s'!!!\n", pszTypeName);
		return NULL;
	}

	CEqUI_Panel* pElement = NULL;

	switch(resolvedType)
	{
		case EQUI_PANEL:
			pElement = new CEqUI_Panel();
		case EQUI_BUTTON:
			pElement = new CEqUI_Panel();	// TODO: button
	}

	if(pElement != NULL)
		m_allocatedPanels.append( pElement );

	return pElement;
}

void CEqUI_Manager::DestroyElement( CEqUI_Panel* pPanel )
{
	if(!pPanel)
		return;

	pPanel->Shutdown();

	delete pPanel;

	m_allocatedPanels.remove(pPanel);

	if(m_rootPanel == pPanel)
		m_rootPanel = NULL;
}

CEqUI_Panel* CEqUI_Manager::FindPanel( const char* pszPanelName )
{
	for(int i = 0; i < m_allocatedPanels.numElem(); i++)
	{
		if(strlen(m_allocatedPanels[i]->GetName()) > 0)
		{
			if(!stricmp(pszPanelName, m_allocatedPanels[i]->GetName()))
				return m_allocatedPanels[i];
		}
	}

	return NULL;
}

void CEqUI_Manager::DumpPanelsToConsole()
{
	for(int i = 0; i < m_allocatedPanels.numElem(); i++)
	{
		if(strlen(m_allocatedPanels[i]->GetName()) > 0)
		{
			Msg("%s\n", m_allocatedPanels[i]->GetName());
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
	if(!m_rootPanel)
		return;

	m_rootPanel->SetRect( m_viewFrameRect );

	m_rootPanel->Render();
}

bool CEqUI_Manager::ProcessMouseEvents(float x, float y, int nMouseButtons, int nMouseFlags)
{
	if(!m_rootPanel)
		return true;

	if(!m_rootPanel->IsVisible())
		return true;

	return m_rootPanel->ProcessMouseEvents(x, y, nMouseButtons, nMouseFlags);
}

bool  CEqUI_Manager::ProcessKeyboardEvents(int nKeyButtons, int nKeyFlags)
{
	if(!m_rootPanel)
		return true;

	if(!m_rootPanel->IsVisible())
		return true;

	if(nKeyButtons == KEY_TILDE ||
		nKeyButtons == KEY_ESCAPE)
		return true;

	return m_rootPanel->ProcessKeyboardEvents(nKeyButtons, nKeyFlags);
}

static CEqUI_Manager s_eqUIManager;
CEqUI_Manager*	g_pEqUIManager = &s_eqUIManager;