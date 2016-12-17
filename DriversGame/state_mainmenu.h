//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Title screen
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATE_MAINMENU_H
#define STATE_MAINMENU_H

#include "ui_luaMenu.h"

#include "StateManager.h"

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

	void		Event_SelectionEnter();
	void		Event_BackToPrevious();
	void		Event_SelectionUp();
	void		Event_SelectionDown();
	void		Event_SelectMenuItem(int index);

	void		HandleMouseMove( int x, int y, float deltaX, float deltaY );
	void		HandleMouseClick( int x, int y, int buttons, bool down );
	void		HandleMouseWheel(int x,int y,int scroll) {}

	void		HandleJoyAxis( short axis, short value ) {}

	bool		IsMouseCursorVisible() {return true;}

	//------------------------------------------------------

	void		OnEnterSelection( bool isFinal );

protected:

	float					m_fade;
	float					m_textFade;
	bool					m_goesFromMenu;
	int						m_changesMenu;

	float					m_textEffect;

	ITexture*				m_titleTexture;
};

extern CState_MainMenu* g_State_MainMenu;

#endif // STATE_MAINMENU_H
