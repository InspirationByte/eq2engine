//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////

#include "ConCommandBase.h"
#include "IConCommandFactory.h"
#include "core_base_header.h"

// Default constructor
ConCommandBase::ConCommandBase()
{
	m_bIsRegistered	= false;
	m_nFlags = 0;

	m_szName	= NULL;
	m_szDesc	= NULL;
}

void ConCommandBase::Init(char const *name,char const *desc, int flags /*= 0*/,bool bIsConVar /* = false*/ )
{
	// Make Command name shared
	m_szName = name;

	if(desc)
		m_szDesc = desc;
	else
		m_szDesc = "No description";

	m_nFlags = flags;

	if(bIsConVar)
		m_nFlags |= CMDBASE_CONVAR;
	else
		m_nFlags |= CMDBASE_CONCOMMAND;

	if(GetFlags() & CV_UNREGISTERED)
		return;

	//Dynamic initalizer must create GetCvars() first before registering pBase
	if(GetCvars())
	{
		GetCvars()->RegisterCommand( this );

		m_bIsRegistered = true;
	}
}

ConCommandBase::~ConCommandBase()
{
	// Ex: When renderer device is restarting or library is unloading it
	// needs to be unregistered because without it causes NULL pointers
	Unregister(this);
}

void ConCommandBase::Unregister( ConCommandBase *pBase )
{
	GetCvars()->UnregisterCommand(pBase);
}
