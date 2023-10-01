//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium localization
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/IConsoleCommands.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"
#include "Localize.h"

EXPORTED_INTERFACE(ILocalize, CLocalize);

static void languageChangeCb(ConVar* pVar, char const* pszOldValue)
{
	if (!stricmp(pVar->GetString(), pszOldValue))
		return;

	if (!g_localizer->IsInitialized())
		return;

	MsgWarning("Language will be changed upon game restart\n");
#if 0
	g_localizer->SetLanguageName(pVar->GetString());
	g_localizer->ReloadLanguageFiles();
#endif
}
DECLARE_CVAR_CHANGE(language, "", languageChangeCb, "The language of the game", CV_UNREGISTERED | CV_ARCHIVE);

static void LocalizeConvertSymbols(char* str, bool doNewline)
{
	char c = *str;
	char nextchar = *(str+1);

	while(nextchar != '\0')
	{
		c = *str;
		nextchar = *(str + 1);

		if(c == '\\')
		{
			switch(nextchar)
			{
				case 'n':
				{
					if(doNewline)
					{
						str[0] = ' ';
						str[1] = '\n';
						str++;
					}
					break;
				}
				case 't':
				{
					str[0] = ' ';
					str[1] = '\t';
					str++;
					break;
				}
			}
		}

		str++;
	}
}

//-------------------------------------------------------------------------------------------------

static void ReplaceStrFmt(EqWString& str)
{
	int found = 0;
	do {
		found = str.ReplaceSubstr(L"%s", L"%ls", false, found);
	} while (found != -1);
}

CLocToken::CLocToken(const char* tok, const wchar_t* text, bool customToken)
	: m_token(tok), m_text(text), m_customToken(customToken)
{
	ReplaceStrFmt(m_text);
}

CLocToken::CLocToken(const char* tok, const char* text, bool customToken)
	: m_token(tok), m_text(text), m_customToken(customToken)
{
	ReplaceStrFmt(m_text);
}

//-------------------------------------------------------------------------------------------------

CLocalize::CLocalize()
{
	g_eqCore->RegisterInterface(this);
}

CLocalize::~CLocalize()
{
	g_eqCore->UnregisterInterface<CLocalize>();
}

void CLocalize::Init()
{
	ConCommandBase::Register(&language);
	SetLanguageName(language.GetString());

	m_language = m_language.LowerCase();

	// add the copyright
	AddToken("INSCOPYRIGHT", L"\xa9 Inspiration Byte 2009-2023");
	AddTokensFile("core");
}

void CLocalize::Shutdown()
{
	ConCommandBase::Unregister(&language);
	m_tokens.clear(true);
	m_languageFilePrefixes.clear(true);
}

void CLocalize::ReloadLanguageFiles()
{
	// only delete tokens which has been added by the files
	for (auto it = m_tokens.begin(); !it.atEnd(); ++it)
	{
		if (!it.value().m_customToken)
			m_tokens.remove(it);
	}

	for (EqString& tokenFilePrefix : m_languageFilePrefixes)
		ParseLanguageFile(tokenFilePrefix);

	Msg("Language files reloaded: %d\n", m_languageFilePrefixes.numElem());
}

void CLocalize::SetLanguageName(const char* name)
{
	// first need to change localize path
	g_fileSystem->RemoveSearchPath("$LOCALIZE$");

	m_language = name;

	// try using EqConfig regional settings instead
	if (m_language.Length() == 0)
	{
		const KVSection* pRegional = g_eqCore->GetConfig()->FindSection("RegionalSettings", KV_FLAG_SECTION);
		if (!pRegional)
		{
			Msg("Core config missing RegionalSettings section... force english!\n");
			return;
		}
		const KVSection* defaultLanguage = pRegional ? pRegional->FindSection("DefaultLanguage") : nullptr;
		m_language = KV_GetValueString(defaultLanguage, 0, "english");
	}

	// add localized path
	EqString localizedPath(g_fileSystem->GetCurrentGameDirectory() + _Es("_") + m_language);
	g_fileSystem->AddSearchPath("$LOCALIZE$", localizedPath);

	m_language = m_language.LowerCase();

	Msg("Language '%s' set\n", m_language.ToCString());
}

const char* CLocalize::GetLanguageName() const
{
	return m_language.GetData();
}

void CLocalize::AddTokensFile(const char* pszFilePrefix)
{
	if (arrayFindIndex(m_languageFilePrefixes, pszFilePrefix) != -1)
	{
		ASSERT_FAIL("CLocalize: %s is already loaded", pszFilePrefix);
		return;
	}

	m_languageFilePrefixes.append(pszFilePrefix);
	ParseLanguageFile(pszFilePrefix);
}

void CLocalize::ParseLanguageFile(const char* pszFilePrefix)
{
	EqString path = EqString::Format("resources/text_%s/%s.txt", GetLanguageName(), pszFilePrefix);

	KVSection kvSec;
	if (!KV_LoadFromFile(path, -1, &kvSec))
	{
		MsgWarning("Cannot load language file '%s'\n", path.ToCString());
		return;
	}

	if(!kvSec.unicode)
		MsgWarning("Localization warning (%s): file is not unicode\n", path.ToCString());

	for(const KVSection* key : kvSec.keys)
	{
		if(!stricmp(key->name, "#include" ))
		{
			ParseLanguageFile( KV_GetValueString(key) );
			continue;
		}

		// Cannot add same one
		if(_FindToken( key->name ))
		{
			MsgWarning("Localization warning (%s): Token '%s' already registered\n", pszFilePrefix, key->name );
			continue;
		}

		const char* pszUTF8TokenString = KV_GetValueString(key);
		LocalizeConvertSymbols((char*)pszUTF8TokenString, true);

		const int hash = StringToHash(key->name, true);
		m_tokens.insert(hash, CLocToken(key->name, pszUTF8TokenString, false));
	}
}

void CLocalize::AddToken(const char* token, const wchar_t* pszTokenString)
{
	if(token == nullptr || pszTokenString == nullptr)
		return;

	LocalizeConvertSymbols( (char*)pszTokenString, true );

	const int hash = StringToHash(token, true);
	m_tokens.insert(hash, CLocToken(token, pszTokenString, true));
}

void CLocalize::AddToken(const char* token, const char* pszUTF8TokenString)
{
	if(token == nullptr || pszUTF8TokenString == nullptr)
		return;

	LocalizeConvertSymbols( (char*)pszUTF8TokenString, true );

	const int hash = StringToHash(token, true);
	m_tokens.insert(hash, CLocToken(token, pszUTF8TokenString, true));
}

const wchar_t* CLocalize::GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken) const
{
	const ILocToken* foundTok = GetToken(pszToken);

	return foundTok ? foundTok->GetText() : pszDefaultToken;
}

const ILocToken* CLocalize::GetToken( const char* pszToken ) const
{
	const ILocToken* foundTok = _FindToken(pszToken);

	if(!foundTok)
		DevMsg(DEVMSG_LOCALE, "LOCALIZER: Token '%s' not found\n", pszToken);

	return foundTok;
}

const ILocToken* CLocalize::_FindToken( const char* pszToken ) const
{
	const int tokHash = StringToHash(pszToken, true);

	auto it = m_tokens.find(tokHash);

	if (!it.atEnd())
		return &it.value();

	return nullptr;
}