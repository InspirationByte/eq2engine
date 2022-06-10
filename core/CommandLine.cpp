//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////

#include <ctype.h>
#include <string.h>

#include "ConsoleCommands.h"
#include "utils/strtools.h"
#include "CommandLine.h"

EXPORTED_INTERFACE(ICommandLine, CCommandLine);

CCommandLine::CCommandLine()
{
}

CCommandLine::~CCommandLine()
{
}

void CCommandLine::Init(const char* pszCommandLine)
{
	m_args.clear();
	Parse(pszCommandLine);
}

void CCommandLine::DeInit()
{
	m_args.clear();
}

void CCommandLine::Parse(const char* pszCommandLine)
{
	const char *pChar = pszCommandLine;

	// trim whitespaces
	while ( *pChar && isspace(static_cast<unsigned char>(*pChar)) )
		++pChar;

	bool bInQuotes = false;
	const char *pFirstLetter = NULL;

	for ( ; *pChar; ++pChar )
	{
		if ( bInQuotes )
		{
			if ( *pChar != '\"' )
				continue;

			AddArgument( pFirstLetter, pChar );

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
			AddArgument( pFirstLetter, pChar );
			pFirstLetter = NULL;
		}
	}

	if ( pFirstLetter )
		AddArgument( pFirstLetter, pChar );
}

void CCommandLine::AddArgument( const char *pFirst, const char *pLast )
{
	if ( pLast == pFirst )	// skip empty strings
		return;

	m_args.append(_Es(pFirst, (pLast - pFirst)));
}

int	CCommandLine::GetArgumentCount() const
{
	return m_args.numElem();
}

void CCommandLine::ExecuteCommandLine(cmdLineFilterFn_t filterFn /*= nullptr*/) const
{
	if(!m_args.numElem())
		return;

	g_consoleCommands->ClearCommandBuffer();

	for (int i = 0; i < GetArgumentCount(); i++ )
	{
		const char* cmdOrCvarStr = m_args[i].ToCString();

		if (*cmdOrCvarStr != '+')
			continue;

		const char* argumentValueStr = GetArgumentsOf(i);

		// fill command buffer
		EqString cmdStr(cmdOrCvarStr + 1);
		cmdStr.Append(' ');
		cmdStr.Append(argumentValueStr);

		g_consoleCommands->SetCommandBuffer(cmdStr.ToCString());
		g_consoleCommands->ExecuteCommandBuffer(filterFn);
	}

	//g_consoleCommands->ExecuteCommandBuffer(CmdFilterFlags);
}

const char* CCommandLine::GetArgumentString(int index) const
{
	if(!m_args.inRange(index))
		return nullptr;

	return m_args[index].ToCString();
}

int CCommandLine::FindArgument(const char* arg, int startfrom /* = 0 */) const
{
	if(!m_args.inRange(startfrom))
		return -1;

	for(int i = startfrom; i < m_args.numElem(); i++)
	{
		if(!m_args[i].CompareCaseIns(arg))
			return i;
	}

	return -1;
}

const char* CCommandLine::GetArgumentsOf(int paramIndex) const
{
	if(paramIndex == -1)
		return nullptr;

	// create an static string
	static EqString _tmpArguments;
	_tmpArguments.Empty();

	for (int i = paramIndex+1; i < m_args.numElem(); i++ )
	{
		const char* argStr = m_args[i].ToCString();

		if (*argStr == '+' || *argStr == '-')
			break;

		if(i > paramIndex+1)
			_tmpArguments.Append(' ');

		bool hasSpaces = strchr(argStr, ' ') || strchr(argStr, '\t');
		if (hasSpaces)
			_tmpArguments.Append("\"" + _Es(argStr) + "\"");
		else
			_tmpArguments.Append(argStr);
	}

	return _tmpArguments.ToCString();
}
