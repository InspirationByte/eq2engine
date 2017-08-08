//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI manager
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Manager.h"
#include "core_base_header.h"
#include "in_keys_ident.h"
#include "FontCache.h"

//-----
// include all needed elements here
#include "EqUI_Panel.h"

DECLARE_CMD(ui_showpanel, NULL, CV_CHEAT)
{
	if(CMD_ARGC == 0)
		return;

	equi::Panel* pPanel = equi::Manager->FindPanel( CMD_ARGV(0).c_str() );

	if( pPanel )
		pPanel->Show();
	else
		MsgWarning("No such panel '%s'\n", CMD_ARGV(0).c_str());
}

DECLARE_CMD(ui_hidepanel, NULL, CV_CHEAT)
{
	if(CMD_ARGC == 0)
		return;

	equi::Panel* pPanel = equi::Manager->FindPanel( CMD_ARGV(0).c_str() );

	if( pPanel )
		pPanel->Hide();
	else
		MsgWarning("No such panel '%s'\n", CMD_ARGV(0).c_str());
}

DECLARE_CMD(ui_listpanels, NULL, CV_CHEAT)
{
	equi::Manager->DumpPanelsToConsole();
}

namespace equi
{

CUIManager::CUIManager() : m_keyboardFocus(NULL), m_rootPanel(NULL), m_defaultFont(NULL), m_mousePos(0)
{
}

CUIManager::~CUIManager()
{
	Shutdown();
}

void CUIManager::Init()
{
	EQUI_REGISTER_CONTROL(panel);
	EQUI_REGISTER_CONTROL(label);
	EQUI_REGISTER_CONTROL(button);
	EQUI_REGISTER_CONTROL(image);

	m_rootPanel = (equi::Panel*)CreateElement("Panel");
	m_rootPanel->SetName("equi_root");
	m_rootPanel->Show();

	m_defaultFont = g_fontCache->GetFont("default", 22);

	// TODO: EqUIScheme.res
}

void CUIManager::Shutdown()
{
	// all childs will be cleaned up
	delete m_rootPanel;
	m_rootPanel = NULL;

	m_panels.clear();
}

Panel* CUIManager::GetRootPanel() const
{
	return m_rootPanel;
}

// the element loader
void CUIManager::RegisterFactory(const char* name, EQUICONTROLFACTORYFN factory)
{
	for(int i = 0; i < m_controlFactory.numElem(); i++)
	{
		if(!stricmp(m_controlFactory[i].name, name))
		{
			ASSERTMSG(false, varargs("UI factory '%s' was already registered!", name));
			return;
		}
	}

	ctrlFactory_t fac;
	fac.factory = factory;
	fac.name = name;

	m_controlFactory.append(fac);
}

// the element loader
IUIControl* CUIManager::CreateElement( const char* type )
{
	for(int i = 0; i < m_controlFactory.numElem(); i++)
	{
		if(!stricmp(m_controlFactory[i].name, type))
			return (*m_controlFactory[i].factory)();
	}

	MsgError("EqUI: unknown element type '%s'!!!\n", type);

	return NULL;
}

void CUIManager::AddPanel(Panel* panel)
{
	m_panels.append( panel );
	m_rootPanel->AddChild(panel);
}

void CUIManager::DestroyPanel( Panel* panel )
{
	if(!panel)
		return;

	m_rootPanel->RemoveChild(panel, false);
	m_panels.fastRemove(panel);

	delete panel;
}

Panel* CUIManager::FindPanel( const char* pszPanelName ) const
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

void CUIManager::BringToTop( equi::Panel* panel )
{
	if(m_rootPanel->m_childs.goToFirst())
	{
		do
		{
			if(m_rootPanel->m_childs.getCurrent() == panel)
			{
				m_rootPanel->m_childs.moveCurrentToTop();
				return;
			}
		}
		while(m_rootPanel->m_childs.goToNext());
	}
}

equi::Panel* CUIManager::GetTopPanel() const
{
	if(m_rootPanel->m_childs.goToFirst())
		return (equi::Panel*)m_rootPanel->m_childs.getCurrent();

	return NULL;
}

void CUIManager::DumpPanelsToConsole()
{
	MsgInfo("Available UI panels/windows:\n");

	for(int i = 0; i < m_panels.numElem(); i++)
	{
		if(strlen(m_panels[i]->GetName()) > 0)
		{
			Msg("%s (%s)\n", m_panels[i]->GetName(), m_panels[i]->IsVisible() ? "visible" : "hidden");
		}
	}
}

void CUIManager::SetViewFrame(const IRectangle& rect)
{
	m_viewFrameRect = rect;
}

const IRectangle& CUIManager::GetViewFrame() const
{
	return m_viewFrameRect;
}

void CUIManager::SetFocus( IUIControl* focusTo )
{
	m_keyboardFocus = focusTo;
}

IUIControl* CUIManager::GetFocus() const
{
	return m_keyboardFocus;
}

IUIControl* CUIManager::GetMouseOver() const
{
	return m_mouseOver;
}

bool CUIManager::IsWindowsVisible() const
{
	for(int i = 0; i < m_panels.numElem(); i++)
	{
		if(m_panels[i]->IsVisible() && m_panels[i]->m_windowControls)
			return true;
	}

	return false;
}

void CUIManager::Render()
{
	if(!m_rootPanel)
		return;

	// begin from the render panel
	m_rootPanel->SetRectangle( m_viewFrameRect );
	m_rootPanel->Render();
}

equi::Panel* CUIManager::GetPanelByElement(IUIControl* control)
{
	IUIControl* firstCtrl = control;

	while(firstCtrl && m_panels.findIndex((equi::Panel*)firstCtrl) == -1)
	{
		firstCtrl = firstCtrl->GetParent();
	}

	return (equi::Panel*)firstCtrl;
}

bool CUIManager::ProcessMouseEvents(float x, float y, int nMouseButtons, int flags)
{
	if(!IsWindowsVisible())
		return false;

	IVector2D mousePos(x,y);

	bool changeFocus = !(flags & UIEVENT_MOUSE_MOVE);

	IUIControl* oldMouseOver = m_mouseOver;
	m_mouseOver = m_rootPanel->HitTest(IVector2D(x,y));

	if(nMouseButtons == MOU_B1)
	{
		if(flags & UIEVENT_UP)
			m_keyboardFocus = m_mouseOver;

		// also set panel focus
		equi::Panel* panel = GetPanelByElement(m_mouseOver);
		BringToTop( panel );
	}


	if(oldMouseOver != m_mouseOver)
	{
		if(oldMouseOver)
			oldMouseOver->ProcessMouseEvents(x, y, 0, UIEVENT_MOUSE_OUT | UIEVENT_MOUSE_MOVE);

		if(m_mouseOver)
			m_mouseOver->ProcessMouseEvents(x, y, 0, UIEVENT_MOUSE_IN | UIEVENT_MOUSE_MOVE);
	}

	if( changeFocus ) // focus only when mouse is not moved
	{
		if(m_mouseOver == m_rootPanel) // remove focus if we clicked on root panel
			m_keyboardFocus = NULL;
		else
			m_keyboardFocus = m_mouseOver;
	}

	bool result = false;

	if( m_mouseOver )
		result = m_mouseOver->ProcessMouseEvents(mousePos, mousePos-m_mousePos, nMouseButtons, flags);

	m_mousePos = mousePos;

	return result;
}

bool CUIManager::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	if( nKeyButtons == KEY_TILDE )
		return false;

	if(!IsWindowsVisible())
		return false;

	if(!m_keyboardFocus)
		return false;

	if(!m_keyboardFocus->IsVisible())
		return false;

	return m_keyboardFocus->ProcessKeyboardEvents(nKeyButtons, flags);
}

static CUIManager s_eqUIManager;
CUIManager*	Manager = &s_eqUIManager;


};
