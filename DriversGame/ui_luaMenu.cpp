//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lua Menu
//////////////////////////////////////////////////////////////////////////////////

#include "ui_luaMenu.h"

CLuaMenu::CLuaMenu()
{
	m_numElems = 0;
	m_selection = 0;
}

void CLuaMenu::SetMenuObject(OOLUA::Table& tabl)
{
	m_menuStack = tabl;

	lua_State* state = GetLuaState();

	m_stackGetTitleToken.Get(m_menuStack, "GetTitleToken");
	m_stackReset.Get(m_menuStack, "Reset");
	m_stackGetCurMenu.Get(m_menuStack, "GetCurrentMenu");
	m_stackGetTop.Get(m_menuStack, "GetTop");
	m_stackPush.Get(m_menuStack, "Push");
	m_stackPop.Get(m_menuStack, "Pop");
	m_stackCanPop.Get(m_menuStack, "CanPop");

	if( m_stackReset.Push() && m_stackReset.Call(0, 1) )
	{
		OOLUA::Table stackTop;
		OOLUA::pull( state, stackTop );

		SetMenuStackTop( stackTop );
	}
}

void CLuaMenu::SetMenuStackTop(OOLUA::Table& tabl)
{
	lua_State* state = GetLuaState();

	if( m_stackGetCurMenu.Push() && m_stackGetCurMenu.Call(0, 1) )
	{
		OOLUA::Table menuTable;
		OOLUA::pull( state, menuTable );

		SetMenuTable( menuTable );
	}
}

void CLuaMenu::PushMenu(OOLUA::Table& tabl)
{
	lua_State* state = GetLuaState();

	if( m_stackPush.Push() && OOLUA::push(state, tabl) && m_stackPush.Call(1, 1) )
	{
		OOLUA::Table newStackTop;
		OOLUA::pull( state, newStackTop );

		SetMenuStackTop( newStackTop );
	}
}

void CLuaMenu::PopMenu()
{
	if(!IsCanPopMenu())
		return;

	lua_State* state = GetLuaState();

	if( m_stackPop.Push() && m_stackPop.Call(0, 1) )
	{
		OOLUA::Table newStackTop;
		OOLUA::pull( state, newStackTop );

		SetMenuStackTop( newStackTop );
	}
}

bool CLuaMenu::IsCanPopMenu()
{
	lua_State* state = GetLuaState();

	if( m_stackCanPop.Push() && m_stackCanPop.Call(0, 1) )
	{
		bool canPop = false;
		OOLUA::pull( state, canPop );

		return canPop;
	}

	return false;
}

bool CLuaMenu::PreEnterSelection()
{
	OOLUA::Table elem;
	if( m_menuElems.safe_at(m_selection+1, elem) )
	{
		bool isFinal = false;
		elem.safe_at("isFinal", isFinal);

		OnEnterSelection( isFinal );
		return true;
	}

	return false;
}

bool CLuaMenu::ChangeSelection(int dir)
{
	lua_State* state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	OOLUA::Table elem;
	if( m_menuElems.safe_at(m_selection+1, elem) )
	{
		EqLua::LuaTableFuncRef onChange;
		if(onChange.Get(elem, "onChange", true) && onChange.Push() && OOLUA::push(state,dir) && onChange.Call(1, 1))
		{
			return true;
		}
	}

	return false;
}

ILocToken* CLuaMenu::GetMenuTitleToken()
{
	lua_State* state = GetLuaState();

	if( m_stackGetTitleToken.Push() && m_stackGetTitleToken.Call(0, 1) )
	{
		ILocToken* tok = NULL;
		OOLUA::pull( state, tok );

		return tok;
	}

	return NULL;
}

void CLuaMenu::EnterSelection()
{
	lua_State* state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	OOLUA::Table elem;
	if( m_menuElems.safe_at(m_selection+1, elem) )
	{
		EqLua::LuaTableFuncRef onEnter;
		if(onEnter.Get(elem, "onEnter", false) && onEnter.Push() && onEnter.Call(1, 1))
		{
			if( onEnter.Push() && onEnter.Call(0, 1) )
			{
				if( lua_type(state, -1) == LUA_TTABLE )
				{
					OOLUA::Table params;
					OOLUA::pull( state, params );

					std::string commandStr = "";
					if(params.safe_at("command", commandStr))
						OnMenuCommand( commandStr.c_str() );

					if(params.safe_at("menuCommand", commandStr))
					{
						if(!stricmp(commandStr.c_str(), "Pop"))
							PopMenu();
					}
	
					OOLUA::Table nextMenu;
					if( params.safe_at("nextMenu", nextMenu))
						PushMenu( nextMenu );

					std::string newTitleToken = "";
					if(params.safe_at("titleToken", newTitleToken))
					{
						Msg("Title token set\n");
						m_menuTitleToken = g_localizer->GetToken(newTitleToken.c_str());
					}
				}
			}
		}
	}
}

void CLuaMenu::SetMenuTable( OOLUA::Table& tabl )
{
	m_menuElems = tabl;

	m_selection = 0;
	m_numElems = 0;

	EqLua::LuaStackGuard g(GetLuaState());

	oolua_ipairs(m_menuElems)
		m_numElems++;
	oolua_ipairs_end()

	m_menuTitleToken = GetMenuTitleToken();
}
