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

//bool ExecuteCommand(const char *text);

//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-
//	Console variable factory
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#define COM_TOKEN_MAX_LENGTH 1024
#define	MAX_ARGS 80

class ConCommandFactory : public IConCommandFactory
{
public:
	ConCommandFactory();

	void RegisterCommands();

	// Register new variable
	void								RegisterCommand(ConCommandBase *pCmd);
	// Unregister variable
	void								UnregisterCommand(ConCommandBase *pCmd);


	const ConVar*						FindCvar(const char* name);
	const ConCommand*					FindCommand(const char* name);
	const ConCommandBase*				FindBase(const char* name);

	// Returns all bases array
	const DkList<ConCommandBase*>*		GetAllCommands() { return &m_pCommandBases; }

	// Unregister all
	void								DeInit();
private:

	DkList<ConCommandBase*>	m_pCommandBases;

	DkList<char*>			m_pFailedCommands;

	bool					m_bCanTryExecute;
	bool					m_bTrying;
};

#endif //CONCOMMANDFACTORY_H
