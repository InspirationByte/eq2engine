//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Variables factory - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "ConCommandFactory.h"
#include "IFileSystem.h"
#include "utils/strtools.h"
#include "ConVar.h"
#include "ConCommand.h"
#include "DebugInterface.h"
#include "IDkCore.h"

EXPORTED_INTERFACE(IConsoleCommands, CConsoleCommands);

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <malloc.h>
#	include <crtdbg.h>
#endif

CConsoleCommands::CConsoleCommands()
{
	m_bCanTryExecute = true;
	m_bTrying = false;

	memset(m_pszCommandBuffer, 0, sizeof(m_pszCommandBuffer));
	memset(m_pszLastCommandBuffer, 0, sizeof(m_pszLastCommandBuffer));

	m_nSameCommandsExecuted = 0;
	m_bEnableInitOnlyChange = false;
}

void MsgFormatCVListText(ConVar *pConVar)
{
	Msg("%s = \"%s\"\n\n",pConVar->GetName(),pConVar->GetString());
	MsgError("   \"%s\"\n",pConVar->GetDesc());
}

void MsgFormatCMDListText(ConCommandBase *pConCommand)
{
	Msg("%s\n",pConCommand->GetName());
	MsgError("   \"%s\"\n",pConCommand->GetDesc());
}

void CC_Cvarlist_f(DkList<EqString> *args)
{
	MsgWarning("********Variable list starts*********\n");

	const ConCommandBase	*pBase;			// Temporary Pointer to cvars

	int iCVCount = 0;
	// Loop through cvars...

	const DkList<ConCommandBase*> *pCommandBases = g_sysConsole->GetAllCommands();

	for ( int i = 0; i < pCommandBases->numElem(); i++ )
	{
		pBase = pCommandBases->ptr()[i];

		if(pBase->IsConVar())
		{
			ConVar *cv = (ConVar*)pBase;

			if(!(cv->GetFlags() & CV_INVISIBLE))
				MsgFormatCVListText(cv);

			iCVCount++;
		}
	}

	MsgWarning(" %i Total Cvars\n",iCVCount);
	MsgWarning("********Variable list ends*********\n");
}

void CC_Cmdlist_f(DkList<EqString> *args)
{
	MsgWarning("********Command list starts*********\n");
	const ConCommandBase	*pBase;			// Temporary Pointer to cvars
	int iCVCount = 0;

	// Loop through cvars...
	const DkList<ConCommandBase*> *pCommandBases = g_sysConsole->GetAllCommands();

	for ( int i = 0; i < pCommandBases->numElem(); i++ )
	{
		pBase = pCommandBases->ptr()[i];

		if(pBase->IsConCommand())
		{
			if(!(pBase->GetFlags() & CV_INVISIBLE))
				MsgFormatCMDListText((ConCommandBase*)pBase);

			iCVCount++;
		}
	}

	MsgWarning(" %i Total Commands\n",iCVCount);
	MsgWarning("********Command list ends*********\n");
}

void CC_Revert_f(DkList<EqString> *args)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: revert <cvar name>\n");
		return;
	}

	ConVar* cv = (ConVar*)g_sysConsole->FindCvar( CMD_ARGV(0).c_str() );
	if(cv)
	{
		cv->SetValue( cv->GetDefaultValue() );
		Msg("%s set to '%s'\n", cv->GetName(), cv->GetString());
	}
	else
		MsgError("No such cvar '%s'\n", CMD_ARGV(0).c_str() );
}

void cc_toggle_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgError("No command and arguments for toggle.\n");
        return;
    }

    ConVar *pConVar = (ConVar*)g_sysConsole->FindCvar(args->ptr()[0].GetData());

    if (!pConVar)
    {
        MsgError("Unknown variable '%s'\n",args->ptr()[0].GetData());
        return;
    }

	bool nValue = pConVar->GetBool();

    if (!(pConVar->GetFlags() & CV_CHEAT) && !(pConVar->GetFlags() & CV_INITONLY))
    {
        pConVar->SetBool(!nValue);
        MsgWarning("%s = %s\n",pConVar->GetName(),pConVar->GetString());
    }
}

void cc_exec_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgWarning("Example: exec <filename> [command lookup] - executes configuration file\n");
        return;
    }

    g_sysConsole->ClearCommandBuffer();

	if(args->numElem() > 1)
		g_sysConsole->ParseFileToCommandBuffer((char*)(args->ptr()[0]).GetData(),args->ptr()[1].GetData());
	else
		g_sysConsole->ParseFileToCommandBuffer((char*)(args->ptr()[0]).GetData());

	((CConsoleCommands*)g_sysConsole.GetInstancePtr())->EnableInitOnlyVarsChangeProtection(false);
	g_sysConsole->ExecuteCommandBuffer();
}

