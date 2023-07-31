//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"

#include "ConCommand.h"
#include "ConVar.h"
#include "IConsoleCommands.h"

ConCommand::ConCommand(const char* name, CON_COMMAND_CALLBACK callback, int flags /*= 0*/)
	: ConCommandBase(name, flags | CMDBASE_CONCOMMAND)
{
	ASSERT_MSG(callback, "Command %s requires a callback", name);
	m_fnCallback = callback;

	if ((m_nFlags & CV_UNREGISTERED) == 0)
		Register(this);
}

void ConCommand::DispatchFunc(Array<EqString>& args)
{
	if(GetFlags() & CV_CHEAT)
	{
		const ConVar *cheats = (ConVar*)g_consoleCommands->FindCvar("__cheats"); //find cheats cvar
		if(cheats && !cheats->GetBool())
		{
			Msg("Cannot access to %s console command during cheats is off\n",GetName());
			return;
		}
	}

	if ( m_fnCallback )
		( *m_fnCallback )(this, args);
}