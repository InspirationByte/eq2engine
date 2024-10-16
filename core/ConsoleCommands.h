//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IConsoleCommands.h"

//	Console variable factory

// To prevent stack buffer overflow
#define MAX_SAME_COMMANDS 5
#define COMMANDBUFFER_SIZE (32*1024) // 32 kb buffer, equivalent of MAX_CFG_SIZE
#define COM_TOKEN_MAX_LENGTH 1024
#define	MAX_ARGS 80

class CConsoleCommands;
typedef void (CConsoleCommands::* FUNC)(const char* str, int len, void* extra);

class CConsoleCommands : public IConsoleCommands
{
public:
	CConsoleCommands();

	void					RegisterCommands();

	// Unregister all
	void					DeInit();

	// Register new variable
	void					RegisterCommand(ConCommandBase *pCmd);

	// Unregister variable
	void					UnregisterCommand(ConCommandBase *pCmd);

	const ConVar*			FindCvar(const char* name);
	const ConCommand*		FindCommand(const char* name);
	const ConCommandBase*	FindBase(const char* name);

	// Returns all bases array
	const ConCommandListRef	GetAllCommands() const { return m_registeredCommands; }

	void					PushConVarInitialValue(const char* name, const char* value);

	// Executes file
	void					ParseFileToCommandBuffer(const char* pszFilename);

	// Sets command buffer
	void					SetCommandBuffer(const char* pszBuffer);

	// Appends to command buffer, also can find only the needed command
	void					AppendToCommandBuffer(const char* pszBuffer);

	// Clears to command buffer
	void					ClearCommandBuffer();

	// Resets counter of same commands
	void					ResetCounter();

	// Executes command buffer
	bool					ExecuteCommandBuffer(CommandFilterFn filterFn = nullptr, bool quiet = false, Array<EqString>* failedCmds = nullptr);

	//-------------------------
	bool					IsInitialized() const { return true; }

private:
	void					ForEachSeparated(const char* str, FUNC fn, void* extra);
	void					ParseAndAppend(const char* str, int len, void* extra);
	void					SplitOnArgsAndExec(const char* str, int len, void* extra);

	void					SortCommands();

	// a special store used for loading configs early
	Map<int, EqString>		m_cvarValueStore{ PP_SL };

	Array<ConCommandBase*>	m_registeredCommands{ PP_SL };

	char					m_currentCommands[COMMANDBUFFER_SIZE];
	char					m_lastExecutedCommands[COMMANDBUFFER_SIZE];

	int						m_sameCommandsExecuted{ false };
	bool					m_commandListDirty{ false };
};
