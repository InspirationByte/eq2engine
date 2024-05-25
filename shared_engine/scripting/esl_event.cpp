#include <lua.hpp>
#include "core/core_common.h"

#include "esl.h"
#include "esl_event.h"

esl::LuaEvent::Handle::~Handle()
{
	if (owner)
		owner->RemoveHandlerInternal(id);
}

esl::LuaEvent::Handle::Handle(LuaEvent* event, uint id)
	: owner(CWeakPtr(event))
	, id(id)
{
}

esl::LuaEvent::Handle::Handle(Handle&& other) noexcept
	: owner(std::move(other.owner))
	, id(std::move(other.id)) 
{
}

esl::LuaEvent::Handle& esl::LuaEvent::Handle::operator=(Handle&& other) noexcept 
{
	owner = std::move(other.owner); 
	id = std::move(other.id);
	return *this; 
}

EQSCRIPT_BIND_TYPE_NO_PARENT(esl::LuaEvent::Handle, "LuaEventHandle", REF_PTR)
EQSCRIPT_TYPE_BEGIN(esl::LuaEvent::Handle)
EQSCRIPT_TYPE_END

EQSCRIPT_BIND_TYPE_NO_PARENT(esl::LuaEvent, "LuaEvent", BY_REF)
EQSCRIPT_TYPE_BEGIN(esl::LuaEvent)
	EQSCRIPT_BIND_FUNC(AddHandler)
	EQSCRIPT_BIND_FUNC(RemoveHandler)
	EQSCRIPT_BIND_FUNC(Clear)
EQSCRIPT_TYPE_END

namespace esl
{

esl::LuaEvent::HandlePtr LuaEvent::AddHandler(const esl::LuaFunctionRef& func)
{
	HandlePtr handle = CRefPtr_new(Handle, CWeakPtr(this), m_handleIdCnt++);
	m_eventHandlers.insert(handle->id, func);
	return handle;
} 

void LuaEvent::RemoveHandler(const HandlePtr& handle)
{
	if (!handle)
		return;
	RemoveHandlerInternal(handle->id);
}

void LuaEvent::RemoveHandlerInternal(uint handle)
{
	m_eventHandlers.remove(handle);
}

void LuaEvent::Clear()
{
	m_eventHandlers.clear();
}

}