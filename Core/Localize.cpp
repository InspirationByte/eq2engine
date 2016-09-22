//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

CLocalize::CLocalize()
{

}

CLocalize::~CLocalize()
{

}

void CLocalize::Init()
{
	kvkeybase_t* pRegional = GetCore()->GetConfig()->FindKeyBase("RegionalSettings", KV_FLAG_SECTION);

	if(!pRegional)
	{
		Msg("Core config missing RegionalSettings section... force English!\n");
		return;
	}

    m_szLanguageName = KV_GetValueString(pRegional->FindKeyBase("DefaultLanguage"), 0, "English" );

    Msg("Language '%s' set\n", m_szLanguageName.c_str());

	int langArg = g_cmdLine->FindArgument("-language");

	if(langArg != -1)
	{
		char* args = g_cmdLine->GetArgumentsOf(langArg);

		if(strlen(args) > 0)
			m_szLanguageName = args;
		else
			MsgError("Error: -language must have argument\n");
	}

	AddTokensFile("core");
}

const char* CLocalize::GetLanguageName()
{
	return m_szLanguageName.GetData();
}

void CLocalize::AddTokensFile(const char* pszFilePrefix)
{
	// TODO: check for already loaded token files!
	char path[2048];
	sprintf(path, "resources/%s_%s.txt", pszFilePrefix, GetLanguageName());

	KeyValues kvs;

	if(!kvs.LoadFromFile(path))
	{
		Msg("Cannot add tokens from file 'resources/%s_%s.txt'\n",pszFilePrefix, GetLanguageName());
		return;
	}

	if(!kvs.GetRootSection()->unicode)
	{
		MsgWarning("Localization warning (%s): file is not unicode\n", pszFilePrefix );
	}

	for(int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
	{
		if(!stricmp( kvs.GetRootSection()->keys[i]->name, "#include" ))
		{
			AddTokensFile( KV_GetValueString(kvs.GetRootSection()->keys[i]) );
			continue;
		}

		// Cannot add same one
		if(GetTokenString( kvs.GetRootSection()->keys[i]->name ) != NULL)
		{
			MsgWarning("Localization warning (%s): Token '%s' already registered\n", pszFilePrefix, kvs.GetRootSection()->keys[i]->name );
			continue;
		}

		AddToken(kvs.GetRootSection()->keys[i]->name, KV_GetValueString(kvs.GetRootSection()->keys[i]));
	}
}

void CLocalize::AddToken(const char* token, const wchar_t* pszTokenString)
{
	if(token == NULL || pszTokenString == NULL)
		return;

	xstr_loc_convert_special_symbols( (char*)pszTokenString, true );

	CLocToken* pToken = new CLocToken(token, pszTokenString);

	m_lTokens.append(pToken);
}

void CLocalize::AddToken(const char* token, const char* pszTokenString)
{
	if(token == NULL || pszTokenString == NULL)
		return;

	xstr_loc_convert_special_symbols( (char*)pszTokenString, true );

	CLocToken* pToken = new CLocToken(token, pszTokenString);

	m_lTokens.append(pToken);
}

const wchar_t* CLocalize::GetTokenString(const char* pszToken,const wchar_t* pszDefaultToken)
{
	int tokHash = StringToHash(pszToken, true);

	for(int i = 0; i < m_lTokens.numElem();i++)
	{
		if(m_lTokens[i]->m_tokHash == tokHash)
			return m_lTokens[i]->GetText();
	}

	DevMsg(DEVMSG_LOCALE, "LOCALIZER: Token %s not found\n", pszToken);

	return pszDefaultToken;
}

ILocToken* CLocalize::GetToken( const char* pszToken )
{
	int tokHash = StringToHash(pszToken, true);

	for(int i = 0; i < m_lTokens.numElem();i++)
	{
		if(m_lTokens[i]->m_tokHash == tokHash)
			return m_lTokens[i];
	}

	DevMsg(DEVMSG_LOCALE, "LOCALIZER: Token %s not found\n", pszToken);

	return NULL;
}
