//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

	void					EnterSelection(const char* actionFunc = "onEnter");
	bool					PreEnterSelection();
	bool					ChangeSelection(int dir);

	bool					GetCurrentMenuElement(OOLUA::Table& elem);

	virtual void			OnEnterSelection( bool isFinal ) = 0;
	virtual void			OnMenuCommand( const char* command ) {}

	void					UpdateMenu(float fDt);

protected:

	const wchar_t*			GetMenuItemString(OOLUA::Table& menuElem);

	void					SetMenuStack(OOLUA::Table& tabl);
	void					PushMenu(OOLUA::Table& paramsTable);

	virtual void			UpdateCurrentMenu();

	OOLUA::Table			m_menuStack;

	EqLua::LuaTableFuncRef	m_stackInitItems;
	EqLua::LuaTableFuncRef	m_stackPush;
	EqLua::LuaTableFuncRef	m_stackPop;
	EqLua::LuaTableFuncRef	m_stackReset;
	EqLua::LuaTableFuncRef	m_stackUpdate;
	EqLua::LuaTableFuncRef	m_updateUIScheme;

	OOLUA::Table			m_menuElems;
	int						m_selection;
	int						m_numElems;

	EqWString				m_menuTitleStr;


};

#endif // UI_LUAMENU_H