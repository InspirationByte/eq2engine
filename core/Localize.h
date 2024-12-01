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
	CLocToken(int fileIdx, const char* tok, const wchar_t* text);
	CLocToken(int fileIdx, const char* tok, const char* text);

	const char*		GetToken() const	{return m_token.ToCString();}
	const wchar_t*	GetText() const		{return m_text.ToCString();}

private:
	EqString		m_token;
	EqWString		m_text;
	int				m_fileIdx{ -1 };
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
	void				RemoveTokensFile(const char* pszFilePrefix);

	const ILocToken* 	AddToken(const char* token, const wchar_t* pszTokenString);
	const ILocToken* 	AddToken(const char* token, const char* pszUTF8TokenString);

	const wchar_t*		GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken = 0) const;
	const ILocToken*	GetToken( const char* pszToken ) const;

	bool				IsInitialized() const {return m_languageFilePrefixes.numElem() > 0;}

private:
	void				ParseLanguageFile(int langFileIdx, bool reload = false);

	const ILocToken*	_FindToken( const char* pszToken ) const;

	Map<int, CLocToken>	m_tokens{ PP_SL };
	Array<EqString>		m_languageFilePrefixes{ PP_SL };
	Array<int>			m_freeSlots{ PP_SL };
	EqString			m_language;
};
