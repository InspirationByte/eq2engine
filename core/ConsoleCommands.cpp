//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Variables factory
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"
#include "core/IDkCore.h"
#include "ConsoleCommands.h"

EXPORTED_INTERFACE(IConsoleCommands, CConsoleCommands);

constexpr const char* CON_SEPARATORS = ";\n";

static bool IsAllowedToExecute(ConCommandBase* base)
{
	if (base->GetFlags() & CV_PROTECTED)
		return false;

	static ConVar* cheats = (ConVar*)g_consoleCommands->FindCvar("__cheats");

	const bool cheatsEnabled = cheats ? cheats->GetBool() : false;
	if (base->GetFlags() & CV_CHEAT)
		return cheatsEnabled;

	return true;
}

static void PrintConVar(ConVar* pConVar)
{
	Msg("%s = \"%s\"\n\n", pConVar->GetName(), pConVar->GetString());
	MsgError("   \"%s\"\n", pConVar->GetDesc());
}

static void PrintConCommand(ConCommandBase* pConCommand)
{
	Msg("%s\n", pConCommand->GetName());
	MsgError("   \"%s\"\n", pConCommand->GetDesc());
}

static int alpha_cmd_comp(const ConCommandBase* a, const ConCommandBase* b)
{
	return stricmp(a->GetName(), b->GetName());
}

static bool isCvarChar(char c)
{
	return c == '+' || c == '-' || c == '_' || CType::IsAlphabetic(c);
}

DECLARE_CONCOMMAND_FN(cvarlist)
{
	MsgWarning("********Variable list starts*********\n");

	const ConCommandBase* pBase;			// Temporary Pointer to cvars

	int iCVCount = 0;
	// Loop through cvars...

	const ConCommandListRef cmdList = g_consoleCommands->GetAllCommands();

	for (int i = 0; i < cmdList.numElem(); i++)
	{
		pBase = cmdList[i];

		if (pBase->IsConVar())
		{
			ConVar* cv = (ConVar*)pBase;

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
	const ConCommandBase* pBase;			// Temporary Pointer to cvars
	int iCVCount = 0;

	// Loop through cvars...
	const ConCommandListRef cmdList = g_consoleCommands->GetAllCommands();

	for (int i = 0; i < cmdList.numElem(); i++)
	{
		pBase = cmdList[i];

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

void cvar_list_collect(const ConCommandBase* cmd, Array<EqString>& list, const char* query)
{
	const ConCommandBase* pBase;
	const ConCommandListRef cmdList = g_consoleCommands->GetAllCommands();

	const int LIST_LIMIT = 50;

	for (int i = 0; i < cmdList.numElem(); i++)
	{
		if (list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		pBase = cmdList[i];

		if (!pBase->IsConVar())
			continue;

		if (pBase->GetFlags() & CV_INVISIBLE)
			continue;

		if (*query == 0 || xstristr(pBase->GetName(), query))
			list.append(pBase->GetName());
	}
}

DECLARE_CONCOMMAND_FN(revertvar)
{
	if (CMD_ARGC == 0)
	{
		Msg("Usage: revert <cvar name>\n");
		return;
	}

	ConVar* cv = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0));
	if (cv)
	{
		cv->SetValue(cv->GetDefaultValue());
		Msg("'%s' set to '%s'\n", cv->GetName(), cv->GetString());
	}
	else
		MsgError("No such cvar '%s'\n", CMD_ARGV(0).ToCString());
}

DECLARE_CONCOMMAND_FN(togglevar)
{
	if (CMD_ARGC == 0)
	{
		MsgError("No command and arguments for toggle.\n");
		return;
	}

	ConVar* pConVar = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0));

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

	g_consoleCommands->ParseFileToCommandBuffer(CMD_ARGV(0));
}

// sets ConVar value
DECLARE_CONCOMMAND_FN(set)
{
	if (CMD_ARGC == 0)
	{
		MsgError("Usage: set <convar>\n");
		return;
	}

	ConVar* pConVar = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0));

	if (!pConVar)
	{
		MsgError("Unknown variable '%s'\n", CMD_ARGV(0).ToCString());
		return;
	}

	EqString joinArgs;

	for (int i = 1; i < CMD_ARGC; i++)
		joinArgs.Append(EqString::Format(i < CMD_ARGC - 1 ? (char*)"%s " : (char*)"%s", CMD_ARGV(i).ToCString()));

	if (IsAllowedToExecute(pConVar))
	{
		pConVar->SetValue(joinArgs.GetData());
		Msg("'%s' set to '%s'\n", pConVar->GetName(), pConVar->GetString());
	}
}

