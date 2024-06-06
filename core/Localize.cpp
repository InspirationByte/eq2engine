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
	if (!CString::CompareCaseIns(pVar->GetString(), pszOldValue))
		return;

	if (!g_localizer->IsInitialized())
		return;

	g_localizer->SetLanguageName(pVar->GetString());
	g_localizer->ReloadLanguageFiles();
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
	: m_token(tok)
	, m_text(text)
	, m_customToken(customToken)
{
	ReplaceStrFmt(m_text);
}

CLocToken::CLocToken(const char* tok, const char* text, bool customToken)
	: m_token(tok)
	, m_customToken(customToken)
{
	AnsiUnicodeConverter(m_text, text);
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
		CLocToken& token = *it;
		if (token.m_customToken)
			continue;

		// de-localize token
		token.m_token.Clear();
		AnsiUnicodeConverter(token.m_text, token.m_token);
	}

	for (EqString& tokenFilePrefix : m_languageFilePrefixes)
		ParseLanguageFile(tokenFilePrefix, true);

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
	if (arrayFindIndex(m_languageFilePrefixes, EqStringRef(pszFilePrefix)) != -1)
	{
		ASSERT_FAIL("CLocalize: %s is already loaded", pszFilePrefix);
		return;
	}

	m_languageFilePrefixes.append(pszFilePrefix);
	ParseLanguageFile(pszFilePrefix);
}

void CLocalize::ParseLanguageFile(const char* pszFilePrefix, bool reload)
{
	const EqString textFilePath = EqString::Format("resources/text_%s/%s.txt", GetLanguageName(), pszFilePrefix);

	VSSize fileSize = 0;
	char* fileData = reinterpret_cast<char*>(g_fileSystem->GetFileBuffer(textFilePath, &fileSize));
	if (!fileData)
		return;

	char* fileStart = fileData;
	defer{
		PPFree(fileData);
	};

	// check for BOM and skip if needed
	{
		const ushort byteordermark = *((ushort*)fileStart);
		if (byteordermark == 0xbbef)
		{
			// skip this three byte bom
			fileStart += 3;
			fileSize -= 3;
		}
		else if (byteordermark == 0xfeff)
		{
			MsgError("%s: Only UTF-8 language files supported", textFilePath.ToCString());
			return;
		}
	}

	FixedArray<EqString, 2> currentTokens;

	KV_Tokenizer(fileStart, fileSize, textFilePath, [&](int line, const char* dataPtr, const char* sig, va_list args) {
		switch (*sig)
		{
		case 't':   // text token
		{
			const char* text = va_arg(args, char*);
			currentTokens.append(text);

			break;
		}
		case 'b': // break
		{
			if (currentTokens.numElem() == 2)
			{
				// Cannot add same one
				if (!reload && _FindToken(currentTokens[0]))
				{
					MsgWarning("Localization warning (%s): Token '%s' already registered\n", pszFilePrefix, currentTokens[0].ToCString());
					currentTokens.clear();
					break;
				}

				LocalizeConvertSymbols((char*)currentTokens[1].GetData(), true);

				const int hash = StringToHash(currentTokens[0].ToCString(), true);
				m_tokens.insert(hash, CLocToken(currentTokens[0], currentTokens[1], false));
			}

			currentTokens.clear();
			break;
		}
		}

		return KV_PARSE_RESUME;
	});
}

const ILocToken* CLocalize::AddToken(const char* token, const wchar_t* pszTokenString)
{
	if(token == nullptr || pszTokenString == nullptr)
		return nullptr;

	LocalizeConvertSymbols( (char*)pszTokenString, true );

	const int hash = StringToHash(token, true);
	auto newIt = m_tokens.insert(hash, CLocToken(token, pszTokenString, true));
	return &newIt.value();
}

const ILocToken* CLocalize::AddToken(const char* token, const char* pszUTF8TokenString)
{
	if(token == nullptr || pszUTF8TokenString == nullptr)
		return nullptr;

	LocalizeConvertSymbols( (char*)pszUTF8TokenString, true );

	const int hash = StringToHash(token, true);
	auto newIt = m_tokens.insert(hash, CLocToken(token, pszUTF8TokenString, true));
	return &newIt.value();
}

const wchar_t* CLocalize::GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken) const
{
	const ILocToken* foundTok = GetToken(pszToken);

	if(!foundTok)
	{
		DevMsg(DEVMSG_LOCALE, "Localization warning - no text for token '%s'\n", pszToken);
	}

	return foundTok ? foundTok->GetText() : pszDefaultToken;
}

const ILocToken* CLocalize::GetToken( const char* pszToken ) const
{
	const ILocToken* foundTok = _FindToken(pszToken);

	if(!foundTok)
	{
		DevMsg(DEVMSG_LOCALE, "Localization warning - token '%s' not found\n", pszToken);
	}

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