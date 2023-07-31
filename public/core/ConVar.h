//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ConCommandBase.h"

class ConVar;
typedef void (*CONVAR_CHANGE_CALLBACK)(ConVar* pVar, char const* pszOldValue);

class ConVar : public ConCommandBase
{
public:
	// variadic dynamic initializer for ConVar
	struct ChangeCallback
	{
		ChangeCallback(CONVAR_CHANGE_CALLBACK cb) : cb(cb) {}
		void operator()(ConVar* cv) { cv->m_fnChangeCallback = cb; }

		CONVAR_CHANGE_CALLBACK cb;
	};

	struct Clamp
	{
		Clamp(float fmin, float fmax) : fmin(fmin), fmax(fmax) {}
		void operator()(ConVar* cv) { cv->m_fClampMin = fmin; cv->m_fClampMax = fmax; cv->m_bClamp = true; }

		float fmin;
		float fmax;
	};

	~ConVar();
	ConVar(const char* name, const char* pszDefaultValue, int flags = 0);

	template<typename... Ts>
	ConVar(char const* name, char const* pszDefaultValue, int flags, Ts... ts)
		: ConCommandBase(name, flags | CMDBASE_CONVAR)
	{
		char _[] = { CmdInit<>(this, ts)... };
		Init(pszDefaultValue);
	}

	void			RevertToDefaultValue(); //Reverts to default value
	const char*		GetDefaultValue() const;

	void			SetValue(const char* newvalue); //Sets string value
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

private:

	void			Init(const char* defaultValue);

	bool			CheckCommandLine(int startAt = 0 );

	void			InternalSetValue(const char* newvalue);
	void			InternalSetString(const char* value);

	bool			ClampValue(float &value);

	// Static data
	char const*				m_szDefaultValueString{ nullptr };
	CONVAR_CHANGE_CALLBACK	m_fnChangeCallback{ nullptr };

	// Dynamically allocated value
	EqString				m_szValueString;

	// Other values
	float					m_flValue{ 0.0f };
	int						m_nValue{ 0 };

	// Is clamped
	float					m_fClampMin{ 0.0f };
	float					m_fClampMax{ 1.0f };
	bool					m_bClamp{ false };
};

#define DECLARE_CVAR_RENAME_G(name, cvname, value, desc, flags) \
	ConVar name(cvname, value, flags, ConVar::Desc(desc));

#define DECLARE_CVAR_G(name, value, desc, flags) \
	ConVar name(#name, value, flags, ConVar::Desc(desc));

#define DECLARE_CVAR_CLAMP_G(name, value, minclamp, maxclamp, desc, flags) \
	ConVar name(#name, value, flags, ConVar::Desc(desc), ConVar::Clamp(minclamp, maxclamp));

#define DECLARE_CVAR_CHANGE_G(name, value, changecb, desc, flags) \
	ConVar name(#name, value, flags, ConVar::Desc(desc), ConVar::ChangeCallback(changecb));

#define DECLARE_CVAR_CLAMP_CHANGE_G(name, value, changecb, minclamp, maxclamp, desc, flags) \
	ConVar name(#name, value, flags, ConVar::Desc(desc), ConVar::Clamp(minclamp, maxclamp), ConVar::ChangeCallback(changecb));

#define DECLARE_CVAR_VARIANTS_G(name, value, variantscb, desc, flags) \
	ConVar name(#name, value, flags, ConVar::Desc(desc), ConVar::Variants(variantscb));

#define DECLARE_CVAR(name, value, desc, flags) \
	static DECLARE_CVAR_G(name, value, desc, flags)

#define DECLARE_CVAR_RENAME(name, cvname, value, desc, flags) \
	static DECLARE_CVAR_RENAME_G(name, cvname, value, desc, flags)

#define DECLARE_CVAR_CLAMP(name, value, minclamp, maxclamp, desc, flags) \
	static DECLARE_CVAR_CLAMP_G(name, value, minclamp, maxclamp, desc, flags)

#define DECLARE_CVAR_CHANGE(name, value, changecb, desc, flags) \
	DECLARE_CVAR_CHANGE_G(name, value, changecb, desc, flags)

#define DECLARE_CVAR_CLAMP_CHANGE(name, value, changecb, minclamp, maxclamp, desc, flags) \
	DECLARE_CVAR_CLAMP_CHANGE_G(name, value, changecb, minclamp, maxclamp, desc, flags)

#define DECLARE_CVAR_VARIANTS(name, value, variantscb, desc, flags) \
	DECLARE_CVAR_VARIANTS_G(name, value, variantscb, desc, flags)

#define HOOK_TO_CVAR(name)		\
	static ConVar *name = (ConVar*)g_consoleCommands->FindCvar(#name);
