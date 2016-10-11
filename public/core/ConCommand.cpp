//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////

#include "ConCommand.h"
#include "ConVar.h"
#include "IConCommandFactory.h"
#include "DebugInterface.h"

ConCommand::ConCommand(char const *name,CON_COMMAND_CALLBACK callback,char const *desc, int flags /*= 0*/) : ConCommandBase()
{
	Create(name,callback,NULL,desc,flags);
}

ConCommand::ConCommand(char const *name,CON_COMMAND_CALLBACK callback, CMDBASE_VARIANTS_CALLBACK variantsList,char const *desc, int flags)
{
	Create(name,callback,variantsList,desc,flags);
}

void ConCommand::Create(char const *pszName,CON_COMMAND_CALLBACK callback, CMDBASE_VARIANTS_CALLBACK variantsList, char const *pszHelpString, int nFlags)
{
	m_fnCallback = callback;
	m_fnVariantsList = variantsList;
	Init(pszName,pszHelpString,nFlags,false);
}

void ConCommand::DispatchFunc(DkList<EqString>& args)
{
	if((GetFlags() & CV_CHEAT))
	{
		ConVar *cheats = (ConVar*)g_sysConsole->FindCvar("__cheats"); //find cheats cvar
		if(cheats)
		{
			if(!cheats->GetBool())
			{
				Msg("Cannot access to %s console command during cheats is off\n",GetName());
				return;
			}
		}
	}

	if ( m_fnCallback )
		( *m_fnCallback )(args);
}