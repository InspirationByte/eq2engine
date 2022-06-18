//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium localization
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "Localize.h"

EXPORTED_INTERFACE(ILocalize, CLocalize);

void xstr_loc_convert_special_symbols(char* str, bool doNewline)
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

CLocToken::CLocToken(const char* tok, const wchar_t* text)
	: m_token(tok), m_text(text)
{
	int found = 0;
	do
	{
		found = m_text.ReplaceSubstr(L"%s", L"%ls", false, found);
	} while (found != -1);
};

CLocToken::CLocToken(const char* tok, const char* text)
	: m_token(tok), m_text(text)
{
	int found = 0;
	do
	{
		found = m_text.ReplaceSubstr(L"%s", L"%ls", false, found);
	} while (found != -1);
}

//-------------------------------------------------------------------------------------------------

void CLocalize::Init()
{
	KVSection* pRegional = GetCore()->GetConfig()->FindSection("RegionalSettings", KV_FLAG_SECTION);

	if(!pRegional)
	{
		Msg("Core config missing RegionalSettings section... force english!\n");
		return;
	}

    m_language = KV_GetValueString(pRegional->FindSection("DefaultLanguage"), 0, "english" );
    Msg("Language '%s' set\n", m_language.ToCString());

	// add localized path
	EqString localizedPath(g_fileSystem->GetCurrentGameDirectory() + _Es("_") + m_language);
	g_fileSystem->AddSearchPath("$LOCALIZE$", localizedPath.ToCString());

	int langArg = g_cmdLine->FindArgument("-language");

	if(langArg != -1)
	{
		const char* args = g_cmdLine->GetArgumentsOf(langArg);

		if(strlen(args) > 0)
			m_language = args;
		else
			MsgError("Error: -language must have argument\n");
	}

	// add the copyright
	AddToken("INSCOPYRIGHT", L"\xa9 Inspiration Byte 2009-2021");

	AddTokensFile("core");
}

void CLocalize::Shutdown()
{
	m_tokens.clear();
}

const char* CLocalize::GetLanguageName()
{
	return m_language.GetData();
}

void CLocalize::AddTokensFile(const char* pszFilePrefix)
{
	// TODO: check for already loaded token files!
	EqString path = EqString::Format("resources/text_%s/%s.txt", GetLanguageName(), pszFilePrefix);

	KVSection kvSec;
	if (!KV_LoadFromFile(path.ToCString(), -1, &kvSec))
	{
		MsgWarning("Cannot load language file '%s'\n", path.ToCString());
		return;
	}

	if(!kvSec.unicode)
		MsgWarning("Localization warning (%s): file is not unicode\n", path.ToCString());

	for(int i = 0; i < kvSec.keys.numElem(); i++)
	{
		KVSection* key = kvSec.keys[i];

		if(!stricmp(key->name, "#include" ))
		{
			AddTokensFile( KV_GetValueString(key) );
			continue;
		}

		// Cannot add same one
		if(_FindToken( key->name ))
		{
			MsgWarning("Localization warning (%s): Token '%s' already registered\n", pszFilePrefix, key->name );
			continue;
		}

		AddToken(key->name, KV_GetValueString(key));
	}
}

void CLocalize::AddToken(const char* token, const wchar_t* pszTokenString)
{
	if(token == nullptr || pszTokenString == nullptr)
		return;

	xstr_loc_convert_special_symbols( (char*)pszTokenString, true );

	const int hash = StringToHash(token, true);
	m_tokens.insert(hash, CLocToken(token, pszTokenString));
}

void CLocalize::AddToken(const char* token, const char* pszTokenString)
{
	if(token == nullptr || pszTokenString == nullptr)
		return;

	xstr_loc_convert_special_symbols( (char*)pszTokenString, true );

	const int hash = StringToHash(token, true);
	new(&m_tokens[hash]) CLocToken(token, pszTokenString);
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

	if (it != m_tokens.end())
		return &it.value();

	return nullptr;
}