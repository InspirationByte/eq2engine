//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech localization
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//--------------------------------------------------------------
// Localize token
//--------------------------------------------------------------
class ILocToken
{
public:
	virtual	~ILocToken() {}

	virtual const char*		GetToken() const = 0;
	virtual const wchar_t*	GetText() const = 0;
};

//--------------------------------------------------------------
// Token cache
//--------------------------------------------------------------
class ILocalize : public IEqCoreModule
{
public:
	CORE_INTERFACE("E2_Localizer_003")

	virtual void				Init() = 0;
	virtual void				Shutdown() = 0;

	virtual void				ReloadLanguageFiles() = 0;

	virtual void				SetLanguageName(const char* name) = 0;
	virtual const char*			GetLanguageName() const = 0;

	virtual void				AddTokensFile(const char* pszFilePrefix) = 0;
	virtual void				AddToken(const char* token, const wchar_t* pszTokenString) = 0;
	virtual void				AddToken(const char* token, const char* pszTokenString) = 0;

	virtual const wchar_t*		GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken = 0) const = 0;
	virtual const ILocToken*	GetToken( const char* pszToken ) const = 0;
};

//--------------------------------------------------------------

INTERFACE_SINGLETON( ILocalize, CLocalize, g_localizer )

#define DKLOC_CONCAT(x) L ## x
#define DKLOC(tok, def) g_localizer->GetTokenString( tok, DKLOC_CONCAT(def) )

inline const wchar_t* LocalizedString( const char* pszString )
{
	static EqWString defaultUnicodeString;
	defaultUnicodeString.Empty();

	if(!pszString || (pszString && pszString[0] == '\0'))
		return defaultUnicodeString;

	EqStringConv::CUTF8Conv(defaultUnicodeString, pszString);

	if(pszString[0] == L'#')
	{
		return g_localizer->GetTokenString(pszString+1, defaultUnicodeString+1);
	}
	else
	{
		return defaultUnicodeString;
	}

}
