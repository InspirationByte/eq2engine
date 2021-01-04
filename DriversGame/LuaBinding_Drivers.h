//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate OOLua binding
//////////////////////////////////////////////////////////////////////////////////

#ifndef LUABINDING_DRIVERS
#define LUABINDING_DRIVERS

#include "luabinding/LuaBinding.h"
#include "luabinding/LuaBinding_Engine.h"
#include "EqUI_DrvSynTimer.h"


OOLUA_PROXY(equi::DrvSynTimerElement, equi::IUIControl)
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC(SetType)
	OOLUA_MFUNC(SetTimeValue)

OOLUA_PROXY_END

bool LuaBinding_InitDriverSyndicateBindings(lua_State* state);

OOLUA::Script& GetLuaState();

#endif // LUABINDING_DRIVERS