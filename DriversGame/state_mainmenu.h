//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Title screen
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATE_MAINMENU_H
#define STATE_MAINMENU_H

#include "ui_luaMenu.h"

#include "DrvSynStates.h"

class CState_MainMenu : public CBaseStateHandler, public CLuaMenu
{
public:
				CState_MainMenu();
				~CState_MainMenu();

	int			GetType() const {return GAME_STATE_MAINMENU;}

	void		OnEnter( CBaseStateHandler* from );
	void		OnLeave( CBaseStateHandler* to );

	bool		Update( float fDt );

	void		HandleKeyPress( int key, bool down );

	void		Event_SelectionEnter(const char* actionName = "onEnter");
	void		Event_BackToPrevious();
	void		Event_SelectionUp();
	void		Event_SelectionDown();
	void		Event_SelectMenuItem(int index);

	void		HandleMouseMove( int x, int y, float deltaX, float deltaY );
	void		HandleMouseClick( int x, int y, int buttons, bool down );
	void		HandleMouseWheel(int x, int y, int scroll);

	void		HandleJoyAxis(short axis, short value);

	void		GetMouseCursorProperties(bool &visible, bool& centered) {visible = true, centered = false;}

	//------------------------------------------------------

	void		OnEnterSelection( bool isFinal );
	void		OnMenuCommand(const char* command);

protected:

	void		ResetScroll();

	void		ResetKeys();
	bool		MapKeysToCurrentAction();

	void		GetEnteredKeysString(EqString& keysStr);

	float					m_fade;
	float					m_textFade;
	bool					m_goesFromMenu;
	int						m_menuMode;

	int						m_keysPos;
	int						m_keysEntered[16];
	bool					m_keysError;

	float					m_textEffect;

	float					m_menuScrollInterp;
	float					m_menuScrollTarget;

	equi::IUIControl*		m_uiLayout;
	equi::IUIControl*		m_menuDummy;
	equi::IUIControl*		m_menuHeaderDummy;

	int						m_maxMenuItems;
};

extern CState_MainMenu* g_State_MainMenu;

#endif // STATE_MAINMENU_H
