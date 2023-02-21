//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"
#include "ConVar.h"
#include "ICommandLine.h"

#pragma warning(disable: 4267)

ConVar::~ConVar()
{
}

ConVar::ConVar(const char* name, const char* pszDefaultValue, int flags)
	: ConCommandBase(name, flags | CMDBASE_CONVAR)
{
	Init(pszDefaultValue);
}

void ConVar::Init(const char* defaultValue)
{
	m_szDefaultValueString = defaultValue;
	InternalSetValue(GetDefaultValue());
}

void ConVar::SetClamp(bool bClamp,float fMin,float fMax)
{
	m_bClamp = bClamp;
	m_fClampMin = fMin;
	m_fClampMax = fMax;
}

void ConVar::SetCallback(CONVAR_CHANGE_CALLBACK newCallBack)
{
	m_fnChangeCallback = newCallBack;
}

void ConVar::RevertToDefaultValue()
{
	InternalSetValue( GetDefaultValue() );
}

const char*	ConVar::GetDefaultValue() const
{
	static const char* empty_string = "";
	return m_szDefaultValueString ? m_szDefaultValueString : empty_string;
}

void ConVar::SetValue(const char* newvalue)
{
	InternalSetValue(newvalue);
}

void ConVar::InternalSetValue(const char* newvalue)
{
	if(newvalue == nullptr)
		return;

	float fNewValue;
	char  tempVal[ 32 ];
	char  *val;

	val = (char *)newvalue;
	fNewValue = ( float )atof( newvalue );

	if ( ClampValue( fNewValue ) )
	{
        snprintf( tempVal,sizeof(tempVal), "%f", fNewValue );
		val = tempVal;
	}

	// Redetermine value
	m_flValue		= fNewValue;
	m_nValue		= ( int )( m_flValue );

	InternalSetString( val );
}

//-----------------------------------------------------------------------------
// Check whether to clamp and then perform clamp
//-----------------------------------------------------------------------------
bool ConVar::ClampValue( float& value )
{
	if (m_bClamp && value < m_fClampMin)
	{
		value = m_fClampMin;
		return true;
	}

	if (m_bClamp && value > m_fClampMax)
	{
		value = m_fClampMax;
		return true;
	}

	return false;
}

void ConVar::InternalSetString(const char* value)
{
	int len = strlen(value) + 1;
	
	char* pszOldValue = (char*)stackalloc(m_szValueString.Length()+1 );
	pszOldValue[0] = '\0';

	if (m_fnChangeCallback)
	{
		strncpy(pszOldValue, m_szValueString.ToCString(), m_szValueString.Length() + 1);
		pszOldValue[m_szValueString.Length()] = '\0';
	}

	m_szValueString = value;

	if ( m_fnChangeCallback )
	{
		(m_fnChangeCallback)(this, pszOldValue);
	}

	stackfree( pszOldValue );
}


void ConVar::SetFloat(const float newvalue)
{
	if(m_flValue != newvalue)
		SetValue(EqString::Format("%f",newvalue).ToCString());
}

void ConVar::SetInt(const int newvalue)
{
	if (m_nValue != newvalue)
		SetValue(EqString::Format("%i",newvalue).ToCString());
}

void ConVar::SetBool(const bool newvalue)
{
	int newnValue = (int)newvalue;
	if(m_nValue != newnValue)
		SetValue(EqString::Format("%i",(int)newvalue).ToCString());
}

bool ConVar::CheckCommandLine(int startAt/* = 0 */)
{
	int indx = g_cmdLine->FindArgument(EqString::Format("+%s",GetName()).ToCString(),startAt);
	if(indx != -1)
		return true;
	else
		return false;
}