void cc_set_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgError("Usage: set <convar>\n");
        return;
    }

    ConVar *pConVar = (ConVar*)g_sysConsole->FindCvar(args->ptr()[0].GetData());

    if (!pConVar)
    {
        MsgError("Unknown variable '%s'\n",args->ptr()[0].GetData());
        return;
    }

    EqString pArgs;
	char tmp_path[2048];
    for (int i = 1; i < args->numElem();i++)
    {
		sprintf(tmp_path, i < args->numElem()-1 ? (char*) "%s " : (char*) "%s" , args->ptr()[i].GetData());

        pArgs.Append(tmp_path);
    }

    if (!(pConVar->GetFlags() & CV_CHEAT) && !(pConVar->GetFlags() & CV_INITONLY))
    {
        pConVar->SetValue( pArgs.GetData() );
        Msg("%s set to %s\n",pConVar->GetName(), pArgs.GetData());
    }
}

void cc_seta_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgError("Usage: seta <convar>\n");
        return;
    }

    ConVar *pConVar = (ConVar*)g_sysConsole->FindCvar(args->ptr()[0].GetData());

    if (!pConVar)
    {
        MsgError("Unknown variable '%s'\n",args->ptr()[0].GetData());
        return;
    }

    EqString pArgs;
	char tmp_path[2048];
    for (int i = 1; i < args->numElem();i++)
    {
		sprintf(tmp_path, i < args->numElem()-1 ? (char*) "%s " : (char*) "%s" , args->ptr()[i].GetData());

        pArgs.Append(tmp_path);
    }

    pConVar->SetValue( pArgs.GetData() );
}

ConCommand toggle("togglevar",cc_toggle_f,"Toggles ConVar value",CV_UNREGISTERED);
ConCommand exec("exec",cc_exec_f,"Execute configuration file",CV_UNREGISTERED);
ConCommand set("set",cc_set_f,"Sets cvar value",CV_UNREGISTERED);
ConCommand seta("seta",cc_seta_f,"Sets cvar value incl. restricted",CV_UNREGISTERED);
ConCommand cvarlist("cvarlist",CC_Cvarlist_f,"Prints out all aviable cvars",CV_UNREGISTERED);
ConCommand cmdlist("cmdlist",CC_Cmdlist_f,"Prints out all aviable commands",CV_UNREGISTERED);
ConCommand revert("revert",CC_Revert_f,"Reverts cvar to it's default value",CV_UNREGISTERED);

void CConsoleCommands::RegisterCommands()
{
	RegisterCommand(&toggle);
	RegisterCommand(&exec);
	RegisterCommand(&set);
	RegisterCommand(&seta);
	RegisterCommand(&cvarlist);
	RegisterCommand(&cmdlist);
	RegisterCommand(&revert);
}

