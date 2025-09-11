#include <stdarg.h>
#include <lua.hpp>

#include "core/core_common.h"

#include "esl.h"
#include "esl_luaref.h"

namespace esl
{
void ScriptState::GCStop() const
{
	lua_gc(m_state, LUA_GCRESTART);
}

void ScriptState::GCRestart() const
{
	lua_gc(m_state, LUA_GCRESTART);
}

void ScriptState::GCStep(int stepSize) const
{
	lua_gc(m_state, LUA_GCSTEP, stepSize);
}

void ScriptState::GCCollect() const
{
	lua_gc(m_state, LUA_GCCOLLECT);
}

int ScriptState::LoadFileBuffer(IFileStream* virtStream, const char* name, const char* mode) const
{
	if (!virtStream)
	{
		lua_pushfstring(m_state, "Cannot load script %s", name);
		return LUA_ERRMEM;
	}

	CMemoryStream memStream(PPSourceLine::Make(name, 0));
	memStream.Open(FS_OPEN_WRITE | FS_OPEN_READ);

	CMemoryStream* useStream = &memStream;
	if (virtStream->GetType() == FS_TYPE_MEMORY)
		useStream = static_cast<CMemoryStream*>(virtStream);
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

	return luaL_loadbufferx(m_state, luaSrc, fileSize, useStream->GetName(), mode);
}

bool ScriptState::RunFileBuffer(IFileStream* virtStream, const char* name, const char* mode) const
{
	const int loadStatus = LoadFileBuffer(virtStream, name, mode);
	if (loadStatus != LUA_OK)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	const int result = lua_pcall(m_state, 0, LUA_MULTRET, 0);
	if (result != LUA_OK)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}
	return true;
}

bool ScriptState::RunChunk(EqStringRef chunk, const char* name) const
{
	esl::runtime::StackGuard g(m_state);

	const int res = luaL_loadbuffer(m_state, chunk.ToCString(), chunk.Length(), name);
	if (res != LUA_OK)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	const int result = lua_pcall(m_state, 0, LUA_MULTRET, 0);
	if (result != LUA_OK)
	{
		esl::runtime::SetLuaErrorFromTopOfStack(m_state);
		return false;
	}

	return true;
}

int ScriptState::GetStackTop() const
{
	return lua_gettop(m_state);
}

int ScriptState::GetStackType(int index) const
{
	return lua_type(m_state, index);
}

void ScriptState::ThrowError(const char* fmt, ...) const
{
	va_list argp;
	va_start(argp, fmt);
	luaL_where(m_state, 1);
	lua_pushvfstring(m_state, fmt, argp);
	va_end(argp);
	lua_concat(m_state, 2);
	lua_error(m_state);
}

esl::LuaTable ScriptState::CreateTable() const
{
	lua_newtable(m_state);
	const int tableIdx = lua_gettop(m_state);
	esl::LuaTable result = esl::LuaTable(m_state, tableIdx);
	lua_pop(m_state, 1);
	return result;
}

}