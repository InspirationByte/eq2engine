//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////


#ifndef CONVAR_H
#define CONVAR_H

#include <stdio.h>
#include <stdlib.h>
#include "ConCommandBase.h"

// The Microsoft C++ Compiler fixes the typedef problem
// Buf for linux we needs this:
class ConVar;

typedef void (*CONVAR_CHANGE_CALLBACK)(ConVar* pVar,char const* pszOldValue);

class ConVar : public ConCommandBase
{
public:
	ConVar();
	~ConVar();
	ConVar(char const *name,char const *pszDefaultValue, char const *desc = 0, int flags = 0);
	ConVar(char const *name,char const *pszDefaultValue, CONVAR_CHANGE_CALLBACK callback, char const *desc = 0, int flags = 0);

	ConVar(char const *name,char const *pszDefaultValue, CMDBASE_VARIANTS_CALLBACK variantsList, char const *desc = 0, int flags = 0);
	ConVar(char const *name,char const *pszDefaultValue, CONVAR_CHANGE_CALLBACK callback, CMDBASE_VARIANTS_CALLBACK variantsList, char const *desc = 0, int flags = 0);

	ConVar(char const *name,char const *pszDefaultValue, float clampmin,float clampmax, char const *desc = 0, int flags = 0);
	ConVar(char const *name,char const *pszDefaultValue, float clampmin,float clampmax, CONVAR_CHANGE_CALLBACK callback,char const *desc = 0, int flags = 0);

	void			RevertToDefaultValue(); //Reverts to default value
	const char*		GetDefaultValue() const;

	void			SetValue(char const *newvalue); //Sets string value
	void			SetFloat(const float newvalue);
	void			SetInt(const int newvalue);
	void			SetBool(const bool newvalue);

	void			SetClamp(bool bClamp,float fMin,float fMax);

	void			SetCallback(CONVAR_CHANGE_CALLBACK newCallBack);

	bool			HasClamp() const	{return m_bClamp;}
	float			GetMinClamp() const {return m_fClampMin;}
	float			GetMaxClamp() const {return m_fClampMax;}

	// Value returning
	float			GetFloat() const	{return m_flValue;}
	const char*		GetString() const	{return m_szValueString.ToCString();}
	int				GetInt() const		{return m_nValue;}
	bool			GetBool() const		{return (m_nValue > 0);}

	virtual void	LuaCleanup();

private:

	bool			CheckCommandLine(int startAt = 0 );

	void			Create(char const *pszName,char const *pszDefaultValue,char const *pszHelpString, int nFlags);
	void			Create(char const *pszName,char const *pszDefaultValue,float fClampMin,float fClampMax,char const *pszHelpString, int nFlags);

	void			InternalSetValue(char const *newvalue);
	void			InternalSetString(char const *value);

	bool			ClampValue(float &value);

private:

	// Static data
	char const*				m_szDefaultValueString;

	CONVAR_CHANGE_CALLBACK	m_fnChangeCallback;

	// Dynamically allocated value
	EqString				m_szValueString;

	// Other values
	float					m_flValue;
	int						m_nValue;

	// Is clamped
	bool					m_bClamp;

	float					m_fClampMin;
	float					m_fClampMax;
};

#define DECLARE_CVAR(name,value,desc,flags)		\
	static ConVar name(#name,#value,desc,flags);

#define DECLARE_CVAR_NONSTATIC(name,value,desc,flags)		\
	ConVar name(#name,#value,desc,flags);

#define DECLARE_CVAR_CLAMPED(name,value,minclamp,maxclamp,desc,flags)		\
	static ConVar name(#name,#value,minclamp,maxclamp,desc,flags);

#define DECLARE_CVAR_CLAMPED_NONSTATIC(name,value,minclamp,maxclamp,desc,flags)		\
	ConVar name(#name,#value,minclamp,maxclamp,desc,flags);

// If you has error in your input
#define DECLARE_CVAR_CLAMP				DECLARE_CVAR_CLAMPED
#define DECLARE_CVAR_CLAMP_NONSTATIC	DECLARE_CVAR_CLAMPED_NONSTATIC

#define HOOK_TO_CVAR(name)		\
	static ConVar *name = (ConVar*)g_consoleCommands->FindCvar(#name);

#endif //_CONVARSYSTEM_H_
