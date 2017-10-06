//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech localization
//////////////////////////////////////////////////////////////////////////////////

#ifndef CLOCALIZE
#define CLOCALIZE

#include "ILocalize.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"
#include "utils/eqwstring.h"

//--------------------------------------------------------------
// Localize token
//--------------------------------------------------------------
class CLocToken : public ILocToken
{
	friend class CLocalize;

public:
					CLocToken(const char* tok, const wchar_t* text);
					CLocToken(const char* tok, const char* text);

	const char*		GetToken() const	{return m_token.c_str();}
	const wchar_t*	GetText() const		{return m_text.c_str();}

private:
	EqString		m_token;
	EqWString		m_text;

	int				m_tokHash;
};

//--------------------------------------------------------------
// Token cache
//--------------------------------------------------------------
class CLocalize : public ILocalize
{
public:
						CLocalize() {}
						~CLocalize() {}

	void				Init();
	void				Shutdown();

	const char*			GetLanguageName();

	void				AddTokensFile(const char* pszFilePrefix);
	void				AddToken(const char* token, const wchar_t* pszTokenString);
	void				AddToken(const char* token, const char* pszTokenString);

	const wchar_t*		GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken = 0);
	ILocToken*			GetToken( const char* pszToken );

	bool				IsInitialized() const {return m_language.Length() > 0;}
	const char*			GetInterfaceName() const {return LOCALIZER_INTERFACE_VERSION;}

private:
	DkList<CLocToken*>	m_tokens;
	EqString			m_language;
};

#endif //CLOCALIZE
