//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable factory/registrator
//////////////////////////////////////////////////////////////////////////////////

#ifndef _ICONCOMMANDFACTORY_H_
#define _ICONCOMMANDFACTORY_H_

#include <stdio.h>
#include <stdlib.h>
#include "utils/DkList.h"
#include "ConVar.h"
#include "ConCommand.h"
#include "ConCommandBase.h"
#include "InterfaceManager.h"

#define CONSOLE_INTERFACE_VERSION		"CORE_ConsoleCommands_003"

class IConsoleCommands : public ICoreModuleInterface
{
public:
    friend class ConCommandBase;

    virtual void								RegisterCommand(ConCommandBase *pCmd) = 0;
    virtual void								UnregisterCommand(ConCommandBase *pCmd) = 0;

    virtual const	ConVar*						FindCvar(const char* name) = 0;
    virtual const	ConCommand *				FindCommand(const char* name) = 0;
    virtual const	ConCommandBase *			FindBase(const char* name) = 0;

    virtual	const	DkList<ConCommandBase*>*	GetAllCommands() = 0;

    // Executes file
    virtual void								ParseFileToCommandBuffer(const char* pszFilename, const char* lookupForCommand = NULL) = 0;

    // Sets command buffer
    virtual void								SetCommandBuffer(const char* pszBuffer) = 0;

    // Appends to command buffer
    virtual void								AppendToCommandBuffer(const char* pszBuffer) = 0;

    // Clears to command buffer
    virtual void								ClearCommandBuffer() = 0;

    // Resets counter of same commands
    virtual void								ResetCounter() = 0;

    // Executes command buffer
    virtual bool								ExecuteCommandBuffer(unsigned int CmdFilterFlags = -1) = 0;	
};

INTERFACE_SINGLETON( IConsoleCommands, CConsoleCommands, CONSOLE_INTERFACE_VERSION, g_sysConsole )

#endif //_ICONCOMMANDFACTORY_H_
