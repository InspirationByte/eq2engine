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

DECLARE_CMD(ui_showpanel, NULL, CV_CHEAT)
{
	if(CMD_ARGC == 0)
		return;

	CEqUI_Panel* pPanel = g_pEqUIManager->FindPanel( CMD_ARGV(0).c_str() );

	if( pPanel )
		pPanel->Show();
	else
		MsgWarning("No such panel '%s'\n", CMD_ARGV(0).c_str());
}

DECLARE_CMD(ui_hidepanel, NULL, CV_CHEAT)
{
	if(CMD_ARGC == 0)
		return;

	CEqUI_Panel* pPanel = g_pEqUIManager->FindPanel( CMD_ARGV(0).c_str() );

	if( pPanel )
		pPanel->Hide();
	else
		MsgWarning("No such panel '%s'\n", CMD_ARGV(0).c_str());
}

DECLARE_CMD(ui_listpanels, NULL, CV_CHEAT)
{
	g_pEqUIManager->DumpPanelsToConsole();
}

CEqUI_Manager::CEqUI_Manager() : m_focus(NULL), m_rootPanel(NULL)
{
}

CEqUI_Manager::~CEqUI_Manager()
{
	Shutdown();
}

void CEqUI_Manager::Init()
{
	m_rootPanel = (CEqUI_Panel*)g_pEqUIManager->CreateElement("Panel");
	m_rootPanel->SetName("equi_root");
	m_rootPanel->Show();
}

void CEqUI_Manager::Shutdown()
{
	m_rootPanel = NULL;

	for(int i = 0; i < m_panels.numElem(); i++)
	{
		delete m_panels[i];
	}

	m_panels.clear();
}

CEqUI_Panel* CEqUI_Manager::GetRootPanel() const
{
	return m_rootPanel;
}

// the element loader
IEqUIControl* CEqUI_Manager::CreateElement( const char* pszTypeName )
{
	EUIElementType resolvedType = EqUI_ResolveElementTypeString( pszTypeName );

	if(resolvedType == EQUI_INVALID)
	{
		MsgError("EqUI: unknown element type '%s'!!!\n", pszTypeName);
		return NULL;
	}

	return CreateElement(resolvedType);
}

IEqUIControl* CEqUI_Manager::CreateElement( EUIElementType type )
{
	CEqUI_Panel* pElement = NULL;

	switch(type)
	{
		case EQUI_PANEL:
			pElement = new CEqUI_Panel();
		//case EQUI_BUTTON:
		//	pElement = new CEqUI_Panel();	// TODO: button
	}

	return pElement;
}

void CEqUI_Manager::AddPanel(CEqUI_Panel* panel)
{
	m_panels.append( panel );
	m_rootPanel->AddChild(panel);
}

void CEqUI_Manager::DestroyPanel( CEqUI_Panel* panel )
{
	if(!panel)
		return;

	m_rootPanel->RemoveChild(panel, false);
	m_panels.fastRemove(panel);

	delete panel;
}

CEqUI_Panel* CEqUI_Manager::FindPanel( const char* pszPanelName ) const
{
	for(int i = 0; i < m_panels.numElem(); i++)
	{
		if(strlen(m_panels[i]->GetName()) > 0)
		{
			if(!stricmp(pszPanelName, m_panels[i]->GetName()))
				return m_panels[i];
		}
	}

	return NULL;
}

void CEqUI_Manager::DumpPanelsToConsole()
{
	for(int i = 0; i < m_panels.numElem(); i++)
	{
		if(strlen(m_panels[i]->GetName()) > 0)
		{
			Msg("%s\n", m_panels[i]->GetName());
		}
	}
}

void CEqUI_Manager::SetViewFrame(const IRectangle& rect)
{
	m_viewFrameRect = rect;
}

const IRectangle& CEqUI_Manager::GetViewFrame() const
{
	return m_viewFrameRect;
}

void CEqUI_Manager::SetFocus( IEqUIControl* focusTo )
{
	m_focus = focusTo;
}

IEqUIControl* CEqUI_Manager::GetFocus() const
{
	return m_focus;
}

bool CEqUI_Manager::IsPanelsVisible() const
{
	for(int i = 0; i < m_panels.numElem(); i++)
	{
		if(m_panels[i]->IsVisible())
			return true;
	}

	return false;
}

void CEqUI_Manager::Render()
{
	if(!m_rootPanel)
		return;

	// begin from the render panel
	m_rootPanel->SetRectangle( m_viewFrameRect );
	m_rootPanel->Render();
}

bool CEqUI_Manager::ProcessMouseEvents(float x, float y, int nMouseButtons, int flags)
{
	if(!m_rootPanel)
		return false;

	if(!m_rootPanel->IsVisible())
		return false;

	if(!IsPanelsVisible())
		return false;

	return m_rootPanel->ProcessMouseEvents(x, y, nMouseButtons, flags);
}

bool  CEqUI_Manager::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	if( nKeyButtons == KEY_TILDE )
		return false;

	if(!IsPanelsVisible())
		return false;

	if(!m_focus)
		return false;

	if(!m_focus->IsVisible())
		return false;

	return m_focus->ProcessKeyboardEvents(nKeyButtons, flags);
}

static CEqUI_Manager s_eqUIManager;
CEqUI_Manager*	g_pEqUIManager = &s_eqUIManager;
