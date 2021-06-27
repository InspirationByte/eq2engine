//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Variables factory
//////////////////////////////////////////////////////////////////////////////////

#include "ConsoleCommands.h"

#include <stdio.h>
#include <stdlib.h>

#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/DebugInterface.h"
#include "core/IDkCore.h"

#include "utils/strtools.h"

EXPORTED_INTERFACE(IConsoleCommands, CConsoleCommands);

#define CON_SEPARATOR ';'

bool IsAllowedToExecute(ConCommandBase* base)
{
	if (base->GetFlags() & CV_INITONLY)
		return false;

	static ConVar* cheats = (ConVar*)g_consoleCommands->FindCvar("__cheats");

	bool cheatsEnabled = cheats ? cheats->GetBool() : false;

	if (base->GetFlags() & CV_CHEAT)
		return cheatsEnabled;

	return true;
}

void PrintConVar(ConVar *pConVar)
{
	Msg("%s = \"%s\"\n\n", pConVar->GetName(), pConVar->GetString());
	MsgError("   \"%s\"\n", pConVar->GetDesc());
}

void PrintConCommand(ConCommandBase *pConCommand)
{
	Msg("%s\n", pConCommand->GetName());
	MsgError("   \"%s\"\n", pConCommand->GetDesc());
}

int alpha_cmd_comp(const void * a, const void * b)
{
	ConCommandBase* pBase1 = *(ConCommandBase**)a;
	ConCommandBase* pBase2 = *(ConCommandBase**)b;

	return stricmp(pBase1->GetName(), pBase2->GetName());
}

bool isCvarChar(char c)
{
	return c == '+' || c == '-' || c == '_' || isalpha(c);
}

DECLARE_CONCOMMAND_FN(cvarlist)
{
	MsgWarning("********Variable list starts*********\n");

	const ConCommandBase	*pBase;			// Temporary Pointer to cvars

	int iCVCount = 0;
	// Loop through cvars...

	const DkList<ConCommandBase*> *pCommandBases = g_consoleCommands->GetAllCommands();

	for (int i = 0; i < pCommandBases->numElem(); i++)
	{
		pBase = pCommandBases->ptr()[i];

		if (pBase->IsConVar())
		{
			ConVar *cv = (ConVar*)pBase;

			if (!(cv->GetFlags() & CV_INVISIBLE))
				PrintConVar(cv);

			iCVCount++;
		}
	}

	MsgWarning(" %i Total Cvars\n", iCVCount);
	MsgWarning("********Variable list ends*********\n");
}

DECLARE_CONCOMMAND_FN(cmdlist)
{
	MsgWarning("********Command list starts*********\n");
	const ConCommandBase	*pBase;			// Temporary Pointer to cvars
	int iCVCount = 0;

	// Loop through cvars...
	const DkList<ConCommandBase*>* pCommandBases = g_consoleCommands->GetAllCommands();

	for (int i = 0; i < pCommandBases->numElem(); i++)
	{
		pBase = pCommandBases->ptr()[i];

		if (pBase->IsConCommand())
		{
			if (!(pBase->GetFlags() & CV_INVISIBLE))
				PrintConCommand((ConCommandBase*)pBase);

			iCVCount++;
		}
	}

	MsgWarning(" %i Total Commands\n", iCVCount);
	MsgWarning("********Command list ends*********\n");
}

