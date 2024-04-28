//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium localization
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/ILocalize.h"

//--------------------------------------------------------------
// Localize token
//--------------------------------------------------------------
class CLocToken : public ILocToken
{
	friend class CLocalize;

public:
	CLocToken() = default;
	CLocToken(const char* tok, const wchar_t* text, bool customToken);
	CLocToken(const char* tok, const char* text, bool customToken);

	const char*		GetToken() const	{return m_token.ToCString();}
	const wchar_t*	GetText() const		{return m_text.ToCString();}

private:
	EqString		m_token;
	EqWString		m_text;
	bool			m_customToken{ false };
};

//--------------------------------------------------------------
// Token cache
//--------------------------------------------------------------
class CLocalize : public ILocalize
{
public:
	CLocalize();
	~CLocalize();

	void				Init();
	void				Shutdown();

	void				ReloadLanguageFiles();

	void				SetLanguageName(const char* name);
	const char*			GetLanguageName() const;

	void				AddTokensFile(const char* pszFilePrefix);
	void				AddToken(const char* token, const wchar_t* pszTokenString);
	void				AddToken(const char* token, const char* pszUTF8TokenString);

	const wchar_t*		GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken = 0) const;
	const ILocToken*	GetToken( const char* pszToken ) const;

	bool				IsInitialized() const {return m_languageFilePrefixes.numElem() > 0;}

private:
	void				ParseLanguageFile(const char* pszFilePrefix, bool reload = false);

	const ILocToken*	_FindToken( const char* pszToken ) const;

	Map<int,CLocToken>	m_tokens{ PP_SL };
	Array<EqString>		m_languageFilePrefixes{ PP_SL };
	EqString			m_language;
};
