//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech command interpreter - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include "CommandAccessor.h"
#include "core_base_header.h"
#include "IConCommandFactory.h"
#include <ctype.h>

#ifdef _WIN32
EXPOSE_SINGLE_INTERFACE_EXPORTS_EX(CommandAccessor,ConCommandAccessor,IConCommandAccessor)
#else
	static ConCommandAccessor g_CommandAccessor;
	IConCommandAccessor *s_pCommandAccessor = ( IConCommandAccessor * )&g_CommandAccessor;
    IEXPORTS IConCommandAccessor *GetCommandAccessor( void ) {return s_pCommandAccessor;}
#endif

extern ConVar *c_SupressAccessorMessages;

ConCommandAccessor::ConCommandAccessor()
{
	//m_pFailedCommands.clear();

	memset(m_pszCommandBuffer, 0, sizeof(m_pszCommandBuffer));
	memset(m_pszLastCommandBuffer, 0, sizeof(m_pszLastCommandBuffer));

	m_nSameCommandsExecuted = 0;
	m_bEnableInitOnlyChange = false;
}

ConCommandAccessor::~ConCommandAccessor()
{
	//m_pFailedCommands.clear();
}

int splitstring_singlecharseparator(char* str, char separator, DkList<EqString> &outStrings)
{
	char c = str[0];

	char* iterator = str;

	char* pFirst = str;
	char* pLast = NULL;

	while(c != 0)
	{
		c = *iterator;

		if(c == separator || c == 0)
		{
			pLast = iterator;

			int char_count = pLast - pFirst;

			if(char_count > 0)
			{
				// add new string
				outStrings.append(_Es(pFirst, char_count));
			}

			pFirst = iterator+1;
		}

		iterator++;
	}

	return outStrings.numElem();
}

// Executes file
void ConCommandAccessor::ParseFileToCommandBuffer(const char* pszFilename, const char* lookupForCommand)
{
	EqString cfgFileName(pszFilename);

	if( cfgFileName.Path_Extract_Ext() != "cfg" && !GetFileSystem()->FileExist(cfgFileName.c_str()) )
	{
		if(!GetFileSystem()->FileExist(("cfg/" + cfgFileName).c_str()))
			cfgFileName = cfgFileName.Path_Strip_Ext() + ".cfg";
		else
			cfgFileName = "cfg/" + cfgFileName;
	}

	if(!GetFileSystem()->FileExist(cfgFileName.c_str()))
		cfgFileName = "cfg/" + cfgFileName;

	bool doCommandLookup = (lookupForCommand != NULL);

	char *buf = GetFileSystem()->GetFileBuffer(cfgFileName.c_str(), NULL, -1, true);

	if(!buf)
	{
		MsgError("Couldn't execute configuraton file '%s'\n",pszFilename);
		return;
	}
	else
	{
		if(!c_SupressAccessorMessages->GetBool())
			MsgInfo("Executing configuraton file '%s'\n",pszFilename);
	}

	if(strlen(buf) < 1)
	{
		PPFree(buf);
		return; //Don't parse me about empty file
	}

	DkList<EqString> cmds;

	//xstrsplit(buf,"\n",cmds);

	if(splitstring_singlecharseparator(buf, '\n', cmds) <= 0)
	{
		PPFree(buf);
		return;
	}

	for(int i = 0;i < cmds.numElem();i++)
	{
		int index;

		EqString tmpStr(cmds[i]);

		index = tmpStr.Find("//");

		if(index != -1)
		{
			//int delStrLen = strlen(cmds[i]);
			tmpStr = tmpStr.Left(index);//,delStrLen - (index+1)); //Clear comments
		}
		else
			tmpStr = xstrtrim(cmds[i].GetData());

		if(tmpStr.GetLength() > 0)
		{
			if(doCommandLookup)
			{
				// Handle 'exec' commands
				if(tmpStr.Find("exec") != -1)
					tmpStr.Append(varargs(" %s",lookupForCommand));

				if(tmpStr.Find(lookupForCommand) == -1)
					continue;

				AppendToCommandBuffer((char*)tmpStr.GetData());
			}
			else
			{
				AppendToCommandBuffer((char*)tmpStr.GetData());
			}
		}
	}

	PPFree(buf);
}

void ConCommandAccessor::EnableInitOnlyVarsChangeProtection(bool bEnable)
{
	m_bEnableInitOnlyChange = bEnable;
}

