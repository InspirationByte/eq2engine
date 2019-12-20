//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech localization
//////////////////////////////////////////////////////////////////////////////////

#include "utils/strtools.h"

#include "Localize.h"
#include "DebugInterface.h"
#include "IDkCore.h"
#include "utils/KeyValues.h"

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
{
	m_token = tok;
	m_text = text;

	m_tokHash = StringToHash(tok, true);
}

CLocToken::CLocToken(const char* tok, const char* text)
{
	m_token = tok;
	m_text = text;

	m_tokHash = StringToHash(tok, true);
}

//-------------------------------------------------------------------------------------------------

void CLocalize::Init()
{
	kvkeybase_t* pRegional = GetCore()->GetConfig()->FindKeyBase("RegionalSettings", KV_FLAG_SECTION);

	if(!pRegional)
	{
		Msg("Core config missing RegionalSettings section... force English!\n");
		return;
	}

    m_language = KV_GetValueString(pRegional->FindKeyBase("DefaultLanguage"), 0, "English" );

    Msg("Language '%s' set\n", m_language.c_str());

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
	AddToken("INSCOPYRIGHT", L"\xa9 Inspiration Byte 2019");

	AddTokensFile("core");
}

void CLocalize::Shutdown()
{
	for(int i = 0; i < m_tokens.numElem(); i++)
	{
		delete m_tokens[i];
	}

	m_tokens.clear();
}

const char* CLocalize::GetLanguageName()
{
	return m_language.GetData();
}

void CLocalize::AddTokensFile(const char* pszFilePrefix)
{
	// TODO: check for already loaded token files!
	char path[2048];
	sprintf(path, "resources/%s_%s.txt", pszFilePrefix, GetLanguageName());

	kvkeybase_t kvSec;
	if (!KV_LoadFromFile(path, -1, &kvSec))
	{
		MsgWarning("Cannot load language file 'resources/%s_%s.txt'\n", pszFilePrefix, GetLanguageName());
		return;
	}

	if(!kvSec.unicode)
		MsgWarning("Localization warning (%s): file is not unicode\n", pszFilePrefix );

	for(int i = 0; i < kvSec.keys.numElem(); i++)
	{
		kvkeybase_t* key = kvSec.keys[i];

		if(!stricmp(key->name, "#include" ))
		{
			AddTokensFile( KV_GetValueString(key) );
			continue;
		}

		// Cannot add same one
		if(_FindToken(key->name ) != NULL)
		{
			MsgWarning("Localization warning (%s): Token '%s' already registered\n", pszFilePrefix, key->name );
			continue;
		}

		AddToken(key->name, KV_GetValueString(key));
	}
}

void CLocalize::AddToken(const char* token, const wchar_t* pszTokenString)
{
	if(token == NULL || pszTokenString == NULL)
		return;

	xstr_loc_convert_special_symbols( (char*)pszTokenString, true );

	CLocToken* pToken = new CLocToken(token, pszTokenString);

	m_tokens.append(pToken);
}

void CLocalize::AddToken(const char* token, const char* pszTokenString)
{
	if(token == NULL || pszTokenString == NULL)
		return;

	xstr_loc_convert_special_symbols( (char*)pszTokenString, true );

	CLocToken* pToken = new CLocToken(token, pszTokenString);

	m_tokens.append(pToken);
}

const wchar_t* CLocalize::GetTokenString(const char* pszToken, const wchar_t* pszDefaultToken) const
{
	ILocToken* foundTok = GetToken(pszToken);

	return foundTok ? foundTok->GetText() : pszDefaultToken;
}

ILocToken* CLocalize::GetToken( const char* pszToken ) const
{
	ILocToken* foundTok = _FindToken(pszToken);

	if(!foundTok)
		DevMsg(DEVMSG_LOCALE, "LOCALIZER: Token %s not found\n", pszToken);

	return foundTok;
}

ILocToken* CLocalize::_FindToken( const char* pszToken ) const
{
	int tokHash = StringToHash(pszToken, true);

	for(int i = 0; i < m_tokens.numElem();i++)
	{
		if(m_tokens[i]->m_tokHash == tokHash)
			return m_tokens[i];
	}

	return NULL;
}