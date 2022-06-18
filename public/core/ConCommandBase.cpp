//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable - base class
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"
#include "ConCommandBase.h"
#include "IConsoleCommands.h"

static const char* defaultDescString = "No description";

// Default constructor
ConCommandBase::ConCommandBase()
{
	m_fnVariantsList = nullptr;
	m_bIsRegistered	= false;
	m_nFlags = 0;

	m_szName	= nullptr;
	m_szDesc	= nullptr;
}

void ConCommandBase::Init(char const *name,char const *desc, int flags /*= 0*/,bool bIsConVar /* = false*/ )
{
	// Make Command name shared
	m_szName = name;

	if(desc)
		m_szDesc = desc;
	else
		m_szDesc = defaultDescString;

	m_nFlags = flags;

	if(bIsConVar)
		m_nFlags |= CMDBASE_CONVAR;
	else
		m_nFlags |= CMDBASE_CONCOMMAND;

	if(GetFlags() & CV_UNREGISTERED)
		return;

    Register(this);
}

ConCommandBase::~ConCommandBase()
{
	if(m_bIsRegistered) Unregister( this );
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
    GetCConsoleCommands()->RegisterCommand( pBase );
}

// static
void ConCommandBase::Unregister( ConCommandBase* pBase )
{
	GetCConsoleCommands()->UnregisterCommand(pBase);
}