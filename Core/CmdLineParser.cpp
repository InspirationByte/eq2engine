//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include "CmdLineParser.h"
#include "utils/strtools.h"
#include "Platform.h"
#include "ConVar.h"
#include "DebugInterface.h"
#include "ConCommandFactory.h"
#include <ctype.h>

EXPORTED_INTERFACE(ICommandLineParse, CommandLineParse);

CommandLineParse::CommandLineParse()
{
	m_szFullArgString = " ";
	m_iArgsCount = 0;
}

CommandLineParse::~CommandLineParse()
{
	m_iArgsCount = 0;
	m_szParams.clear();
}

void CommandLineParse::Init(const char* pszCommandLine)
{
	if(m_szFullArgString.GetLength())
		m_szFullArgString.Empty();

	m_iArgsCount = 0;

	m_szFullArgString = pszCommandLine;

	Parse();
}

void CommandLineParse::DeInit()
{

}

void CommandLineParse::Parse()
{
	const char *pChar = m_szFullArgString.GetData();
	while ( *pChar && isspace(static_cast<unsigned char>(*pChar)) )
	{
		++pChar;
	}

	bool bInQuotes = false;
	const char *pFirstLetter = NULL;
	for ( ; *pChar; ++pChar )
	{
		if ( bInQuotes )
		{
			if ( *pChar != '\"' )
				continue;

			Parse_AddParameter( pFirstLetter, pChar );

			pFirstLetter = NULL;
			bInQuotes = false;
			continue;
		}

		// Haven't started a word yet...
		if ( !pFirstLetter )
		{
			if ( *pChar == '\"' )
			{
				bInQuotes = true;
				pFirstLetter = pChar + 1;
				continue;
			}

			if ( isspace( static_cast<unsigned char>(*pChar) ) )
				continue;

			pFirstLetter = pChar;
			continue;
		}

		// Here, we're in the middle of a word. Look for the end of it.
		if ( isspace( *pChar ) )
		{
			Parse_AddParameter( pFirstLetter, pChar );
			pFirstLetter = NULL;
		}
	}

	if ( pFirstLetter )
	{
		Parse_AddParameter( pFirstLetter, pChar );
	}
}

void CommandLineParse::Parse_AddParameter( const char *pFirst, const char *pLast )
{
	if ( pLast == pFirst )
		return;

	ASSERT( m_iArgsCount < 256 );
	int nLen = (int)pLast - (int)pFirst;

	m_szParams.append(EqString(pFirst, nLen));

	//Msg("Added %s\n", EqString(pFirst, nLen).GetData());

	++m_iArgsCount;
}

void CommandLineParse::ExecuteCommandLine(bool cvars,bool commands, unsigned int CmdFilterFlags/* = -1*/)
{
	if(m_iArgsCount < 1)
		return;

	unsigned int filterFlags;

	if(CmdFilterFlags == -1)
	{
		if(cvars)
			filterFlags |= CMDBASE_CONVAR;
		if(commands)
			filterFlags |= CMDBASE_CONCOMMAND;
	}
	else
	{
		filterFlags = CmdFilterFlags;
	}

	g_sysConsole->ClearCommandBuffer();

	char tmp_path[2048];

	for (int i = 0; i < GetArgumentCount();i++ )
	{
		if(!IsArgument( (char*)m_szParams[i].GetData(), true))
		{
			EqString tempStr = m_szParams[i].Mid(1,m_szParams[i].GetLength() - 1);
			EqString tmpArgs( GetArgumentsOf(i) );

			sprintf(tmp_path, "%s %s",tempStr.GetData(), tmpArgs.GetData());

			g_sysConsole->AppendToCommandBuffer(tmp_path);
		}
	}

	((CConsoleCommands*)g_sysConsole.GetInstancePtr())->EnableInitOnlyVarsChangeProtection(true);
	g_sysConsole->ExecuteCommandBuffer(filterFlags);
	((CConsoleCommands*)g_sysConsole.GetInstancePtr())->EnableInitOnlyVarsChangeProtection(false);
}

char* CommandLineParse::GetArgumentString(int index)
{
	if(index > GetArgumentCount() || index < 0)
		return NULL;

	return (char*)m_szParams[index].GetData();
}

int CommandLineParse::FindArgument(const char* arg,int startfrom /* = 0 */)
{
	if(startfrom > GetArgumentCount()-1)
		return -1;

	for(int i = startfrom;i < GetArgumentCount();i++)
	{
		if(!m_szParams[i].CompareCaseIns(arg))
			return i;
	}
	return -1;
}

bool CommandLineParse::IsArgument(char* pszString, bool skipMinus)
{
	int uind = -1;

	char* pStrChar = strchr(pszString, '+');

	if(pStrChar == pszString)
		return false;

	if(!skipMinus)
	{
		char* pStrChar = strchr(pszString, '-');

		if(pStrChar == pszString)
			return false;
	}

	// Not found any parameter-specific words? It's argument!
	return true;
}

char* CommandLineParse::GetArgumentsOf(int paramIndex)
{
	if(paramIndex == -1)
		return NULL;

	// create an static string
	static EqString tmpArguments;
	tmpArguments.Empty();

	char tmp_path[2048];

	int nLastArg = m_iArgsCount;

	// get the last argument
	for (int i = paramIndex+1; i < m_iArgsCount; i++ )
	{
		// if that's not argument, stop
		if(!IsArgument( (char*)m_szParams[i].GetData() ))
			break;

		nLastArg = i+1;
	}

	for (int i = paramIndex+1; i < nLastArg; i++ )
	{
		bool isLast = (i+1 == nLastArg);

		if(!isLast)
		{
			sprintf(tmp_path, "%s ",m_szParams[i].GetData());
			tmpArguments.Append(tmp_path);
		}
		else
			tmpArguments.Append( m_szParams[i] );
	}

	return (char*)tmpArguments.GetData();
}
