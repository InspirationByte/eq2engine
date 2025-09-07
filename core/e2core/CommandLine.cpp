//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "CommandLine.h"
#include "ConsoleCommands.h"

EXPORTED_INTERFACE(ICommandLine, CCommandLine);

CCommandLine::CCommandLine()
{
	g_eqCore->RegisterInterface(this);
}

void CCommandLine::Init(ArrayCRef<const char*> args)
{
	m_args.clear();
	if (args.numElem() == 1)
	{
		if(args.front())
			Parse(args.front());
	}
	else
	{
		m_args.reserve(args.numElem());
		for (int i = 0; i < args.numElem(); ++i)
			m_args.append(args[i]);
	}
}

void CCommandLine::DeInit()
{
	m_args.clear(true);
}

void CCommandLine::Parse(const char* pszCommandLine)
{
	const char *pChar = pszCommandLine;

	// trim whitespaces
	while ( *pChar && CType::IsSpace(*pChar) )
		++pChar;

	bool bInQuotes = false;
	const char *pFirstLetter = nullptr;

	for ( ; *pChar; ++pChar )
	{
		if ( bInQuotes )
		{
			if ( *pChar != '\"' )
				continue;

			AddArgument( pFirstLetter, pChar );

			pFirstLetter = nullptr;
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

			if (CType::IsSpace(*pChar) )
				continue;

			pFirstLetter = pChar;
			continue;
		}

		// Here, we're in the middle of a word. Look for the end of it.
		if (CType::IsSpace( *pChar ) )
		{
			AddArgument( pFirstLetter, pChar );
			pFirstLetter = nullptr;
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

void CCommandLine::ExecuteCommandLine(CommandFilterFn filterFn /*= nullptr*/) const
{
	if(!m_args.numElem())
		return;

	g_consoleCommands->ClearCommandBuffer();

	for (int i = 0; i < m_args.numElem(); i++)
	{
		const char* cmdOrCvarStr = m_args[i];
		if (*cmdOrCvarStr != '+')
			continue;

		g_consoleCommands->AppendToCommandBuffer(EqString::Format("%s %s;", cmdOrCvarStr + 1, GetArgumentsOf(i)));
	}
	g_consoleCommands->ExecuteCommandBuffer(filterFn);
}

int CCommandLine::Find(const char* arg, int startfrom /* = 0 */) const
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

	for (int i = paramIndex + 1; i < m_args.numElem(); i++)
	{
		const char* argStr = m_args[i];
		if (*argStr == '+' || *argStr == '-')
			break;

		if (i > paramIndex + 1)
			_tmpArguments.Append(' ');

		bool hasSpaces = strchr(argStr, ' ') || strchr(argStr, '\t');
		if (hasSpaces)
			_tmpArguments.Append("\"" + _Es(argStr) + "\"");
		else
			_tmpArguments.Append(argStr);
	}

	return _tmpArguments;
}


int	CCommandLine::GetArgumentsOf(int paramIndex, const char** values, int maxValues) const
{
	if (paramIndex == -1)
		return 0;

	int numArgs = 0;
	for (int i = paramIndex + 1; i < m_args.numElem() && maxValues > 0; ++i, --maxValues)
	{
		const char* argStr = m_args[i];
		if (*argStr == '+' || *argStr == '-')
			break;

		values[numArgs++] = argStr;
	}
	return numArgs;
}