void cvar_list_collect(const ConCommandBase* cmd, DkList<EqString>& list, const char* query)
{
	const ConCommandBase* pBase;
	const DkList<ConCommandBase*>* pCommandBases = g_consoleCommands->GetAllCommands();

	const int LIST_LIMIT = 50;

	for (int i = 0; i < pCommandBases->numElem(); i++)
	{
		if (list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		pBase = pCommandBases->ptr()[i];

		if (!pBase->IsConVar())
			continue;

		if (pBase->GetFlags() & CV_INVISIBLE)
			continue;

		if (*query == 0 || xstristr(pBase->GetName(), query))
			list.append(pBase->GetName());
	}
}

DECLARE_CONCOMMAND_FN(revertcvar)
{
	if (CMD_ARGC == 0)
	{
		Msg("Usage: revert <cvar name>\n");
		return;
	}

	ConVar* cv = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0).ToCString());
	if (cv)
	{
		cv->SetValue(cv->GetDefaultValue());
		Msg("'%s' set to '%s'\n", cv->GetName(), cv->GetString());
	}
	else
		MsgError("No such cvar '%s'\n", CMD_ARGV(0).ToCString());
}

DECLARE_CONCOMMAND_FN(togglecvar)
{
	if (CMD_ARGC == 0)
	{
		MsgError("No command and arguments for toggle.\n");
		return;
	}

	ConVar *pConVar = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0).ToCString());

	if (!pConVar)
	{
		MsgError("Unknown variable '%s'\n", CMD_ARGV(0).ToCString());
		return;
	}

	if (IsAllowedToExecute(pConVar))
	{
		bool nValue = pConVar->GetBool();

		pConVar->SetBool(!nValue);
		MsgWarning("%s = %s\n", pConVar->GetName(), pConVar->GetString());
	}
}

DECLARE_CONCOMMAND_FN(exec)
{
	if (CMD_ARGC == 0)
	{
		MsgWarning("Example: exec <filename> [command lookup] - executes configuration file\n");
		return;
	}

	g_consoleCommands->ParseFileToCommandBuffer(CMD_ARGV(0).ToCString());
}

DECLARE_CONCOMMAND_FN(set)
{
	if (CMD_ARGC == 0)
	{
		MsgError("Usage: set <convar>\n");
		return;
	}

	ConVar *pConVar = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0).ToCString());

	if (!pConVar)
	{
		MsgError("Unknown variable '%s'\n", CMD_ARGV(0).ToCString());
		return;
	}

	EqString joinArgs;

	for (int i = 1; i < CMD_ARGC; i++)
		joinArgs.Append(varargs(i < CMD_ARGC - 1 ? (char*) "%s " : (char*) "%s", CMD_ARGV(i).ToCString()));

	if (IsAllowedToExecute(pConVar))
	{
		pConVar->SetValue(joinArgs.GetData());
		Msg("'%s' set to '%s'\n", pConVar->GetName(), pConVar->GetString());
	}
}

DECLARE_CONCOMMAND_FN(seti)
{
	if (CMD_ARGC == 0)
	{
		MsgError("Usage: seti <convar>\n");
		return;
	}

	ConVar *pConVar = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0).ToCString());

	if (!pConVar)
	{
		MsgError("Unknown variable '%s'\n", CMD_ARGV(0).ToCString());
		return;
	}

	EqString joinArgs;

	for (int i = 1; i < CMD_ARGC; i++)
		joinArgs.Append(varargs(i < CMD_ARGC - 1 ? (char*) "%s " : (char*) "%s", CMD_ARGV(i).ToCString()));

	pConVar->SetValue(joinArgs.GetData());
}

void fncfgfiles_variants(const ConCommandBase* cmd, DkList<EqString>& list, const char* query)
{
	DKFINDDATA* findData = nullptr;
	char* fileName = (char*)g_fileSystem->FindFirst("cfg/*.cfg", &findData, SP_MOD);

	if (fileName)
	{
		list.append(_Es(fileName).Path_Strip_Ext());

		while (fileName = (char*)g_fileSystem->FindNext(findData))
		{
			if (!g_fileSystem->FindIsDirectory(findData))
				list.append(_Es(fileName).Path_Strip_Ext());
		}

		g_fileSystem->FindClose(findData);
	}
}

