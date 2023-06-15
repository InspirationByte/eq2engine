//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"
#include "ConCommandBase.h"
#include "IConsoleCommands.h"

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

// registering and unregistering commands must be done more internally without singletons
IEXPORTS IConsoleCommands* GetCConsoleCommands( void );

bool ConCommandBase::HasVariants() const
{
	return m_fnVariantsList != nullptr;
}

void ConCommandBase::GetVariants(Array<EqString>& list, const char* query) const
{
	if(m_fnVariantsList != nullptr)
		( *m_fnVariantsList )(this, list, query);
}

// static
void ConCommandBase::Register( ConCommandBase* pBase )
{
	if (pBase->m_nFlags & CV_UNREGISTERED)
		return;

    GetCConsoleCommands()->RegisterCommand( pBase );
}

// static
void ConCommandBase::Unregister( ConCommandBase* pBase )
{
	if (!pBase->m_bIsRegistered)
		return;
	GetCConsoleCommands()->UnregisterCommand(pBase);
}