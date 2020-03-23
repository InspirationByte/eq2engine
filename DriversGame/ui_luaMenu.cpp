//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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

	m_stackReset.Get(m_menuStack, "Reset");
	m_stackPush.Get(m_menuStack, "Push");
	m_stackPop.Get(m_menuStack, "Pop");

	if( m_stackReset.Push() && m_stackReset.Call(0, 0) )
	{
		UpdateCurrentMenu();
	}
}

//
// Refreshes the menu. Should be called after Push or Pop
//
void CLuaMenu::UpdateCurrentMenu()
{
	OOLUA::Table currentStack;
	if (!m_menuStack.safe_at("stack", currentStack))
		return;

	if (!currentStack.safe_at("prevSelection", m_selection))
		m_selection = 0;

	lua_State* state = GetLuaState();

	// update items
	OOLUA::Table itemsTable;
	if (currentStack.safe_at("items", itemsTable))
	{
		EqLua::LuaStackGuard g(state);

		m_numElems = 0;
		oolua_ipairs(itemsTable)
			m_numElems++;
		oolua_ipairs_end()

		m_menuElems = itemsTable;
	}

	// update title string
	std::string tok_str;
	ILocToken* tok = nullptr;

	if (currentStack.safe_at("title", tok_str))
	{
		m_menuTitleStr = LocalizedString(tok_str.c_str());
	}
	else if (currentStack.safe_at("title", tok))
	{
		m_menuTitleStr = tok ? tok->GetText() : L"(nulltoken)";
	}
}

void CLuaMenu::PushMenu(OOLUA::Table& tabl, OOLUA::Table& paramsTable)
{
	lua_State* state = GetLuaState();

	if( m_stackPush.Push() && 
		OOLUA::push(state, tabl) && 
		OOLUA::push(state, m_selection) &&
		OOLUA::push(state, paramsTable) &&
		m_stackPush.Call(3, 0) )
	{
		UpdateCurrentMenu();
	}
}

void CLuaMenu::PopMenu()
{
	if(!IsCanPopMenu())
		return;

	lua_State* state = GetLuaState();

	if( m_stackPop.Push() && m_stackPop.Call(0, 0) )
	{
		UpdateCurrentMenu();
	}
}

bool CLuaMenu::IsCanPopMenu()
{
	OOLUA::Table currentStack;
	if (!m_menuStack.safe_at("stack", currentStack))
		return false;

	OOLUA::Table _dummy;
	return currentStack.safe_at("prevStack", _dummy);
}

bool CLuaMenu::PreEnterSelection()
{
	OOLUA::Table elem;
	if (GetCurrentMenuElement(elem))
	{
		OOLUA::Lua_func_ref enterFunc;
		if (!elem.safe_at("onEnter", enterFunc))
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
			{
				PushMenu(nextMenu, params);
			}

		}
	}
	else
		MsgError("CLuaMenu::EnterSelection error:\n %s\n", OOLUA::get_last_error(state).c_str());
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