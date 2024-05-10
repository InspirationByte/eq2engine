#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_event.h"

EQSCRIPT_BIND_TYPE_NO_PARENT(esl::LuaEvent, "LuaEvent", BY_REF)
EQSCRIPT_TYPE_BEGIN(esl::LuaEvent)
	EQSCRIPT_BIND_FUNC(AddHandler)
	EQSCRIPT_BIND_FUNC(RemoveHandler)
	EQSCRIPT_BIND_FUNC(Clear)
EQSCRIPT_TYPE_END

namespace esl
{

void LuaEvent::AddHandler(const esl::LuaFunctionRef& func)
{
	m_eventHandlers.insert(func);
}

void LuaEvent::RemoveHandler(const esl::LuaFunctionRef& func)
{
	m_eventHandlers.remove(func);
}

void LuaEvent::Clear()
{
	m_eventHandlers.clear();
}

}