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

// Default constructor
ConVar::ConVar() : ConCommandBase()
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;
}

ConVar::~ConVar()
{
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,char const *desc, int flags /* = 0 */) : ConCommandBase()
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,desc,flags);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,CONVAR_CHANGE_CALLBACK callback,char const *desc, int flags/* = 0*/) : ConCommandBase()
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,desc,flags);

	SetCallback(callback);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue, CMDBASE_VARIANTS_CALLBACK variantsList, char const *desc, int flags)
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,desc,flags);

	SetVariantsCallback(variantsList);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue, CONVAR_CHANGE_CALLBACK callback, CMDBASE_VARIANTS_CALLBACK variantsList, char const *desc, int flags)
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,desc,flags);

	SetCallback(callback);
	SetVariantsCallback(variantsList);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,float clampmin,float clampmax,char const *desc /*= 0*/, int flags /*= 0*/) : ConCommandBase()
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,clampmin,clampmax,desc,flags);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,float clampmin,float clampmax,CONVAR_CHANGE_CALLBACK callback,char const *desc /*= 0*/, int flags /*= 0*/) : ConCommandBase()
{
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,clampmin,clampmax,desc,flags);

	SetCallback(callback);
}

void ConVar::Create(char const *pszName,char const *pszDefaultValue,char const *pszHelpString, int nFlags)
{
	static const char* empty_string = "";

	// Setup default value first
	m_szDefaultValueString = pszDefaultValue;

	SetValue( GetDefaultValue() );

	// Not clamped by this initializer
	m_bClamp = false;

	// Init base class

	//! Don't link for now
	Init(pszName,pszHelpString,nFlags,true);
}

void ConVar::Create(char const *pszName,char const *pszDefaultValue,float fClampMin,float fClampMax,char const *pszHelpString, int nFlags)
{
	// Setup default value first
	m_szDefaultValueString = pszDefaultValue;

	SetValue( GetDefaultValue() );

	// Clamped by this initializer
	m_bClamp = true;
	m_fClampMin = fClampMin;
	m_fClampMax = fClampMax;


	Init(pszName,pszHelpString,nFlags,true);
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

void ConVar::InternalSetValue(char const *newvalue)
{
	if(newvalue == NULL)
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
	if ( m_bClamp && ( value < m_fClampMin ) )
	{
		value = m_fClampMin;
		return true;
	}

	if ( m_bClamp && ( value > m_fClampMax ) )
	{
		value = m_fClampMax;
		return true;
	}

	return false;
}

void ConVar::InternalSetString(char const *value)
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
