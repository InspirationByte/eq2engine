#pragma once

extern "C" {
using lua_CFunction = int (*)(lua_State*);
struct lua_State;
}

namespace OOLUA {
	class Script;
}

using EqScriptCtx = OOLUA::Script&;

