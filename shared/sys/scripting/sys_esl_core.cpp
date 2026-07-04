//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//////////////////////////////////////////////////////////////////////////////////

// TODO

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ILocalize.h"
#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/ICommandLine.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#ifdef ENABLE_MULTIPLAYER
#include "Network/NETThread.h"
#endif

#include "sys_esl.h"
#include "sys_esl_core.h"

struct LuaConCommandFunc
{
	void Clean()
	{
		SAFE_DELETE_ARRAY(name);
		SAFE_DELETE_ARRAY(desc);
		SAFE_DELETE_ARRAY(defaultValue);
		SAFE_DELETE(instance);
	}

	esl::LuaFunctionRef cmdFuncRef;
	esl::LuaFunctionRef variantsFuncRef;
	esl::LuaFunctionRef changeCbFuncRef;
	ConCommandBase* instance{ nullptr };
	const char* name{ nullptr };
	const char* desc{ nullptr };
	const char* defaultValue{ nullptr };
};

static Map<int, LuaConCommandFunc> s_luaComCommands(PP_SL);

static LuaConCommandFunc* FindLuaCommandRef(const char* cmdName)
{
	const int nameHash = StringId24(cmdName);
	auto it = s_luaComCommands.find(nameHash);
	if (it.atEnd())
		return nullptr;

	return &(*it);
}

DECLARE_CONCOMMAND_FN(luaConCommandHandler)
{
	LuaConCommandFunc* ref = FindLuaCommandRef(cmd->GetName());
	ASSERT_MSG(ref, "luaConCommandHandler ref is invalid for %s", cmd->GetName());

	esl::ScriptState state(eslSys::GetScriptState());

	esl::LuaTable argTable = state.CreateTable();
	for (int i = 0; i < CMD_ARGC; i++)
		argTable.Set(i + 1, CMD_ARGV(i).ToCString());

	using ConCommandFunc = esl::runtime::FunctionCall<void, const esl::LuaTable&>;
	auto callResult = ConCommandFunc::Invoke(ref->cmdFuncRef, argTable);
	LUA_CHECK_CALL(callResult, cmd->GetName());
}

static void LuaCommandVariantsFunc(const ConCommandBase* base, Array<EqString>& variants, const char* query)
{
	LuaConCommandFunc* ref = FindLuaCommandRef(base->GetName());
	ASSERT_MSG(ref, "LuaCommandVariantsFunc ref is invalid for %s", base->GetName());

	using VariantsFunc = esl::runtime::FunctionCall<esl::LuaTable, const char*>;
	auto callResult = VariantsFunc::Invoke(ref->variantsFuncRef, query);
	if (!LUA_CHECK_CALL(callResult, base->GetName()))
		return;

	for (auto it = callResult.value.IPairs(); !it.AtEnd(); ++it)
	{
		const EqString elem = *callResult.value.Get<EqString>(*it);
		variants.append(elem);
	}
}

static void LuaConVarChangeCallbackFunc(ConVar* pVar, char const* pszOldValue)
{
	LuaConCommandFunc* ref = FindLuaCommandRef(pVar->GetName());
	ASSERT_MSG(ref, "LuaConVarChangeCallbackFunc ref is invalid for %s", pVar->GetName());

	using OnChangedFunc = esl::runtime::FunctionCall<void, const char*>;
	auto callResult = OnChangedFunc::Invoke(ref->changeCbFuncRef, pszOldValue);
	LUA_CHECK_CALL(callResult, pVar->GetName());
}

static void S_ConCommandBase_SetVariantsCallback(ConCommandBase* base, const esl::LuaFunctionRef& variantsFunc)
{
	// TODO: create command ref for native cvars
	LuaConCommandFunc* ref = FindLuaCommandRef(base->GetName());
	ASSERT_MSG(ref, "LuaConVarChangeCallbackFunc ref is invalid for %s", base->GetName());

	ref->variantsFuncRef = variantsFunc;

	if (variantsFunc.IsValid())
		base->SetVariantsCallback(LuaCommandVariantsFunc);
	else
		base->SetVariantsCallback(nullptr);
}

