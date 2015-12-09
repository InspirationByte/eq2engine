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

#define CMDFACTORY_INTERFACE_VERSION		"CORE_CommandFactory_002"

class IConCommandFactory
{
public:
    friend class ConCommandBase;

    virtual void								RegisterCommand(ConCommandBase *pCmd) = 0;
    virtual void								UnregisterCommand(ConCommandBase *pCmd) = 0;

    virtual const	ConVar*						FindCvar(const char* name) = 0;
    virtual const	ConCommand *				FindCommand(const char* name) = 0;
    virtual const	ConCommandBase *			FindBase(const char* name) = 0; // Mades execution and life easier

    virtual	const	DkList<ConCommandBase*>*	GetAllCommands() = 0;
};

IEXPORTS IConCommandFactory* GetCvars( void );

#endif //_ICONCOMMANDFACTORY_H_
