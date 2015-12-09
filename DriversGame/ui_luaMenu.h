//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lua Menu
//////////////////////////////////////////////////////////////////////////////////

#ifndef UI_LUAMENU_H
#define UI_LUAMENU_H

#include "LuaBinding_Drivers.h"

class CLuaMenu
{
public:
				CLuaMenu();
		virtual	~CLuaMenu() {}

	//------------------------------------------------------

	void					PopMenu();
	bool					IsCanPopMenu();

	void					EnterSelection();
	bool					PreEnterSelection();
	bool					ChangeSelection(int dir);

	ILocToken*				GetMenuTitleToken();

	virtual void			OnEnterSelection( bool isFinal ) = 0;
	virtual void			OnMenuCommand( const char* command ) {}

protected:

	

	void					SetMenuObject(OOLUA::Table& tabl);

	void					SetMenuStackTop(OOLUA::Table& tabl);
	void					SetMenuTable(OOLUA::Table& tabl);
	
	void					PushMenu(OOLUA::Table& tabl);

	OOLUA::Table			m_menuStack;

	EqLua::LuaTableFuncRef	m_stackReset;
	EqLua::LuaTableFuncRef	m_stackGetTop;
	EqLua::LuaTableFuncRef	m_stackGetCurMenu;
	EqLua::LuaTableFuncRef	m_stackPush;
	EqLua::LuaTableFuncRef	m_stackPop;
	EqLua::LuaTableFuncRef	m_stackCanPop;
	EqLua::LuaTableFuncRef	m_stackGetTitleToken;

	OOLUA::Table			m_menuElems;
	int						m_selection;
	int						m_numElems;

	ILocToken*				m_menuTitleToken;

};

#endif // UI_LUAMENU_H