static void S_ConVar_SetChangeCallback(ConVar* base, const esl::LuaFunctionRef& variantsFunc)
{
	// TODO: create command ref for native cvars
	LuaConCommandFunc* ref = FindLuaCommandRef(base->GetName());
	ASSERT_MSG(ref, "LuaConVarChangeCallbackFunc ref is invalid for %s", base->GetName());

	ref->changeCbFuncRef = variantsFunc;

	if (variantsFunc.IsValid())
		base->SetCallback(LuaConVarChangeCallbackFunc);
	else
		base->SetCallback(nullptr);
}

EQSCRIPT_TYPE_BEGIN(ConCommandBase)
	EQSCRIPT_BIND_FUNC(GetName)
	EQSCRIPT_BIND_FUNC(GetDesc)
	EQSCRIPT_BIND_FUNC(GetFlags)
	EQSCRIPT_BIND_FUNC(IsConVar)
	EQSCRIPT_BIND_FUNC(IsConCommand)
	EQSCRIPT_BIND_FUNC(IsRegistered)
	EQSCRIPT_BIND_STATIC_FUNC("SetVariantsCallback", S_ConCommandBase_SetVariantsCallback)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(ConCommand)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(ConVar)
	EQSCRIPT_BIND_FUNC(RevertToDefaultValue)
	EQSCRIPT_BIND_FUNC_NAMED("SetString", SetValue)
	EQSCRIPT_BIND_FUNC(SetFloat)
	EQSCRIPT_BIND_FUNC(SetInt)
	EQSCRIPT_BIND_FUNC(SetBool)
	EQSCRIPT_BIND_FUNC(SetClamp)
	EQSCRIPT_BIND_FUNC(HasClamp)
	EQSCRIPT_BIND_FUNC(GetMinClamp)
	EQSCRIPT_BIND_FUNC(GetMaxClamp)
	EQSCRIPT_BIND_FUNC(GetFloat)
	EQSCRIPT_BIND_FUNC(GetString)
	EQSCRIPT_BIND_FUNC(GetInt)
	EQSCRIPT_BIND_FUNC(GetBool)

	EQSCRIPT_BIND_STATIC_FUNC("SetChangeCallback", S_ConVar_SetChangeCallback)
EQSCRIPT_TYPE_END

static const char* S_ICommandLine_GetArgumentString(ICommandLine* self, int idx)
{
	return self->GetParameters()[idx];
}

static int S_ICommandLine_GetArgumentCount(ICommandLine* self)
{
	return self->GetParameters().numElem();
}

EQSCRIPT_TYPE_BEGIN(ICommandLine)
	EQSCRIPT_BIND_FUNC(Find)
	EQSCRIPT_BIND_STATIC_FUNC("GetArgumentString", S_ICommandLine_GetArgumentString)
	EQSCRIPT_BIND_STATIC_FUNC("GetArgumentCount", S_ICommandLine_GetArgumentCount)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(ILocToken)
EQSCRIPT_TYPE_END


//---------------------------------------------------------------------------------------
// Debug output
//---------------------------------------------------------------------------------------

static EqString GetMsgStrArgs(const esl::ScriptState& state)
{
	const int n = lua_gettop(state);

	EqString out;
	for (int i = 1; i <= n; i++)
	{
		size_t l;
		const char* s = luaL_tolstring(state, i, &l);
		if (i > 1) out.Append(' ');
		out.Append(s);
		lua_pop(state, 1);
	}
	return out;
}