// Sets command buffer
void ConCommandAccessor::SetCommandBuffer(const char* pszBuffer)
{
	ASSERT(strlen(pszBuffer) < COMMANDBUFFER_SIZE);

	strcpy(m_pszCommandBuffer, pszBuffer);
}

// Appends to command buffer
void ConCommandAccessor::AppendToCommandBuffer(const char* pszBuffer)
{
	int new_len = strlen(pszBuffer) + strlen(m_pszCommandBuffer);

	ASSERT(new_len < COMMANDBUFFER_SIZE);

	strcat(m_pszCommandBuffer, varargs("%s;",pszBuffer));
}

// Clears to command buffer
void ConCommandAccessor::ClearCommandBuffer()
{
	memset(m_pszCommandBuffer, 0, sizeof(m_pszCommandBuffer));
}

void SplitCommandForValidArguments(const char* command, DkList<EqString> *commands)
{
	const char *pChar = command;
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

			int nLen = (int)pChar - (int)pFirstLetter;

			commands->append(_Es(pFirstLetter,nLen));

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
			int nLen = (int)pChar - (int)pFirstLetter;
			commands->append(_Es(pFirstLetter,nLen));
			pFirstLetter = NULL;
		}
	}

	if ( pFirstLetter )
	{
		int nLen = (int)pChar - (int)pFirstLetter;
		commands->append(_Es(pFirstLetter,nLen));
	}
}

// Executes command buffer
bool ConCommandAccessor::ExecuteCommandBuffer(unsigned int CmdFilterFlags/* = -1*/)
{
	if(!stricmp(m_pszLastCommandBuffer, m_pszCommandBuffer))
		m_nSameCommandsExecuted++;

	strcpy(m_pszLastCommandBuffer, m_pszCommandBuffer);

	if(m_nSameCommandsExecuted > MAX_SAME_COMMANDS-1)
	{
		MsgError("Stack buffer overflow prevented! Exiting from cycle!\n");
		ClearCommandBuffer();

		m_nSameCommandsExecuted = 0;

		return false;
	}

	if(strlen(m_pszCommandBuffer) <= 0)
		return false;

	DkList<EqString> szCommands;

	// separate on commands
	if(splitstring_singlecharseparator(m_pszCommandBuffer, ';', szCommands) <= 0)
		return false;

	for(int i = 0; i < szCommands.numElem();i++)
	{
		DkList<EqString> pArgs;

		// split it
		SplitCommandForValidArguments(szCommands[i].GetData(), &pArgs);

		if(pArgs.numElem() == 0)
			continue;

		ConCommandBase *pBase = (ConCommandBase*)GetCvars()->FindBase( pArgs[0].GetData() );

		if(!pBase) //Failed?
		{
			//if(!c_SupressAccessorMessages->GetBool())
				MsgError("Unknown command or variable '%s'\n", pArgs[0].GetData());

			//if(!m_bTrying)
			//	m_pFailedCommands.append(szCommands[i]);

			continue;
		}

		if(CmdFilterFlags != -1)
		{
			if(!(pBase->GetFlags() & CmdFilterFlags))
				continue;
		}

		pArgs.removeIndex(0); // Remove first command

		static ConVar* cheats = (ConVar*)GetCvars()->FindCvar("__cheats");

		bool is_cheat = (pBase->GetFlags() & CV_CHEAT) > 0;

		if(cheats)
		{
			if(is_cheat && !cheats->GetBool())
			{
				MsgWarning("Cannot access to %s command/variable during cheats is off\n",pBase->GetName());
				continue;
			}
		}
		else if(is_cheat)
		{
			Msg("Not in this life\n");

			// TODO: send message to engine to ban the player in next versions of engine

			exit(0);
		}

		if((pBase->GetFlags() & CV_INITONLY) && !m_bEnableInitOnlyChange)
		{
			MsgWarning("Cannot access to %s command/variable from console\n",pBase->GetName());
			continue;
		}

		if(pBase->IsConVar())
		{
			ConVar *pConVar = (ConVar*) pBase;

			// Primitive executor tries to find optional arguments
			if(pArgs.numElem() == 0)
			{
				MsgInfo("%s = %s\n",pConVar->GetName(),pConVar->GetString());
				continue;
			}

			pConVar->SetValue(pArgs[0].GetData());

			if(!c_SupressAccessorMessages->GetBool())
				MsgInfo("%s set to %s\n",pConVar->GetName(), pArgs[0].GetData());
		}
		else
		{
			ConCommand *pConCommand = (ConCommand*)pBase;
			pConCommand->DispatchFunc(&pArgs);
		}
	}

	return true;
}
