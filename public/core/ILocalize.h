//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech localization
//////////////////////////////////////////////////////////////////////////////////

#ifndef ILOCALIZE
#define ILOCALIZE

#include "IDkCore.h"

#define LOCALIZER_INTERFACE_VERSION		"CORE_Localizer_002"

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
class ILocalize
{
public:
							~ILocalize() {}
	virtual void			Init() = 0;

	virtual const char*		GetLanguageName() = 0;

	virtual void			AddTokensFile(const char* pszFilePrefix) = 0;
	virtual void			AddToken(const char* token, const wchar_t* pszTokenString) = 0;
	virtual void			AddToken(const char* token, const char* pszTokenString) = 0;

	virtual const wchar_t*	GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken = 0) = 0;
	virtual ILocToken*		GetToken( const char* pszToken ) = 0;
};

//--------------------------------------------------------------

#ifndef _DKLAUNCHER_

INTERFACE_SINGLETON( ILocalize, CLocalize, LOCALIZER_INTERFACE_VERSION, g_localizer )

#define DKLOC(tok, def) g_localizer->GetTokenString( tok, def )

inline int ConvertBytesToUnicode(const char *pszData, wchar_t *pszUnicode, int nUnicodeBufferBytes)
{
#ifdef _WIN32
	int chars = MultiByteToWideChar(CP_UTF8, 0, pszData, -1, pszUnicode, nUnicodeBufferBytes / sizeof(wchar_t));
	pszUnicode[(nUnicodeBufferBytes / sizeof(wchar_t)) - 1] = 0;
	return chars;
#else
	// USE ICONV

	ASSERTMSG(false, "ConvertBytesToUnicode - not implemented on linux");
	return 0;
#endif
}

inline const wchar_t* LocalizedString( const char* pszString )
{
	static wchar_t defaultUnicodeString[4096];
	defaultUnicodeString[0] = '\0';

	if(!pszString || (pszString && pszString[0] == '\0'))
		return defaultUnicodeString;

	ConvertBytesToUnicode( pszString, defaultUnicodeString, 4095 );

	if(pszString[0] == L'#')
		return g_localizer->GetTokenString(pszString+1, defaultUnicodeString+1);
	else
		return defaultUnicodeString;
}


#endif // _DKLAUNCHER_

#endif //ILOCALIZE
