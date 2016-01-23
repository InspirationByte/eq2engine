//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////

#include "ConVar.h"
#include "IConCommandFactory.h"
#include "ICmdLineParser.h"
#include "DebugInterface.h"

#include "utils/DkList.h"
#include "utils/strtools.h"

#include <malloc.h> // alloca

#if defined(_DEBUG) && defined(_WIN32)
#	define _CRTDBG_MAP_ALLOC
#	include <crtdbg.h>
#endif



// Default constructor
ConVar::ConVar() : ConCommandBase()
{
	m_szValueString = NULL;
	m_iStringLength = 0;
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;
}

ConVar::~ConVar()
{
	if (m_szValueString)
	{
		delete[] m_szValueString; // Sometimes errors
		m_szValueString = NULL;
	}
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,char const *desc, int flags /* = 0 */) : ConCommandBase()
{
	m_szValueString = NULL;
	m_iStringLength = 0;
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,desc,flags);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,CONVAR_CHANGE_CALLBACK callback,char const *desc, int flags/* = 0*/) : ConCommandBase()
{
	m_szValueString = NULL;
	m_iStringLength = 0;
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,desc,flags);

	if(callback)
		SetCallback(callback);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,float clampmin,float clampmax,char const *desc /*= 0*/, int flags /*= 0*/) : ConCommandBase()
{
	m_szValueString = NULL;
	m_iStringLength = 0;
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,clampmin,clampmax,desc,flags);
}

ConVar::ConVar(char const *name,char const *pszDefaultValue,float clampmin,float clampmax,CONVAR_CHANGE_CALLBACK callback,char const *desc /*= 0*/, int flags /*= 0*/) : ConCommandBase()
{
	m_szValueString = NULL;
	m_iStringLength = 0;
	m_nValue = 0;
	m_bClamp = false;
	m_fnChangeCallback = NULL;

	Create(name,pszDefaultValue,clampmin,clampmax,desc,flags);

	if(callback)
		SetCallback(callback);
}

void ConVar::Create(char const *pszName,char const *pszDefaultValue,char const *pszHelpString, int nFlags)
{
	static const char* empty_string = "";

	// Setup default value first
	m_szDefaultValueString = pszDefaultValue ? pszDefaultValue : empty_string;

	SetValue(m_szDefaultValueString);
	/*
#ifdef CVAR_USE_STRING_AS_DYNAMICALLY_ALLOCATED_VALUE
	// Setup value
	m_iStringLength = strlen( m_szDefaultValueString ) + 1;
	m_szValueString = m_szDefaultValueString;
#else
	// Setup value
	m_iStringLength = strlen( m_szDefaultValueString ) + 1;
	m_szValueString = new char[m_iStringLength];
	memcpy( m_szValueString, m_szDefaultValueString, m_iStringLength );
#endif

	// Setup numeric values
	m_flValue = (float)atof(m_szValueString);
	m_nValue = atoi(m_szValueString);
	*/
	// Not clamped by this initializer
	m_bClamp = false;

	// Init base class

	//! Don't link for now
	Init(pszName,pszHelpString,nFlags,true);
}

void ConVar::Create(char const *pszName,char const *pszDefaultValue,float fClampMin,float fClampMax,char const *pszHelpString, int nFlags)
{
	static const char* empty_string = "";

	// Setup default value first
	m_szDefaultValueString = pszDefaultValue ? pszDefaultValue : empty_string;

	SetValue(m_szDefaultValueString);

	/*
#ifdef CVAR_USE_STRING_AS_DYNAMICALLY_ALLOCATED_VALUE
	// Setup value
	m_iStringLength = strlen( m_szDefaultValueString ) + 1;
	m_szValueString = m_szDefaultValueString;
#else
	// Setup value
	m_iStringLength = strlen( m_szDefaultValueString ) + 1;
	m_szValueString = new char[m_iStringLength];
	memcpy( m_szValueString, m_szDefaultValueString, m_iStringLength );
#endif


	// Setup numeric values and clamp
	m_flValue = (float)clamp((float)atof(m_szValueString),fClampMin,fClampMax);
	m_nValue = (int)clamp((float)atof(m_szValueString),fClampMin,fClampMax);
	*/
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
	InternalSetValue(m_szDefaultValueString);
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
#ifdef CVAR_USE_STRING_AS_DYNAMICALLY_ALLOCATED_VALUE
	// Setup value
	m_iStringLength = strlen( value ) + 1;
	m_szValueString = value;
#else
	int len = strlen(value) + 1;

	char* pszOldValue = (char*)stackalloc( m_iStringLength+1 );
	pszOldValue[0] = '\0';

	if ( m_fnChangeCallback && m_szValueString )
		memcpy( pszOldValue, m_szValueString, m_iStringLength+1 );

	if (len > m_iStringLength)
	{
		if (m_szValueString)
			delete [] m_szValueString; // Sometimes errors

		m_szValueString = new char[len];
		m_iStringLength = len;
	}

	if (m_szValueString)
		memcpy( m_szValueString, value, len );
#endif
	if ( m_fnChangeCallback )
	{
		(m_fnChangeCallback)(this,pszOldValue);
	}

	stackfree( pszOldValue );
}


void ConVar::SetFloat(const float newvalue)
{
	SetValue(varargs("%f",newvalue));
}

void ConVar::SetInt(const int newvalue)
{
	SetValue(varargs("%i",newvalue));
}

void ConVar::SetBool(const bool newvalue)
{
	SetValue(varargs("%i",(int)newvalue));
}

bool ConVar::CheckCommandLine(int startAt/* = 0 */)
{
	int indx = GetCmdLine()->FindArgument(varargs("+%s",GetName()),startAt);
	if(indx != -1)
		return true;
	else
		return false;
}
