//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include "CmdLineParser.h"
#include "utils/strtools.h"
#include "platform/Platform.h"
#include "ConVar.h"
#include "DebugInterface.h"
#include "ConCommandFactory.h"
#include <ctype.h>

EXPORTED_INTERFACE(ICommandLineParse, CommandLineParse);

CommandLineParse::CommandLineParse()
{
}

CommandLineParse::~CommandLineParse()
{
}

void CommandLineParse::Init(const char* pszCommandLine)
{
	m_args.clear();
	Parse(pszCommandLine);
}

void CommandLineParse::DeInit()
{
	m_args.clear();
}

void CommandLineParse::Parse(const char* pszCommandLine)
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

void CommandLineParse::AddArgument( const char *pFirst, const char *pLast )
{
	if ( pLast == pFirst )	// skip empty strings
		return;

	m_args.append(_Es(pFirst, (pLast - pFirst)));
}

int	CommandLineParse::GetArgumentCount() const
{
	return m_args.numElem();
}

void CommandLineParse::ExecuteCommandLine(unsigned int CmdFilterFlags/* = -1*/) const
{
	if(!m_args.numElem())
		return;

	g_sysConsole->ClearCommandBuffer();

	for (int i = 0; i < GetArgumentCount(); i++ )
	{
		const char* argStr = m_args[i].c_str();

		if (*argStr != '+')
			continue;

		EqString cmdStr(argStr + 1);
		cmdStr.Append(' ');
		cmdStr.Append( GetArgumentsOf(i) );

		g_sysConsole->AppendToCommandBuffer( cmdStr.c_str() );
	}

	g_sysConsole->ExecuteCommandBuffer(CmdFilterFlags);
}

const char* CommandLineParse::GetArgumentString(int index) const
{
	if(!m_args.inRange(index))
		return nullptr;

	return m_args[index].c_str();
}

int CommandLineParse::FindArgument(const char* arg, int startfrom /* = 0 */) const
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

const char* CommandLineParse::GetArgumentsOf(int paramIndex) const
{
	if(paramIndex == -1)
		return nullptr;

	// create an static string
	static EqString _tmpArguments;
	_tmpArguments.Empty();

	for (int i = paramIndex+1; i < m_args.numElem(); i++ )
	{
		const char* argStr = m_args[i].c_str();

		if (*argStr == '+' || *argStr == '-')
			break;

		if(i > paramIndex+1)
			_tmpArguments.Append(' ');

		_tmpArguments.Append('\"');
		_tmpArguments.Append(m_args[i]);
		_tmpArguments.Append('\"');
	}

	return _tmpArguments.c_str();
}
