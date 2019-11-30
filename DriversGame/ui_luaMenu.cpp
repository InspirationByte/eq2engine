//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lua Menu
//////////////////////////////////////////////////////////////////////////////////

#include "ui_luaMenu.h"
#include "KeyBinding/InputCommandBinder.h"

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
	if(!tabl.safe_at(3, m_selection))
		m_selection = 0;

	lua_State* state = GetLuaState();

	if( m_stackGetCurMenu.Push() && m_stackGetCurMenu.Call(0, 1) )
	{
		OOLUA::Table menuTable;
		OOLUA::pull( state, menuTable );

		SetMenuTable( menuTable );
	}
}

void CLuaMenu::PushMenu(OOLUA::Table& tabl, const std::string& titleToken, int selection)
{
	lua_State* state = GetLuaState();

	if( m_stackPush.Push() && 
		OOLUA::push(state, tabl) && 
		OOLUA::push(state, titleToken) &&
		OOLUA::push(state, selection) && 
		m_stackPush.Call(3, 1) )
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
	if (GetCurrentMenuElement(elem))
	{
		EqLua::LuaTableFuncRef onChange;
		if (onChange.Get(elem, "onChange", true))
			return false;

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
	if (GetCurrentMenuElement(elem))
	{
		EqLua::LuaTableFuncRef onChange;

		if(onChange.Get(elem, "onChange", true) && 
			onChange.Push() && 
			OOLUA::push(state,dir) && 
			onChange.Call(1, 1))
			return true;
	}

	return false;
}

void CLuaMenu::GetMenuTitleToken(EqWString& outText)
{
	lua_State* state = GetLuaState();

	if( m_stackGetTitleToken.Push() && m_stackGetTitleToken.Call(0, 1) )
	{
		int type = lua_type(state, -1);

		if (type == LUA_TSTRING)
		{
			std::string tok_str;
			OOLUA::pull(state, tok_str);
			outText = LocalizedString(tok_str.c_str());
		}
		else if (type == LUA_TUSERDATA)
		{
			ILocToken* tok = NULL;
			OOLUA::pull( state, tok );

			outText = tok ? tok->GetText() : L"(nulltoken)";
		}
	}
}

bool CLuaMenu::GetCurrentMenuElement(OOLUA::Table& elem)
{
	return m_menuElems.safe_at(m_selection + 1, elem);
}

void CLuaMenu::EnterSelection(const char* actionFunc /*= "onEnter"*/)
{
	lua_State* state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	OOLUA::Table elem;
	if (!GetCurrentMenuElement(elem))
		return;

	EqLua::LuaTableFuncRef actionFuncRef;
	if (!actionFuncRef.Get(elem, actionFunc, false))
		return;

	if(actionFuncRef.Push() && actionFuncRef.Call(0, 1) )
	{
		int retType = lua_type(state, -1);

		if( retType == LUA_TTABLE )
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

			std::string newTitleToken = "";
			params.safe_at("titleToken", newTitleToken);

			OOLUA::Table nextMenu;
			if (params.safe_at("nextMenu", nextMenu))
				PushMenu(nextMenu, newTitleToken, m_selection);

			GetMenuTitleToken(m_menuTitleStr);
		}
	}
	else
		MsgError("CLuaMenu::EnterSelection error:\n %s\n", OOLUA::get_last_error(state).c_str());
}

void CLuaMenu::SetMenuTable( OOLUA::Table& tabl )
{
	m_menuElems = tabl;
	GetMenuTitleToken(m_menuTitleStr);

	m_numElems = 0;

	EqLua::LuaStackGuard g(GetLuaState());

	oolua_ipairs(m_menuElems)
		m_numElems++;
	oolua_ipairs_end()
}

const wchar_t* CLuaMenu::GetMenuItemString(OOLUA::Table& menuElem)
{
	static EqWString _tempStr;
	static EqWString _tempValueStr;
	static std::string tok_str;
	tok_str = "";

	lua_State* state = menuElem.state();
	const wchar_t* resultStr = nullptr;
	ILocToken* tok = nullptr;

	if (menuElem.safe_at("label", tok))
	{
		if (!tok)
		{
			if (menuElem.safe_at("label", tok_str))
			{
				const char* tokenName = tok_str.c_str();
				if (*tokenName == '#')	// only search for token if it starts with '#', just like "LocalizedString" behaves
				{
					// this will slowdown the engine, so we need to find it and optimize
					tok = g_localizer->GetToken(tokenName+1);

					if (tok)	// store for optimization
						menuElem.set("label", tok);
				}

				_tempStr = tok_str.c_str();
				resultStr = tok ? tok->GetText() : _tempStr.c_str();
			}
		}
		else
			resultStr = tok->GetText();
	}

	if (!resultStr)
		return L"Undefined token";

	EqLua::LuaTableFuncRef labelValue;
	if (labelValue.Get(menuElem, "labelValue", true) && labelValue.Push() && labelValue.Call(0, 1))
	{
		int type = lua_type(state, -1);
		if (type == LUA_TSTRING)
		{
			OOLUA::pull(state, tok_str);

			_tempValueStr = tok_str.c_str();
			resultStr = varargs_w(resultStr, _tempValueStr.c_str());
		}
		else if (type == LUA_TNUMBER)
		{
			float val = 0;
			OOLUA::pull(state, val);

			resultStr = varargs_w(resultStr, val);
		}
		else if (type == LUA_TUSERDATA)
		{
			ILocToken* valTok = NULL;
			OOLUA::pull(state, valTok);

			resultStr = varargs_w(resultStr, valTok ? valTok->GetText() : L"Undefined token");
		}
	}

	return resultStr;
}