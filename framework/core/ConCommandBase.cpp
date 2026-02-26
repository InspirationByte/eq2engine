//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"
#include "ConCommandBase.h"
#include "core/IConsoleCommands.h"

ConCommandBase::ConCommandBase(char const *name, int flags)
	: m_szName(name)
	, m_nFlags(flags)
{
}

ConCommandBase::~ConCommandBase()
{
	Unregister( this );
}

//-----------------------------------------------------------


bool ConCommandBase::HasVariants() const
{
	return m_fnVariantsList != nullptr;
}

void ConCommandBase::GetVariants(Array<EqString>& list, const char* query) const
{
	if(m_fnVariantsList != nullptr)
		( *m_fnVariantsList )(this, list, query);
}

IEXPORTS IConsoleCommands* GetIConsoleCommandsImpl();

// static
void ConCommandBase::Register( ConCommandBase* pBase )
{
	GetIConsoleCommandsImpl()->RegisterCommand( pBase );
}

// static
void ConCommandBase::Unregister( ConCommandBase* pBase )
{
	if (!pBase->m_bIsRegistered)
		return;
	GetIConsoleCommandsImpl()->UnregisterCommand(pBase);
}