//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Title screen
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATE_TITLE_H
#define STATE_TITLE_H

#include "DrvSynStates.h"

class CState_Title : public CBaseStateHandler
{
public:
				CState_Title();
				~CState_Title();

	int			GetType() const {return GAME_STATE_TITLESCREEN;}

	void		OnEnter( CBaseStateHandler* from );
	void		OnLeave( CBaseStateHandler* to );

	bool		Update( float fDt );

	void		GoToMainMenu();

	void		HandleKeyPress( int key, bool down );
	void		HandleMouseClick(int x, int y, int buttons, bool down);

	void		HandleMouseMove( int x, int y, float deltaX, float deltaY ) {}
	void		HandleMouseWheel(int x,int y,int scroll) {}

	void		HandleJoyAxis( short axis, short value ) {}

protected:

	float		m_fade;
	bool		m_goesFromTitle;

	float		m_actionTimeout;

	float		m_textEffect;

	int			m_codePos;
	int			m_codeKeysEntered[16];

	int					m_demoId;
	DkList<EqString>	m_demoList;

	equi::IUIControl*	m_titleText;
	equi::IUIControl*	m_uiLayout;

	IVector2D			m_titlePos;
};

extern CState_Title* g_State_Title;

#endif // STATE_TITLE_H