// sets ConVar value before it gets initialized
DECLARE_CONCOMMAND_FN(seti)
{
	if (CMD_ARGC == 0)
	{
		MsgError("Usage: seti <convar>\n");
		return;
	}

	EqString joinArgs;
	for (int i = 1; i < CMD_ARGC; i++)
		joinArgs.Append(EqString::Format(i < CMD_ARGC - 1 ? (char*)"%s " : (char*)"%s", CMD_ARGV(i).ToCString()));

	ConVar* pConVar = (ConVar*)g_consoleCommands->FindCvar(CMD_ARGV(0));

	if (pConVar)
		pConVar->SetValue(joinArgs.GetData());
	else
		g_consoleCommands->PushConVarInitialValue(CMD_ARGV(0), joinArgs.GetData());
}

static void fncfgfiles_variants(const ConCommandBase* cmd, Array<EqString>& list, const char* query)
{
	CFileSystemFind fsFind("cfg/*.cfg", SP_MOD);
	while (fsFind.Next())
	{
		if (fsFind.IsDirectory())
			continue;

		list.append(_Es(fsFind.GetPath()).Path_Strip_Ext());
	}
}

DECLARE_CMD_F(cvarlist, "Prints out all aviable cvars", CV_UNREGISTERED);
DECLARE_CMD_F(cmdlist, "Prints out all aviable commands", CV_UNREGISTERED);
DECLARE_CMD_VARIANTS_F(exec, "Execute configuration file", fncfgfiles_variants, CV_UNREGISTERED);
DECLARE_CMD_VARIANTS_F(togglevar, "Toggles ConVar value", cvar_list_collect, CV_UNREGISTERED);
DECLARE_CMD_VARIANTS_F(set, "Sets cvar value", cvar_list_collect, CV_UNREGISTERED);
DECLARE_CMD_VARIANTS_F(seti, "Sets cvar value before it gets initialized", cvar_list_collect, CV_UNREGISTERED | CV_INVISIBLE);
DECLARE_CMD_VARIANTS_F(revertvar, "Reverts cvar to it's default value", cvar_list_collect, CV_UNREGISTERED);