ConCommand cvarlist("cvarlist", CONCOMMAND_FN(cvarlist), "Prints out all aviable cvars", CV_UNREGISTERED);
ConCommand cmdlist("cmdlist", CONCOMMAND_FN(cmdlist), "Prints out all aviable commands", CV_UNREGISTERED);

ConCommand exec("exec", CONCOMMAND_FN(exec), fncfgfiles_variants, "Execute configuration file", CV_UNREGISTERED);

ConCommand toggle("togglevar", CONCOMMAND_FN(togglecvar), cvar_list_collect, "Toggles ConVar value", CV_UNREGISTERED);
ConCommand set("set", CONCOMMAND_FN(set), cvar_list_collect, "Sets cvar value", CV_UNREGISTERED);
ConCommand seta("seti", CONCOMMAND_FN(seti), cvar_list_collect, "Sets cvar value including restricted ones", CV_UNREGISTERED | CV_INVISIBLE);
ConCommand revert("revert", CONCOMMAND_FN(revertcvar), cvar_list_collect, "Reverts cvar to it's default value", CV_UNREGISTERED);


void SplitCommandForValidArguments(const char* command, DkList<EqString>& commands)
{
	const char *pChar = command;
	while (*pChar && isspace(static_cast<unsigned char>(*pChar)))
	{
		++pChar;
	}

	bool bInQuotes = false;
	const char *pFirstLetter = NULL;
	for (; *pChar; ++pChar)
	{
		if (bInQuotes)
		{
			if (*pChar != '\"')
				continue;

			int nLen = (int)(pChar - pFirstLetter);

			commands.append(_Es(pFirstLetter, nLen));

			pFirstLetter = NULL;
			bInQuotes = false;
			continue;
		}

		// Haven't started a word yet...
		if (!pFirstLetter)
		{
			if (*pChar == '\"')
			{
				bInQuotes = true;
				pFirstLetter = pChar + 1;
				continue;
			}

			if (isspace(static_cast<unsigned char>(*pChar)))
				continue;

			pFirstLetter = pChar;
			continue;
		}

		// Here, we're in the middle of a word. Look for the end of it.
		if (isspace(*pChar))
		{
			int nLen = (int)(pChar - pFirstLetter);
			commands.append(_Es(pFirstLetter, nLen));
			pFirstLetter = NULL;
		}
	}

	if (pFirstLetter)
	{
		int nLen = (int)(pChar - pFirstLetter);
		commands.append(_Es(pFirstLetter, nLen));
	}
}


CConsoleCommands::CConsoleCommands()
{
	memset(m_currentCommands, 0, sizeof(m_currentCommands));
	memset(m_lastExecutedCommands, 0, sizeof(m_lastExecutedCommands));

	m_sameCommandsExecuted = 0;
	m_commandListDirty = false;
}

void CConsoleCommands::RegisterCommands()
{
	RegisterCommand(&cvarlist);
	RegisterCommand(&cmdlist);
	RegisterCommand(&exec);

	RegisterCommand(&toggle);
	RegisterCommand(&set);
	RegisterCommand(&seta);
	RegisterCommand(&revert);
}

const ConVar* CConsoleCommands::FindCvar(const char* name)
{
	ConCommandBase const *pBase = FindBase(name);

	if(pBase && pBase->IsConVar())
		return ( ConVar* )pBase;

	return nullptr;
}

const ConCommand* CConsoleCommands::FindCommand(const char* name)
{
	ConCommandBase const *pBase = FindBase(name);

	if(pBase && pBase->IsConCommand())
		return ( ConCommand* )pBase;

	return nullptr;
}

const ConCommandBase* CConsoleCommands::FindBase(const char* name)
{
	SortCommands();

	for ( int i = 0; i < m_registeredCommands.numElem(); i++ )
	{
		if ( !stricmp( name, m_registeredCommands[i]->GetName() ) )
			return m_registeredCommands[i];
	}

	return nullptr;
}

