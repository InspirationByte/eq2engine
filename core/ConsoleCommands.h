//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////


#ifndef CONCOMMANDFACTORY_H
#define CONCOMMANDFACTORY_H

#include <stdio.h>
#include <stdlib.h>
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IConsoleCommands.h"

//	Console variable factory

// To prevent stack buffer overflow
#define MAX_SAME_COMMANDS 5
#define COMMANDBUFFER_SIZE (32*1024) // 32 kb buffer, equivalent of MAX_CFG_SIZE
#define COM_TOKEN_MAX_LENGTH 1024
#define	MAX_ARGS 80

class CConsoleCommands;

typedef void (CConsoleCommands::*FUNC)(char* str, int len, void* extra);

class CConsoleCommands : public IConsoleCommands
{
public:
	CConsoleCommands();

	void								RegisterCommands();

	// Unregister all
	void								DeInit();

	// Register new variable
	void								RegisterCommand(ConCommandBase *pCmd);

	// Unregister variable
	void								UnregisterCommand(ConCommandBase *pCmd);


	const ConVar*						FindCvar(const char* name);
	const ConCommand*					FindCommand(const char* name);
	const ConCommandBase*				FindBase(const char* name);

	// Returns all bases array
	const Array<ConCommandBase*>*		GetAllCommands() const { return &m_registeredCommands; }

	// Executes file
	void								ParseFileToCommandBuffer(const char* pszFilename);

	// Sets command buffer
	void								SetCommandBuffer(const char* pszBuffer);

	// Appends to command buffer, also can find only the needed command
	void								AppendToCommandBuffer(const char* pszBuffer);

	// Clears to command buffer
	void								ClearCommandBuffer();

	// Resets counter of same commands
	void								ResetCounter();

	// Executes command buffer
	bool								ExecuteCommandBuffer(cmdFilterFn_t filterFn = nullptr, bool quiet = false);

	// returns failed commands
	Array<EqString>&					GetFailedCommands();

	//-------------------------
	bool								IsInitialized() const		{return true;}
	const char*							GetInterfaceName() const	{return CONSOLE_INTERFACE_VERSION;}

private:
	void								ForEachSeparated(char* str, char separator, FUNC fn, void* extra);
	void								ParseAndAppend(char* str, int len, void* extra);
	void								SplitOnArgsAndExec(char* str, int len, void* extra);

	void								SortCommands();

	Array<ConCommandBase*>	m_registeredCommands{ PP_SL };

	Array<EqString>			m_failedCommands{ PP_SL };

	char					m_currentCommands[COMMANDBUFFER_SIZE];
	char					m_lastExecutedCommands[COMMANDBUFFER_SIZE];

	int						m_sameCommandsExecuted;
	bool					m_commandListDirty;
};

#endif //CONCOMMANDFACTORY_H