static void WMsg(const esl::ScriptState& state)
{
	Msg("[Lua] %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgInfo(const esl::ScriptState& state)
{
	MsgInfo("[Lua] %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgWarning(const esl::ScriptState& state)
{
	MsgWarning("[Lua] %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgError(const esl::ScriptState& state)
{
	MsgError("[Lua] %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgAccept(const esl::ScriptState& state)
{
	MsgAccept("[Lua] %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgBox(const esl::ScriptState& state)
{
	InfoMsg("Lua - %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgBoxWarning(const esl::ScriptState& state)
{
	WarningMsg("Lua - %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgBoxError(const esl::ScriptState& state)
{
	ErrorMsg("Lua - %s", GetMsgStrArgs(state).ToCString());
}

static void WMsgBoxAbort(const esl::ScriptState& state)
{
	CrashMsg("Lua - %s", GetMsgStrArgs(state).ToCString());
}

//---------------------------------------------------------------------------------------
// DkCore
//---------------------------------------------------------------------------------------

static const KVSection* S_DkCore_GetConfig(const char* section_name)
{
	const KVSection& kvs = g_eqCore->GetConfig();
	return kvs.FindSection(section_name);
}

//---------------------------------------------------------------------------------------
// Filesystem
//---------------------------------------------------------------------------------------

static EqString S_IPackFileReader_GetFileData(IPackFileReader& pak, const char* fileName)
{
	IFileStreamPtr file = pak.Open(fileName, FS_OPEN_READ);
	if (!file)
		return EqString::EmptyStr;

	const VSSize fileSize = file->GetSize();

	EqString textData;
	EqString::ReadString(file, fileSize, textData);
	return textData;
}

//---------------------------------------------------------------------------------------
// Localizer
//---------------------------------------------------------------------------------------

static const ILocToken* S_Localize_GetToken(const char* pszToken)
{
	return g_localizer->GetToken(pszToken);
}

static void S_Localize_AddTokensFile(const char* filenamePrefix)
{
	return g_localizer->AddTokensFile(filenamePrefix);
}

static void S_Localize_RemoveTokensFile(const char* filenamePrefix)
{
	return g_localizer->RemoveTokensFile(filenamePrefix);
}

static const ILocToken* S_Localize_AddToken(const char* token, const char* text)
{
	return g_localizer->AddToken(token, text);
}

//---------------------------------------------------------------------------------------
// Console
//---------------------------------------------------------------------------------------

static ConVar* Lua_Console_FindCvar(const char* name)
{
	return (ConVar*)g_consoleCommands->FindCvar(name);
}

static ConCommand* Lua_Console_FindCommand(const char* name)
{
	return (ConCommand*)g_consoleCommands->FindCommand(name);
}

static void Lua_Console_ExecuteString(const char* cmd)
{
	g_consoleCommands->SetCommandBuffer(cmd);
	g_consoleCommands->ExecuteCommandBuffer();
}

static void Lua_Console_ExecuteCommandLine()
{
	g_cmdLine->ExecuteCommandLine();
}

static ConCommand* Lua_Console_CreateCommand(char const* name, esl::LuaFunctionRef cmdFunc, char const* desc, int flags)
{
	// register con. command function reference
	ASSERT_MSG(cmdFunc.IsValid() == true, "Not valid function for Lua ConCommand %s", name);

	LuaConCommandFunc ref;
	ref.name = CString::DuplicateNew(name);
	ref.desc = desc ? CString::DuplicateNew(desc) : nullptr;
	ref.cmdFuncRef = cmdFunc;
	ref.instance = PPNew ConCommand(ref.name, CONCOMMAND_FN(luaConCommandHandler), flags, ConCommand::Desc(ref.desc));
	s_luaComCommands.insert(StringId24(name), ref);

	return static_cast<ConCommand*>(ref.instance);
}

static ConVar* Lua_Console_CreateCvar(char const* name, char const* value, char const* desc, int flags)
{
	LuaConCommandFunc ref;
	ref.name = CString::DuplicateNew(name);
	ref.desc = desc ? CString::DuplicateNew(desc) : nullptr;
	ref.defaultValue = CString::DuplicateNew(value);
	ref.instance = PPNew ConVar(ref.name, ref.defaultValue, flags, ConCommand::Desc(ref.desc));
	s_luaComCommands.insert(StringId24(name), ref);

	return static_cast<ConVar*>(ref.instance);
}

static void Lua_Console_RemoveCommandBase(ConCommandBase* cmdbase)
{
	const int nameHash = StringId24(cmdbase->GetName());
	auto it = s_luaComCommands.find(nameHash);
	if (!it.atEnd())
	{
		(*it).Clean();
		s_luaComCommands.remove(it);
	}
}

static void Lua_Console_RegisterCommandBase(ConCommandBase* cmdbase)
{
	g_consoleCommands->RegisterCommand(cmdbase); 
}
static void Lua_Console_UnregisterCommandBase(ConCommandBase* cmdbase)
{
	g_consoleCommands->UnregisterCommand(cmdbase); 
}

#ifdef ENABLE_MULTIPLAYER
// net event class for Lua
class CLuaNetEvent : public Networking::CNetEvent
{
public:
	CLuaNetEvent(lua_State* vm, OOLUA::Table& table);

	void Process(Networking::CNetworkThread* pNetThread);
	void Unpack(Networking::CNetworkThread* pNetThread, Networking::Buffer* pBuf);
	void Pack(Networking::CNetworkThread* pNetThread, Networking::Buffer* pBuf);
	bool OnDeliveryFailed();

	int	GetEventType() const { return NETTHREAD_EVENTS_LUA_START; }

protected:
	lua_State* m_state;
	OOLUA::Table	m_table;

	OOLUA::Lua_func_ref m_pack;
	OOLUA::Lua_func_ref m_unpack;
	OOLUA::Lua_func_ref m_process;
	OOLUA::Lua_func_ref m_deliveryfail;
};

CLuaNetEvent::CLuaNetEvent(lua_State* vm, OOLUA::Table& table)
{
	m_state = vm;
	m_table = table;

	eslSys::LuaStackGuard g(m_state);

	m_table.push_on_stack(m_state);

	lua_getfield(m_state, -1, "Pack");
	m_pack.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));

	lua_getfield(m_state, -1, "Unpack");
	m_unpack.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));

	lua_getfield(m_state, -1, "Process");
	m_process.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));

	lua_getfield(m_state, -1, "OnDeliveryFailed");
	m_deliveryfail.set_ref(m_state, luaL_ref(m_state, LUA_REGISTRYINDEX));
}

void CLuaNetEvent::Process(Networking::CNetworkThread* pNetThread)
{
	int n = lua_gettop(m_state);
	lua_pop(m_state, n);

	eslSys::LuaStackGuard s(m_state);

	// call table function, passing parameters
	if (!m_process.push(m_state))
	{
		MsgError("CLuaNetEvent::Unpack push failed\n");
		return;
	}

	m_table.set("eventId", m_nEventID);

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	// next are to place arguemnts
	OOLUA::push(m_state, pNetThread);

	int res = lua_pcall(m_state, 2, 0, 0);

	// if error
	if (res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);

#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(EqString::Format("CLuaNetEvent::Process error:\n %s\n", esl::runtime::GetLastError(m_state)).ToCString());
#else
		MsgError("CLuaNetEvent::Process error:\n %s\n", esl::runtime::GetLastError(m_state));
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
}

void CLuaNetEvent::Unpack(Networking::CNetworkThread* pNetThread, Networking::Buffer* pStream)
{
	int n = lua_gettop(m_state);
	lua_pop(m_state, n);

	eslSys::LuaStackGuard s(m_state);

	// call table function, passing parameters
	if (!m_unpack.push(m_state))
	{
		MsgError("CLuaNetEvent::Unpack push failed\n");
		return;
	}

	m_table.set("eventId", m_nEventID);

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	// next are to place arguemnts
	OOLUA::push(m_state, pNetThread);
	OOLUA::push(m_state, pStream);

	int res = lua_pcall(m_state, 3, 0, 0);

	// if error
	if (res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);
#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(EqString::Format("CLuaNetEvent::Unpack error:\n %s\n", esl::runtime::GetLastError(m_state)).ToCString());
#else
		MsgError("CLuaNetEvent::Unpack error:\n %s\n", esl::runtime::GetLastError(m_state));
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
}

void CLuaNetEvent::Pack(Networking::CNetworkThread* pNetThread, Networking::Buffer* pStream)
{
	int n = lua_gettop(m_state);
	lua_pop(m_state, n);

	eslSys::LuaStackGuard s(m_state);

	// call table function, passing parameters
	if (!m_pack.push(m_state))
	{
		MsgError("CLuaNetEvent::Unpack push failed\n");
		return;
	}

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	// next are to place arguemnts
	OOLUA::push(m_state, pNetThread);
	OOLUA::push(m_state, pStream);

	int res = lua_pcall(m_state, 3, 0, 0);

	// if error
	if (res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);

#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(EqString::Format("CLuaNetEvent::Pack error:\n %s\n", esl::runtime::GetLastError(m_script->state())).ToCString());
#else
		MsgError("CLuaNetEvent::Pack error:\n %s\n", esl::runtime::GetLastError(m_state));
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
}

// standard message handlers

// delivery completely failed after retries: if returns false, this removes message
bool CLuaNetEvent::OnDeliveryFailed()
{
	eslSys::LuaStackGuard s(m_state);

	// call table function and return it's result
	if (!m_deliveryfail.push(m_state))
		return false;

	// first we place a parent table of 'Pack' function it as this\self
	m_table.push_on_stack(m_state);

	int res = lua_pcall(m_state, 1, 0, 0);

	bool result = false;

	// if error
	if (res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(m_state);

#ifdef NETTHREAD_MESSAGEBOX_ERRORS
		ErrorMsg(EqString::Format("CLuaNetEvent::OnDeliveryFailed error:\n %s\n", esl::runtime::GetLastError(m_state)).ToCString());
#else
		MsgError("CLuaNetEvent::OnDeliveryFailed error:\n %s\n", esl::runtime::GetLastError(m_state));
#endif // NETTHREAD_MESSAGEBOX_ERRORS
	}
	else
	{
		OOLUA::pull(m_state, result);
	}

	return result;
}

Networking::CNetEvent* LUANetEventCallbackFactory(Networking::CNetworkThread* thread, Networking::Buffer* msg)
{
	OOLUA::Script& state = GetScriptState();

	eslSys::LuaStackGuard s(state);

	int nLuaEventID = msg->ReadInt();

	if (!state.call("CreateNetEvent", nLuaEventID))
	{
		Msg("LUANetEventCallbackFactory error:\n %s\n", esl::runtime::GetLastError(state));
		//Msg("	-- this error could be if no CreateNetEvent function found, check 'netevents.lua'\n");
		return nullptr;
	}

	if (lua_isnil(state, -1))
	{
		MsgError("LuaNetEvent ID '%d' is not registered, maybe you forgot to AddNetEvent?\n", nLuaEventID);
	}
	else
	{
		if (!lua_istable(state, -1))
		{
			MsgError("LuaNetEvent ID '%d' has invalid factory type\n", nLuaEventID);
			Msg("	-- follow the documentation in 'netevents.lua'\n");
		}
		else
		{
			OOLUA::Table table;

			table.lua_get(state, -1);

			return (Networking::CNetEvent*)new CLuaNetEvent(state, table);
		}
	}

	return nullptr;
}


// send event called in LUA
int CNetworkThread_SendLuaEvent(Networking::CNetworkThread* thread, OOLUA::Table& luaevent, int nEventType, int client_id)
{
	CLuaNetEvent* pEvent = PPNew CLuaNetEvent(luaevent.state(), luaevent);

	return thread->SendEvent(pEvent, NETTHREAD_EVENTS_LUA_START + nEventType, client_id);
}

// send event with waiter called in LUA
bool CNetworkThread_SendLuaWaitDataEvent(Networking::CNetworkThread* thread, OOLUA::Table& luaevent, int nEventType, Networking::Buffer* pOutputData, int client_id)
{
	CLuaNetEvent* pEvent = PPNew CLuaNetEvent(luaevent.state(), luaevent);

	return thread->SendWaitDataEvent(pEvent, NETTHREAD_EVENTS_LUA_START + nEventType, pOutputData, client_id);
}

OOLUA_CFUNC(CNetworkThread_SendLuaEvent, L_CNetworkThread_SendLuaEvent)
OOLUA_CFUNC(CNetworkThread_SendLuaWaitDataEvent, L_CNetworkThread_SendLuaWaitDataEvent)

OOLUA_EXPORT_FUNCTIONS(Networking::Buffer,
	WriteByte, WriteUByte, WriteInt16, WriteUInt16, WriteInt, WriteUInt, WriteBool, WriteFloat, WriteVector2D,
	WriteVector3D, WriteVector4D,
	ReadByte, ReadUByte, ReadInt16, ReadUInt16, ReadInt, ReadUInt, ReadBool, ReadFloat, ReadVector2D,
	ReadVector3D, ReadVector4D, WriteString, WriteNetBuffer, WriteKeyValues, ReadKeyValues)

OOLUA_EXPORT_FUNCTIONS_CONST(Networking::Buffer, GetMessageLength, GetClientID)

// since direct ReadString is unsafe, we're overriding it with wrapper
EqString S_CNetMessageBuffer_ReadString(Networking::Buffer* buf)
{
	EqString str = buf->ReadString();
	return str.ToCString();
}
OOLUA_CFUNC(S_CNetMessageBuffer_ReadString, L_CNetMessageBuffer_ReadString)

OOLUA_EXPORT_FUNCTIONS(Networking::CNetworkThread, SendData)
OOLUA_EXPORT_FUNCTIONS_CONST(Networking::CNetworkThread)

void eslSysNetworkingInit(const esl::ScriptState& state)
{
	// networking classes
	OOLUA::register_class<Networking::Buffer>(state);
	OOLUA::register_class_static<Networking::Buffer>(state, "ReadString", L_CNetMessageBuffer_ReadString);

	OOLUA::register_class<Networking::CNetworkThread>(state);
	OOLUA::register_class_static<Networking::CNetworkThread>(state, "SendEvent", L_CNetworkThread_SendLuaEvent);
	OOLUA::register_class_static<Networking::CNetworkThread>(state, "SendWaitDataEvent", L_CNetworkThread_SendLuaWaitDataEvent);
}

#endif // ENABLE_MULTIPLAYER

bool eslSysConsoleInit(const esl::ScriptState& state)
{
	LUA_SET_GLOBAL_CONST(state, CV_UNREGISTERED);
	LUA_SET_GLOBAL_CONST(state, CV_CHEAT);
	LUA_SET_GLOBAL_CONST(state, CV_PROTECTED);
	LUA_SET_GLOBAL_CONST(state, CV_INVISIBLE);
	LUA_SET_GLOBAL_CONST(state, CV_ARCHIVE);
	LUA_SET_GLOBAL_CONST(state, CV_CLIENTCONTROLS);

	state.RegisterClass<ConCommandBase>();
	state.RegisterClass<ConVar>();
	state.RegisterClass<ConCommand>();

	state.RegisterClass<ICommandLine>();

	esl::LuaTable consoleTab = state.CreateTable();
	consoleTab.Set("FindCvar", EQSCRIPT_CFUNC(Lua_Console_FindCvar));
	consoleTab.Set("FindCommand", EQSCRIPT_CFUNC(Lua_Console_FindCommand));
	consoleTab.Set("ExecuteString", EQSCRIPT_CFUNC(Lua_Console_ExecuteString));
	consoleTab.Set("ExecuteCommandLine", EQSCRIPT_CFUNC(Lua_Console_ExecuteCommandLine));

	consoleTab.Set("CreateCommand", EQSCRIPT_CFUNC(Lua_Console_CreateCommand));
	consoleTab.Set("CreateCvar", EQSCRIPT_CFUNC(Lua_Console_CreateCvar));
	consoleTab.Set("RemoveCommandBase", EQSCRIPT_CFUNC(Lua_Console_RemoveCommandBase));
	consoleTab.Set("RegisterCommandBase", EQSCRIPT_CFUNC(Lua_Console_RegisterCommandBase));
	consoleTab.Set("UnregisterCommandBase", EQSCRIPT_CFUNC(Lua_Console_UnregisterCommandBase));

	ICommandLine* cmdLineInstance = g_cmdLine;
	state.SetGlobal("console", consoleTab);
	state.SetGlobal("cmdline", cmdLineInstance);
	return true;
}

void eslSysConsoleTerm()
{
	for (auto it = s_luaComCommands.begin(); !it.atEnd(); ++it)
		(*it).Clean();
	s_luaComCommands.clear();
}

ESL_ENUM(ESearchPath);

EQSCRIPT_TYPE_BEGIN(IPackFileReader)
	EQSCRIPT_BIND_FUNC(GetName)
	EQSCRIPT_BIND_FUNC(FileExist)
	EQSCRIPT_BIND_FUNC(FindFileIndex)
	EQSCRIPT_BIND_STATIC_FUNC("GetFileData", S_IPackFileReader_GetFileData)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(IFileSystem)
	EQSCRIPT_BIND_FUNC(RemovePackage)
	EQSCRIPT_BIND_FUNC(AddSearchPath)
	EQSCRIPT_BIND_FUNC(OpenPackage)
	EQSCRIPT_BIND_FUNC(RemoveSearchPath)
	EQSCRIPT_BIND_FUNC(GetCurrentGameDirectory)
	EQSCRIPT_BIND_FUNC(GetCurrentDataDirectory)
	EQSCRIPT_BIND_FUNC(FileExist)
	EQSCRIPT_BIND_FUNC(FileRemove)
	EQSCRIPT_BIND_FUNC(FileCopy)
	EQSCRIPT_BIND_FUNC(GetFileSize)
	EQSCRIPT_BIND_FUNC(GetFileCRC32)
	EQSCRIPT_BIND_FUNC(MakeDir)
	EQSCRIPT_BIND_FUNC(RemoveDir)
	EQSCRIPT_BIND_FUNC(AddPackage)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(CFileSystemFind)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*, int)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*, int, int)
	EQSCRIPT_BIND_FUNC(Init)
	EQSCRIPT_BIND_FUNC(GetDirIndex)
	EQSCRIPT_BIND_FUNC(IsDirectory)
	EQSCRIPT_BIND_FUNC(GetPath)
	EQSCRIPT_BIND_FUNC(Next)
EQSCRIPT_TYPE_END

static EqStringRef L_fnmPathCombine()
{
	lua_State* L = eslSys::GetScriptState();
	const int n = lua_gettop(L);  /* number of arguments */

	size_t len;
	int maxLength = 0;
	FixedArray<EqStringRef, 32> paths;
	for (int i = 1; i <= n; i++)
	{
		const char* str = luaL_tolstring(L, i, &len);
		if (!len)
			continue;
		paths.append(EqStringRef(str, static_cast<int>(len)));
	}

	lua_pop(L, n);

	static EqString outPath;
	outPath.Empty();
	outPath.Resize(maxLength);
	for (int i = 0; i < paths.numElem(); ++i)
	{
		outPath.Append(paths[i]);
		if (i < paths.numElem() - 1 && outPath[outPath.Length() - 1] != '/')
			outPath.Append('/');
	}

	return outPath;
}

bool eslSysFileSystemInit(const esl::ScriptState& state)
{
	LUA_SET_GLOBAL_CONST(state, SP_DATA);
	LUA_SET_GLOBAL_CONST(state, SP_MOD);
	LUA_SET_GLOBAL_CONST(state, SP_ROOT);
	LUA_SET_GLOBAL_CONST(state, CORRECT_PATH_SEPARATOR);

	state.SetGlobal("fnmPathHasExt", EQSCRIPT_CFUNC(fnmPathHasExt));
	state.SetGlobal("fnmPathApplyExt", EQSCRIPT_CFUNC(fnmPathApplyExt));
	state.SetGlobal("fnmPathStripExt", EQSCRIPT_CFUNC(fnmPathStripExt));
	state.SetGlobal("fnmPathStripName", EQSCRIPT_CFUNC(fnmPathStripName));
	state.SetGlobal("fnmPathStripPath", EQSCRIPT_CFUNC(fnmPathStripPath));
	state.SetGlobal("fnmPathExtractExt", EQSCRIPT_CFUNC(fnmPathExtractExt));
	state.SetGlobal("fnmPathExtractName", EQSCRIPT_CFUNC(fnmPathExtractName));
	state.SetGlobal("fnmPathExtractPath", EQSCRIPT_CFUNC(fnmPathExtractPath));
	state.SetGlobal("fnmPathCombine", EQSCRIPT_CFUNC(L_fnmPathCombine));

	state.RegisterClass<IFileSystem>();
	state.RegisterClass<IPackFileReader>();
	state.RegisterClass<CFileSystemFind>();

	IFileSystem* fsInstance = g_fileSystem;
	state.SetGlobal("fileSystem", fsInstance);

	return true;
}

bool eslSysDebugInit(const esl::ScriptState& state)
{
	// replace default Lua print with standard Msg
	state.SetGlobal("print", EQSCRIPT_CFUNC(WMsg));

	state.SetGlobal("Msg", EQSCRIPT_CFUNC(WMsg));
	state.SetGlobal("MsgInfo", EQSCRIPT_CFUNC(WMsgInfo));
	state.SetGlobal("MsgWarning", EQSCRIPT_CFUNC(WMsgWarning));
	state.SetGlobal("MsgError", EQSCRIPT_CFUNC(WMsgError));
	state.SetGlobal("MsgAccept", EQSCRIPT_CFUNC(WMsgAccept));

	state.SetGlobal("MsgBox", EQSCRIPT_CFUNC(WMsgBox));
	state.SetGlobal("MsgBoxWarning", EQSCRIPT_CFUNC(WMsgBoxWarning));
	state.SetGlobal("MsgBoxError", EQSCRIPT_CFUNC(WMsgBoxError));
	state.SetGlobal("MsgBoxAbort", EQSCRIPT_CFUNC(WMsgBoxAbort));

	return true;
}

bool eslSysCoreInit(const esl::ScriptState& state)
{
	// eqCore
	{
		esl::LuaTable eqCoreTable = state.CreateTable();
		eqCoreTable.Set("GetConfig", EQSCRIPT_CFUNC(S_DkCore_GetConfig));
		state.SetGlobal("core", eqCoreTable);
	}

	// Localization system
	{
		state.RegisterClass<ILocToken>();
		esl::LuaTable localizeTable = state.CreateTable();
		localizeTable.Set("GetToken", EQSCRIPT_CFUNC(S_Localize_GetToken));
		localizeTable.Set("AddTokensFile", EQSCRIPT_CFUNC(S_Localize_AddTokensFile));
		localizeTable.Set("RemoveTokensFile", EQSCRIPT_CFUNC(S_Localize_RemoveTokensFile));
		localizeTable.Set("AddToken", EQSCRIPT_CFUNC(S_Localize_AddToken));
		state.SetGlobal("localize", localizeTable);
	}

	state.SetGlobal("StringId24", EQSCRIPT_CFUNC(StringId24));
	state.SetGlobal("StringId", EQSCRIPT_CFUNC(StringId));

	return true;
}
