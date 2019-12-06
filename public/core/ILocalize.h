//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech localization
//////////////////////////////////////////////////////////////////////////////////

#ifndef ILOCALIZE
#define ILOCALIZE

#include "platform/Platform.h"
#include "InterfaceManager.h"
#include "utils/strtools.h"

#ifdef PLAT_POSIX
#   include <wchar.h>
#endif // PLAT_POSIX

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
class ILocalize : public ICoreModuleInterface
{
public:
							~ILocalize() {}
	virtual void			Init() = 0;
	virtual void			Shutdown() = 0;

	virtual const char*		GetLanguageName() = 0;

	virtual void			AddTokensFile(const char* pszFilePrefix) = 0;
	virtual void			AddToken(const char* token, const wchar_t* pszTokenString) = 0;
	virtual void			AddToken(const char* token, const char* pszTokenString) = 0;

	virtual const wchar_t*	GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken = 0) const = 0;
	virtual ILocToken*		GetToken( const char* pszToken ) const = 0;


};

//--------------------------------------------------------------

#ifndef _DKLAUNCHER_

INTERFACE_SINGLETON( ILocalize, CLocalize, LOCALIZER_INTERFACE_VERSION, g_localizer )

#define DKLOC(tok, def) g_localizer->GetTokenString( tok, def )

inline const wchar_t* LocalizedString( const char* pszString )
{
	static wchar_t defaultUnicodeString[4096];
	defaultUnicodeString[0] = '\0';

	EqWString convStr;
	EqStringConv::utf8_to_wchar conv(defaultUnicodeString, 4096, pszString);

	if(!pszString || (pszString && pszString[0] == '\0'))
		return defaultUnicodeString;

	if(pszString[0] == L'#')
	{
		return g_localizer->GetTokenString(pszString+1, defaultUnicodeString+1);
	}
	else
	{
		return defaultUnicodeString;
	}

}


#endif // _DKLAUNCHER_

#endif //ILOCALIZE
