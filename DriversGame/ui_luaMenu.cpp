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

void CLuaMenu::SetMenuStack(OOLUA::Table& newStack)
{
	m_menuStack = newStack;

	lua_State* state = GetLuaState();

	m_stackPush.Get(m_menuStack, "Push");
	m_stackPop.Get(m_menuStack, "Pop");
	m_stackReset.Get(m_menuStack, "Reset");
	m_stackUpdate.Get(m_menuStack, "Update");
	m_stackInitItems.Get(m_menuStack, "InitItems");
	m_updateUIScheme.Get(m_menuStack, "UpdateUIScheme");

	OOLUA::Table initialMenu;
	m_menuStack.safe_at("initialMenu", initialMenu);

	OOLUA::Table paramsTable = OOLUA::new_table(state);
	paramsTable.set("nextMenu", initialMenu);

	if (m_stackReset.Push() && m_stackReset.Call(0,0))
	{
		PushMenu(paramsTable);
	}
	else
		MsgError("CLuaMenu::SetMenuStack Reset error:\n %s\n", OOLUA::get_last_error(state).c_str());
}

void CLuaMenu::UpdateMenu(float fDt)
{
	lua_State* state = GetLuaState();
	if (m_stackUpdate.Push())
	{
		OOLUA::push(state, fDt);
		if(!m_stackUpdate.Call(1, 0))
		{
			MsgError("CLuaMenu::UpdateCurrentMenu UpdateItems error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
}

//
// Refreshes the menu. Should be called after Push or Pop
//
void CLuaMenu::UpdateCurrentMenu()
{
	lua_State* state = GetLuaState();

	// initialize items
	if (m_stackInitItems.Push() && m_stackInitItems.Call(0, 0))
	{
		OOLUA::Table currentStack;
		if (!m_menuStack.safe_at("stack", currentStack))
			return;

		if (!currentStack.safe_at("prevSelection", m_selection))
			m_selection = 0;

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
	else
		MsgError("CLuaMenu::UpdateCurrentMenu UpdateItems error:\n %s\n", OOLUA::get_last_error(state).c_str());
}

void CLuaMenu::PushMenu(OOLUA::Table& paramsTable)
{
	lua_State* state = GetLuaState();

	if( m_stackPush.Push() && 
		OOLUA::push(state, m_selection) &&
		OOLUA::push(state, paramsTable) &&
		m_stackPush.Call(2, 0) )
	{
		UpdateCurrentMenu();
	}
	else
		MsgError("CLuaMenu::PushMenu error:\n %s\n", OOLUA::get_last_error(state).c_str());
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
	else
		MsgError("CLuaMenu::PopMenu error:\n %s\n", OOLUA::get_last_error(state).c_str());
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

		if (onChange.Get(elem, "onChange", true) &&
			onChange.Push() &&
			OOLUA::push(state, dir) &&
			onChange.Call(1, 1))
		{
			return true;
		}
		else
			MsgError("CLuaMenu::ChangeSelection error:\n %s\n", OOLUA::get_last_error(state).c_str());
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
				if (!stricmp(commandStr.c_str(), "Pop"))
				{
					PopMenu();
				}
			}
			else
				PushMenu(params);
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