static void _RegisterOrDie()
{
	if(GetCore()->GetInterface(CONSOLE_INTERFACE_VERSION) == NULL)
		GetCore()->RegisterInterface( CONSOLE_INTERFACE_VERSION, GetCConsoleCommands());
}

void CConsoleCommands::RegisterCommand(ConCommandBase *pCmd)
{
	_RegisterOrDie();

	ASSERT(pCmd != NULL);
	ASSERTMSG(FindBase(pCmd->GetName()) == NULL, varargs("ConCmd/CVar %s already registered", pCmd->GetName()));

	ASSERTMSG(isCvarChar(*pCmd->GetName()), "RegisterCommand - command name has invalid start character!");

	// Already registered
	if ( pCmd->IsRegistered() )
		return;

	m_registeredCommands.append( pCmd );

	pCmd->m_bIsRegistered = true;

	// alphabetic sort
	m_commandListDirty = true;
}

void CConsoleCommands::UnregisterCommand(ConCommandBase *pCmd)
{
	if ( !pCmd->IsRegistered() )
		return;

	m_registeredCommands.remove(pCmd);
}

void CConsoleCommands::DeInit()
{
	for(int i = 0; i < m_registeredCommands.numElem(); i++)
		((ConCommandBase*)m_registeredCommands[i])->m_bIsRegistered = false;

	m_registeredCommands.clear();
}

void CConsoleCommands::SortCommands()
{
	if (!m_commandListDirty)
		return;

	qsort(m_registeredCommands.ptr(), m_registeredCommands.numElem(), sizeof(ConCommandBase*), alpha_cmd_comp);

	m_commandListDirty = false;
}

void CConsoleCommands::ForEachSeparated(char* str, char separator, FUNC fn, void* extra)
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
				(this->*fn)(pFirst, char_count, extra);

			pFirst = iterator+1;
		}

		iterator++;
	}
}

void CConsoleCommands::ParseAndAppend(char* str, int len, void* extra)
{
	EqString tmpStr(str, len);
	int commentIdx = tmpStr.Find("//");

	if(commentIdx != -1)
		tmpStr = tmpStr.Left(commentIdx);

	tmpStr = tmpStr.TrimSpaces();

	if (tmpStr.Length() > 0) 
	{
		EqString cmdStr(tmpStr.TrimSpaces());

		DkList<EqString> cmdArgs;
		SplitCommandForValidArguments(cmdStr.ToCString(), cmdArgs);

		// executing file must be put to the command buffer in proper order
		if (cmdArgs.numElem() && !cmdArgs[0].CompareCaseIns("exec"))
		{
			cmdArgs.removeIndex(0);
			CC_exec_f(nullptr, cmdArgs);
			return;
		}

		strcat(m_currentCommands, varargs("%s;", cmdStr.ToCString()));
	}
		
}

// Executes file
void CConsoleCommands::ParseFileToCommandBuffer(const char* pszFilename)
{
	EqString cfgFileName(pszFilename);

	if( cfgFileName.Path_Extract_Ext() != "cfg" && !g_fileSystem->FileExist(cfgFileName.ToCString()) )
	{
		if(!g_fileSystem->FileExist(("cfg/" + cfgFileName).ToCString()))
			cfgFileName = cfgFileName.Path_Strip_Ext() + ".cfg";
		else
			cfgFileName = "cfg/" + cfgFileName;
	}

	if(!g_fileSystem->FileExist(cfgFileName.ToCString()))
		cfgFileName = "cfg/" + cfgFileName;

	char *buf = g_fileSystem->GetFileBuffer(cfgFileName.ToCString(), NULL, -1);

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

	ForEachSeparated(buf, '\n', &CConsoleCommands::ParseAndAppend, NULL);
	PPFree(buf);
}

// Sets command buffer
void CConsoleCommands::SetCommandBuffer(const char* pszBuffer)
{
	ASSERT(strlen(pszBuffer) < COMMANDBUFFER_SIZE);
	ClearCommandBuffer();

	ForEachSeparated((char*)pszBuffer, CON_SEPARATOR, &CConsoleCommands::ParseAndAppend, NULL);
}