const ConVar *CConsoleCommands::FindCvar(const char* name)
{
	ConCommandBase const *pBase = FindBase(name);
	if(pBase)
	{
		if(pBase->IsConVar())
			return ( ConVar* )pBase;
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

const ConCommand *CConsoleCommands::FindCommand(const char* name)
{
	ConCommandBase const *pBase = FindBase(name);
	if(pBase)
	{
		if(pBase->IsConCommand())
			return ( ConCommand* )pBase;
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

const ConCommandBase *CConsoleCommands::FindBase(const char* name)
{
	for ( int i = 0; i < m_pCommandBases.numElem(); i++ )
	{
		if ( !stricmp( name, m_pCommandBases[i]->GetName() ) )
			return m_pCommandBases[i];
	}
	return NULL;
}

int alpha_cmd_comp(const void * a, const void * b)
{
	ConCommandBase* pBase1 = *(ConCommandBase**)a;
	ConCommandBase* pBase2 = *(ConCommandBase**)b;

	return stricmp(pBase1->GetName(),pBase2->GetName());//( *(int*)a - *(int*)b );
}

static void _RegisterOrDie()
{
	if(GetCore()->GetInterface(CONSOLE_INTERFACE_VERSION) == NULL)
		GetCore()->RegisterInterface( CONSOLE_INTERFACE_VERSION, GetCConsoleCommands());
}

bool isCvarChar(char c)
{
	return c == '+' ||  c == '-' || c == '_' || isalpha(c);
}

void CConsoleCommands::RegisterCommand(ConCommandBase *pCmd)
{
	_RegisterOrDie();

	ASSERT(pCmd != NULL);
	ASSERTMSG(FindBase(pCmd->GetName()) == NULL, varargs("ConCmd/CVar %s already registered", pCmd->GetName()));

	//if(FindBase(pCmd->GetName()) != NULL)
	//	MsgError("%s already declared.\n",pCmd->GetName());

	ASSERTMSG(isCvarChar(*pCmd->GetName()), "RegisterCommand - command name has invalid start character!");

	// Already registered
	if ( pCmd->IsRegistered() )
		return;

	m_bCanTryExecute = true;

	m_pCommandBases.append( pCmd );

	pCmd->m_bIsRegistered = true;

	// alphabetic sort
	qsort(m_pCommandBases.ptr(), m_pCommandBases.numElem(), sizeof(ConCommandBase*), alpha_cmd_comp);
}

void CConsoleCommands::UnregisterCommand(ConCommandBase *pCmd)
{
	if ( !pCmd->IsRegistered() )
		return;

	for ( int i = 0; i < m_pCommandBases.numElem(); i++ )
	{
		if ( !stricmp( pCmd->GetName(), m_pCommandBases[i]->GetName() ) )
		{
			m_pCommandBases.removeIndex(i);
			return;
		}
	}
}

void CConsoleCommands::DeInit()
{
	m_pFailedCommands.clear();

	for(int i = 0; i < m_pCommandBases.numElem(); i++)
		((ConCommandBase*)m_pCommandBases[i])->m_bIsRegistered = false;

	m_pCommandBases.clear();
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
void CConsoleCommands::ParseFileToCommandBuffer(const char* pszFilename, const char* lookupForCommand)
{
	EqString cfgFileName(pszFilename);

	if( cfgFileName.Path_Extract_Ext() != "cfg" && !g_fileSystem->FileExist(cfgFileName.c_str()) )
	{
		if(!g_fileSystem->FileExist(("cfg/" + cfgFileName).c_str()))
			cfgFileName = cfgFileName.Path_Strip_Ext() + ".cfg";
		else
			cfgFileName = "cfg/" + cfgFileName;
	}

	if(!g_fileSystem->FileExist(cfgFileName.c_str()))
		cfgFileName = "cfg/" + cfgFileName;

	bool doCommandLookup = (lookupForCommand != NULL);

	char *buf = g_fileSystem->GetFileBuffer(cfgFileName.c_str(), NULL, -1, true);

	if(!buf)
	{
		MsgError("Couldn't execute configuraton file '%s'\n",pszFilename);
		return;
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

		if(tmpStr.Length() > 0)
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

void CConsoleCommands::EnableInitOnlyVarsChangeProtection(bool bEnable)
{
	m_bEnableInitOnlyChange = bEnable;
}

// Sets command buffer
void CConsoleCommands::SetCommandBuffer(const char* pszBuffer)
{
	ASSERT(strlen(pszBuffer) < COMMANDBUFFER_SIZE);

	strcpy(m_pszCommandBuffer, pszBuffer);
}

// Appends to command buffer
void CConsoleCommands::AppendToCommandBuffer(const char* pszBuffer)
{
	int new_len = strlen(pszBuffer) + strlen(m_pszCommandBuffer);

	ASSERT(new_len < COMMANDBUFFER_SIZE);

	strcat(m_pszCommandBuffer, varargs("%s;",pszBuffer));
}

// Clears to command buffer
void CConsoleCommands::ClearCommandBuffer()
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

			int nLen = (int)(pChar - pFirstLetter);

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
			int nLen = (int)(pChar - pFirstLetter);
			commands->append(_Es(pFirstLetter,nLen));
			pFirstLetter = NULL;
		}
	}

	if ( pFirstLetter )
	{
		int nLen = (int)(pChar - pFirstLetter);
		commands->append(_Es(pFirstLetter,nLen));
	}
}

// Executes command buffer
bool CConsoleCommands::ExecuteCommandBuffer(unsigned int CmdFilterFlags/* = -1*/)
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

		ConCommandBase *pBase = (ConCommandBase*)FindBase( pArgs[0].GetData() );

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

		static ConVar* cheats = (ConVar*)FindCvar("__cheats");

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
				MsgInfo("%s is '%s' (default value is '%s')\n",pConVar->GetName(),pConVar->GetString(), pConVar->GetDefaultValue());
				continue;
			}

			pConVar->SetValue(pArgs[0].GetData());
			Msg("%s set to '%s'\n", pConVar->GetName(), pConVar->GetString());
		}
		else
		{
			ConCommand *pConCommand = (ConCommand*)pBase;
			pConCommand->DispatchFunc(&pArgs);
		}
	}

	return true;
}
