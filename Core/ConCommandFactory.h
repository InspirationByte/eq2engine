//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////


#ifndef CONCOMMANDFACTORY_H
#define CONCOMMANDFACTORY_H

#include <stdio.h>
#include <stdlib.h>
#include "ConVar.h"
#include "ConCommand.h"
#include "InterfaceManager.h"
#include "IConCommandFactory.h"

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
	const DkList<ConCommandBase*>*		GetAllCommands() { return &m_pCommandBases; }

	// Executes file
	void								ParseFileToCommandBuffer(const char* pszFilename);

	// Sets command buffer
	void								SetCommandBuffer(const char* pszBuffer);

	// Appends to command buffer, also can find only the needed command
	void								AppendToCommandBuffer(const char* pszBuffer);

	// Clears to command buffer
	void								ClearCommandBuffer();

	// Resets counter of same commands
	void								ResetCounter() {m_nSameCommandsExecuted = 0;}

	// Executes command buffer
	bool								ExecuteCommandBuffer(unsigned int CmdFilterFlags = -1, bool quiet = false);

	// returns failed commands
	DkList<EqString>&					GetFailedCommands();

	void								EnableInitOnlyVarsChangeProtection(bool bEnable);

	//-------------------------
	bool								IsInitialized() const		{return true;}
	const char*							GetInterfaceName() const	{return CONSOLE_INTERFACE_VERSION;}

private:
	void								ForEachSeparated(char* str, char separator, FUNC fn, void* extra);
	void								ParseAndAppend(char* str, int len, void* extra);
	void								SplitOnArgsAndExec(char* str, int len, void* extra);

	DkList<ConCommandBase*>	m_pCommandBases;

	DkList<EqString>		m_failedCommands;

	char m_pszCommandBuffer[COMMANDBUFFER_SIZE];
	char m_pszLastCommandBuffer[COMMANDBUFFER_SIZE];

	int   m_nSameCommandsExecuted;
	bool m_bEnableInitOnlyChange;
};

#endif //CONCOMMANDFACTORY_H
