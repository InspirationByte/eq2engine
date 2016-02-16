//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////


#ifndef CONCOMMANDBASE_H
#define CONCOMMANDBASE_H

enum CommandBaseFlags_e
{
	CV_UNREGISTERED		= (1 << 0),	// Do not register this console command\cvar to list. So it can't be changed in console.
	CV_CHEAT			= (1 << 1), // Cheat. Must be checked from "__cheats" cvar
	CV_INITONLY			= (1 << 2), // Init-only console command such as "__cheats"
	CV_INVISIBLE		= (1 << 3), // Hidden console command. Does not shows in FastFind and others.
	CV_ARCHIVE			= (1 << 4), //Save on disk. Only ConVar have this flag
	CV_CLIENTCONTROLS	= (1 << 5), // Indicates that this command is client-controlled, and will disable on pause or when console is enabled


	CMDBASE_CONVAR		= (1 << 6), // Is ConVar
	CMDBASE_CONCOMMAND	= (1 << 7) // Is ConCommand
};

#include <stdio.h>
#include <stdlib.h>

class ConCommandBase
{
	friend class ConVar;
	friend class ConCommand;
	friend class CConsoleCommands;

public:
	ConCommandBase();
	virtual ~ConCommandBase();

	// Names, descs, flags
	const char*		GetName()	const	{return m_szName;}
	const char*		GetDesc()	const	{return m_szDesc;}
	int				GetFlags()	const	{return m_nFlags;}

	bool			IsConVar( void )		const	{return (m_nFlags & CMDBASE_CONVAR) > 0;}
	bool			IsConCommand( void )	const	{return (m_nFlags & CMDBASE_CONCOMMAND) > 0;}
	bool			IsRegistered( void )	const	{return m_bIsRegistered;}

	static void		Unregister( ConCommandBase *pBase );

protected:
	void		Init(char const *name,char const *desc, int flags = 0,bool bIsConVar = false);

	bool		m_bIsRegistered;

	int			m_nFlags;

	// Name and description
	const char*	m_szName;
	const char*	m_szDesc;
};

#endif //CONCOMMANDBASE_H