// Appends to command buffer
void CConsoleCommands::AppendToCommandBuffer(const char* pszBuffer)
{
	int new_len = strlen(pszBuffer) + strlen(m_currentCommands);

	ASSERT(new_len < COMMANDBUFFER_SIZE);
	ForEachSeparated((char*)pszBuffer, CON_SEPARATOR, &CConsoleCommands::ParseAndAppend, NULL);

	//strcat(m_currentCommands, varargs("%s;",pszBuffer));
}

// Clears to command buffer
void CConsoleCommands::ClearCommandBuffer()
{
	m_sameCommandsExecuted = 0;
	memset(m_currentCommands, 0, sizeof(m_currentCommands));
}

void CConsoleCommands::ResetCounter()
{
	m_sameCommandsExecuted = 0; 
}

struct execOptions_t
{
	cmdFilterFn_t filterFn;
	bool quiet;
};

void CConsoleCommands::SplitOnArgsAndExec(char* str, int len, void* extra)
{
	execOptions_t* options = (execOptions_t*)extra;

	EqString commandStr(str,len);

	DkList<EqString> cmdArgs;

	// split it
	SplitCommandForValidArguments(commandStr.ToCString(), cmdArgs);

	if(cmdArgs.numElem() == 0)
		return;

	ConCommandBase *pBase = (ConCommandBase*)FindBase( cmdArgs[0].GetData() );

	if(!pBase) //Failed?
	{
		if(!options->quiet)
			MsgError("Unknown command or variable '%s'\n", cmdArgs[0].GetData());

		m_failedCommands.append(commandStr);
		return;
	}

	if(options->filterFn)
	{
		if(!(options->filterFn)(pBase, cmdArgs))
			return;
	}

	// remove cmd name
	cmdArgs.removeIndex(0);

	if (!IsAllowedToExecute(pBase))
	{
		MsgWarning("Cannot access '%s' command/variable\n", pBase->GetName());
		return;
	}

	if(pBase->IsConVar())
	{
		ConVar* pConVar = (ConVar*)pBase;

		// Primitive executor tries to find optional arguments
		if(cmdArgs.numElem() == 0)
		{
			MsgInfo("%s is '%s' (default value is '%s')\n",pConVar->GetName(),pConVar->GetString(), pConVar->GetDefaultValue());
			return;
		}

		pConVar->SetValue(cmdArgs[0].GetData());
		Msg("%s set to '%s'\n", pConVar->GetName(), pConVar->GetString());
	}
	else
	{
		ConCommand* pConCommand = (ConCommand*)pBase;
		pConCommand->DispatchFunc(cmdArgs);
	}
}

// Executes command buffer
bool CConsoleCommands::ExecuteCommandBuffer(cmdFilterFn_t filterFn /*= nullptr*/, bool quiet /*= false*/)
{
	SortCommands();

	m_failedCommands.clear();

	if(!stricmp(m_lastExecutedCommands, m_currentCommands))
		m_sameCommandsExecuted++;

	strcpy(m_lastExecutedCommands, m_currentCommands);

	if(m_sameCommandsExecuted > MAX_SAME_COMMANDS-1)
	{
		MsgError("Stack buffer overflow prevented! Exiting from cycle!\n");
		ClearCommandBuffer();
		return false;
	}

	if(strlen(m_currentCommands) <= 0)
		return false;

	execOptions_t options;
	options.filterFn = filterFn;
	options.quiet = quiet;

	ForEachSeparated(m_currentCommands, CON_SEPARATOR, &CConsoleCommands::SplitOnArgsAndExec,  &options);

	return true;
}

// returns failed commands
DkList<EqString>& CConsoleCommands::GetFailedCommands()
{
	return m_failedCommands;
}