void SplitCommandForValidArguments(const char* command, Array<EqString>& commands)
{
	const char* pChar = command;
	while (*pChar && CType::IsSpace(*pChar))
	{
		++pChar;
	}

	bool bInQuotes = false;
	const char* pFirstLetter = nullptr;
	for (; *pChar; ++pChar)
	{
		if (bInQuotes)
		{
			if (*pChar != '\"')
				continue;

			int nLen = (int)(pChar - pFirstLetter);

			commands.append(_Es(pFirstLetter, nLen));

			pFirstLetter = nullptr;
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

			if (CType::IsSpace(*pChar))
				continue;

			pFirstLetter = pChar;
			continue;
		}

		// Here, we're in the middle of a word. Look for the end of it.
		if (CType::IsSpace(*pChar))
		{
			int nLen = (int)(pChar - pFirstLetter);
			commands.append(_Es(pFirstLetter, nLen));
			pFirstLetter = nullptr;
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

	g_eqCore->RegisterInterface(this);
}

void CConsoleCommands::RegisterCommands()
{
	ConCommandBase::Register(&cvarlist);
	ConCommandBase::Register(&cmdlist);
	ConCommandBase::Register(&exec);
	ConCommandBase::Register(&togglevar);
	ConCommandBase::Register(&set);
	ConCommandBase::Register(&seti);
	ConCommandBase::Register(&revertvar);
}

const ConVar* CConsoleCommands::FindCvar(const char* name)
{
	ConCommandBase const* pBase = FindBase(name);

	if (pBase && pBase->IsConVar())
		return (ConVar*)pBase;

	return nullptr;
}

const ConCommand* CConsoleCommands::FindCommand(const char* name)
{
	ConCommandBase const* pBase = FindBase(name);

	if (pBase && pBase->IsConCommand())
		return (ConCommand*)pBase;

	return nullptr;
}

const ConCommandBase* CConsoleCommands::FindBase(const char* name)
{
	SortCommands();

	for (int i = 0; i < m_registeredCommands.numElem(); i++)
	{
		if (!stricmp(name, m_registeredCommands[i]->GetName()))
			return m_registeredCommands[i];
	}

	return nullptr;
}

void CConsoleCommands::RegisterCommand(ConCommandBase* pCmd)
{
	ASSERT(pCmd != nullptr);
	ASSERT_MSG(FindBase(pCmd->GetName()) == nullptr, "ConCmd/CVar %s already registered", pCmd->GetName());

	ASSERT_MSG(isCvarChar(*pCmd->GetName()), "RegisterCommand - command name has invalid start character!");

	// Already registered
	if (pCmd->IsRegistered())
		return;

	// pull initial value of ConVar if any
	if (pCmd->IsConVar())
	{
		const int nameHash = StringToHash(pCmd->GetName(), true);
		auto it = m_cvarValueStore.find(nameHash);
		if (!it.atEnd())
		{
			ConVar* cvar = static_cast<ConVar*>(pCmd);
			cvar->SetValue(*it);

			pCmd->m_nFlags |= CV_INITVALUE;
			m_cvarValueStore.remove(it);
		}
	}

	m_registeredCommands.append(pCmd);
	pCmd->m_bIsRegistered = true;

	// for alphabetic sort
	m_commandListDirty = true;
}

void CConsoleCommands::UnregisterCommand(ConCommandBase* pCmd)
{
	if (!pCmd->IsRegistered())
		return;

	m_registeredCommands.remove(pCmd);
}

void CConsoleCommands::DeInit()
{
	for (int i = 0; i < m_registeredCommands.numElem(); i++)
		((ConCommandBase*)m_registeredCommands[i])->m_bIsRegistered = false;

	m_registeredCommands.clear(true);
	m_cvarValueStore.clear(true);
}

void CConsoleCommands::SortCommands()
{
	if (!m_commandListDirty)
		return;

	arraySort(m_registeredCommands, alpha_cmd_comp);

	m_commandListDirty = false;
}

static bool hasChar(const char* chrs, char ch)
{
	while (*chrs) 
	{
		if (*chrs++ == ch)
			return true;
	}
	return false;
}

void CConsoleCommands::ForEachSeparated(const char* str, FUNC fn, void* extra)
{
	if (!str || *str == 0)
		return;

	const char* iterator = str;
	char c = *iterator;

	const char* pFirst = str;
	const char* pLast = nullptr;

	while (c)
	{
		c = *iterator;
		if (c == 0 || hasChar(CON_SEPARATORS, c))
		{
			pLast = iterator;
			const int chrCount = pLast - pFirst;
			if (chrCount > 0)
				(this->*fn)(pFirst, chrCount, extra);

			pFirst = iterator + 1;
		}
		iterator++;
	}
}

void CConsoleCommands::ParseAndAppend(const char* str, int len, void* extra)
{
	EqString tmpStr(str, len);
	int commentIdx = tmpStr.Find("//");

	if (commentIdx != -1)
		tmpStr = tmpStr.Left(commentIdx);

	tmpStr = tmpStr.TrimSpaces();

	if (tmpStr.Length() > 0)
	{
		EqString cmdStr(tmpStr.TrimSpaces());

		Array<EqString> cmdArgs(PP_SL);
		SplitCommandForValidArguments(cmdStr, cmdArgs);

		// executing file must be put to the command buffer in proper order
		if (cmdArgs.numElem() && !cmdArgs[0].CompareCaseIns("exec"))
		{
			cmdArgs.removeIndex(0);
			CC_exec_f(nullptr, cmdArgs);
			return;
		}

		strcat(m_currentCommands, EqString::Format("%s;", cmdStr.ToCString()));
	}

}

void CConsoleCommands::PushConVarInitialValue(const char* name, const char* value)
{
	const int nameHash = StringToHash(name, true);
	m_cvarValueStore.insert(nameHash, value);
}

// Executes file
void CConsoleCommands::ParseFileToCommandBuffer(const char* pszFilename)
{
	EqString cfgFileName(pszFilename);

	if (cfgFileName.Path_Extract_Ext() != "cfg" && !g_fileSystem->FileExist(cfgFileName))
	{
		if (!g_fileSystem->FileExist("cfg/" + cfgFileName))
			cfgFileName = cfgFileName.Path_Strip_Ext() + ".cfg";
		else
			cfgFileName = "cfg/" + cfgFileName;
	}

	if (!g_fileSystem->FileExist(cfgFileName))
		cfgFileName = "cfg/" + cfgFileName;

	char* buf = (char*)g_fileSystem->GetFileBuffer(cfgFileName, nullptr, -1);

	if (!buf)
	{
		MsgError("Couldn't execute configuraton file '%s'\n", pszFilename);
		return;
	}

	if (strlen(buf) < 1)
	{
		PPFree(buf);
		return; //Don't parse me about empty file
	}

	ForEachSeparated(buf, &CConsoleCommands::ParseAndAppend, nullptr);
	PPFree(buf);
}

// Sets command buffer
void CConsoleCommands::SetCommandBuffer(const char* pszBuffer)
{
	ASSERT(strlen(pszBuffer) < COMMANDBUFFER_SIZE);
	ClearCommandBuffer();

	ForEachSeparated((char*)pszBuffer, &CConsoleCommands::ParseAndAppend, nullptr);
}

// Appends to command buffer
void CConsoleCommands::AppendToCommandBuffer(const char* pszBuffer)
{
	const size_t new_len = strlen(pszBuffer) + strlen(m_currentCommands);

	ASSERT(new_len < COMMANDBUFFER_SIZE);
	ForEachSeparated((char*)pszBuffer, &CConsoleCommands::ParseAndAppend, nullptr);

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
	Array<EqString>* failedCmds;
	bool quiet;
};

void CConsoleCommands::SplitOnArgsAndExec(const char* str, int len, void* extra)
{
	execOptions_t* options = (execOptions_t*)extra;
	ASSERT(options);

	EqString commandStr(str, len);
	Array<EqString> cmdArgs(PP_SL);

	// split it
	SplitCommandForValidArguments(commandStr, cmdArgs);

	if (cmdArgs.numElem() == 0)
		return;

	ConCommandBase* pBase = (ConCommandBase*)FindBase(cmdArgs[0].GetData());

	if (!pBase) //Failed?
	{
		if (!options->quiet)
			MsgError("Unknown command or variable '%s'\n", cmdArgs[0].GetData());

		if (options->failedCmds)
			options->failedCmds->append(commandStr);
		return;
	}

	if (options->filterFn)
	{
		if (!(options->filterFn)(pBase, cmdArgs))
			return;
	}

	// remove cmd name
	cmdArgs.removeIndex(0);

	if (!IsAllowedToExecute(pBase))
	{
		MsgWarning("ConCommand/ConVar '%s' is%s protected\n", pBase->GetName(), (pBase->GetFlags() & CV_CHEAT) ? " cheat" : "");
		return;
	}

	if (pBase->IsConVar())
	{
		ConVar* pConVar = (ConVar*)pBase;

		// Primitive executor tries to find optional arguments
		if (cmdArgs.numElem() == 0)
		{
			MsgInfo("%s is '%s' (default value is '%s')\n", pConVar->GetName(), pConVar->GetString(), pConVar->GetDefaultValue());
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
bool CConsoleCommands::ExecuteCommandBuffer(cmdFilterFn_t filterFn, bool quiet, Array<EqString>* failedCmds)
{
	SortCommands();

	if (!stricmp(m_lastExecutedCommands, m_currentCommands))
		m_sameCommandsExecuted++;

	strcpy(m_lastExecutedCommands, m_currentCommands);

	if (m_sameCommandsExecuted > MAX_SAME_COMMANDS - 1)
	{
		MsgError("Stack buffer overflow prevented! Exiting from cycle!\n");
		ClearCommandBuffer();
		return false;
	}

	if (strlen(m_currentCommands) <= 0)
		return false;

	execOptions_t options;
	options.filterFn = filterFn;
	options.failedCmds = failedCmds;
	options.quiet = quiet;

	ForEachSeparated(m_currentCommands, &CConsoleCommands::SplitOnArgsAndExec, &options);

	return true;
}