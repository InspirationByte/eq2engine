#include <stdarg.h>
#include <lua.hpp>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "esl.h"
#include "esl_luaref.h"

bool EqScriptState::RunBuffer(IVirtualStream* virtStream, const char* name) const
{
	if (!virtStream)
	{
		esl::runtime::ResetErrorValue(m_state);
		lua_pushfstring(m_state, "Couldn't open '%s'", name);
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	CMemoryStream memStream(nullptr, VS_OPEN_WRITE | VS_OPEN_READ, 0, PPSourceLine::Make(name, 0));
	CMemoryStream* useStream = &memStream;
	if (virtStream->GetType() == VS_TYPE_MEMORY)
		useStream = reinterpret_cast<CMemoryStream*>(virtStream);
	else
		memStream.AppendStream(virtStream);

	long fileSize = useStream->GetSize();
	const char* luaSrc = (const char*)useStream->GetBasePointer();
	{
		ushort byteordermark = *((ushort*)luaSrc);
		if (byteordermark == 0xbbef || byteordermark == 0xfeff)
		{
			luaSrc += 3;
			fileSize -= 3;
		}
	}

	const int bufStatus = luaL_loadbuffer(m_state, luaSrc, fileSize, useStream->GetName());
	if (bufStatus != 0)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	const int result = lua_pcall(m_state, 0, LUA_MULTRET, 0);
	if (result != 0)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}
	return true;
}

bool EqScriptState::RunChunk(const EqString& chunk) const
{
	const int res = luaL_loadbuffer(m_state, chunk.ToCString(), chunk.Length(), "userChunk");
	if (res != 0)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	const int result = lua_pcall(m_state, 0, LUA_MULTRET, 0);
	if (result != 0)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	return true;
}

int EqScriptState::GetStackTop() const
{
	return lua_gettop(m_state);
}

int EqScriptState::GetStackType(int index) const
{
	return lua_type(m_state, index);
}

void EqScriptState::ThrowError(const char* fmt, ...) const
{
	va_list argp;
	va_start(argp, fmt);
	luaL_where(m_state, 1);
	lua_pushvfstring(m_state, fmt, argp);
	va_end(argp);
	lua_concat(m_state, 2);
	lua_error(m_state);
}

esl::LuaTable EqScriptState::CreateTable() const
{
	lua_newtable(m_state);
	const int tableIdx = lua_gettop(m_state);
	esl::LuaTable result = esl::LuaTable(m_state, tableIdx);
	lua_pop(m_state, 1);
	return result;
}