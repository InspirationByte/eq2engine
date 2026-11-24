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
EQSCRIPT_BIND_TYPE_NO_PARENT(IFileSystem, "IFileSystem", esl::BY_REF | esl::ABSTRACT)
EQSCRIPT_BIND_TYPE_NO_PARENT(CFileSystemFind, "CFileSystemFind", esl::BY_VALUE)

//
// Localizer
//
EQSCRIPT_BIND_TYPE_NO_PARENT(ILocToken, "ILocToken", esl::BY_REF | esl::ABSTRACT)

bool eslSysCoreInit(const esl::ScriptState& state);
bool eslSysDebugInit(const esl::ScriptState& state);
bool eslSysFileSystemInit(const esl::ScriptState& state);
bool eslSysConsoleInit(const esl::ScriptState& state);

void eslSysConsoleTerm();