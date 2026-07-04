#pragma once

class ConCommandBase;
class ConCommand;
class ConVar;
class ICommandLine;
class IFileSystem;
class CFileSystemFind;
class ILocToken;

//
// Console commands
//
EQSCRIPT_BIND_TYPE_NO_PARENT(ConCommandBase, "ConCommandBase", esl::BY_REF)
EQSCRIPT_BIND_TYPE_WITH_PARENT(ConCommand, ConCommandBase, "ConCommand")
EQSCRIPT_BIND_TYPE_WITH_PARENT(ConVar, ConCommandBase, "ConVar")
EQSCRIPT_BIND_TYPE_NO_PARENT(ICommandLine, "ICommandLine", esl::BY_REF)

//
// filesystem & streams
//
EQSCRIPT_BIND_TYPE_NO_PARENT(IPackFileReader, "IPackFileReader", esl::REF_PTR)
EQSCRIPT_BIND_TYPE_NO_PARENT(IFileSystem, "IFileSystem", esl::BY_REF)
EQSCRIPT_BIND_TYPE_NO_PARENT(CFileSystemFind, "CFileSystemFind", esl::BY_VALUE)

//
// Localizer
//
EQSCRIPT_BIND_TYPE_NO_PARENT(ILocToken, "ILocToken", esl::BY_REF)

#ifdef ENABLE_MULTIPLAYER

OOLUA_PROXY( Networking::Buffer )

	OOLUA_MFUNC( WriteByte )
	OOLUA_MFUNC( WriteUByte )
	OOLUA_MFUNC( WriteInt16 )
	OOLUA_MFUNC( WriteUInt16 )
	OOLUA_MFUNC( WriteInt )
	OOLUA_MFUNC( WriteUInt )
	OOLUA_MFUNC( WriteBool )
	OOLUA_MFUNC( WriteFloat )

	OOLUA_MFUNC( WriteVector2D )
	OOLUA_MFUNC( WriteVector3D )
	OOLUA_MFUNC( WriteVector4D )

	OOLUA_MFUNC( ReadByte )
	OOLUA_MFUNC( ReadUByte )
	OOLUA_MFUNC( ReadInt16 )
	OOLUA_MFUNC( ReadUInt16 )
	OOLUA_MFUNC( ReadInt )
	OOLUA_MFUNC( ReadUInt )
	OOLUA_MFUNC( ReadBool )
	OOLUA_MFUNC( ReadFloat )

	OOLUA_MFUNC( ReadVector2D )
	OOLUA_MFUNC( ReadVector3D )
	OOLUA_MFUNC( ReadVector4D )

	OOLUA_MEM_FUNC( void, WriteString, const char* )
	OOLUA_MEM_FUNC( char*, ReadString, int& )

	OOLUA_MFUNC( WriteNetBuffer )

	OOLUA_MFUNC( WriteKeyValues )
	OOLUA_MFUNC( ReadKeyValues )

	OOLUA_MFUNC_CONST( GetMessageLength )
	OOLUA_MFUNC_CONST( GetClientID )

OOLUA_PROXY_END

OOLUA_PROXY(Networking::CNetworkThread)

	OOLUA_TAGS( Abstract )
	OOLUA_MFUNC(SendData)

OOLUA_PROXY_END

#endif // ENABLE_MULTIPLAYER

bool eslSysCoreInit(const esl::ScriptState& state);
bool eslSysDebugInit(const esl::ScriptState& state);
bool eslSysFileSystemInit(const esl::ScriptState& state);
bool eslSysConsoleInit(const esl::ScriptState& state);

void eslSysConsoleTerm();