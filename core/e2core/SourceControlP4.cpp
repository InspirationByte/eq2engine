//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Perforce source control provider (p4 command line client)
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

#include <stdlib.h>

#include "core/core_common.h"
#include "SourceControlP4.h"

using namespace Threading;

// global options of every p4 call: script output, tagged fields, network wait limit
static constexpr const char* P4_GLOBAL_OPTIONS = "-s -ztag -zprog=e2core -v net.maxwait=15 ";

// 'add', 'edit' or 'reopen' with the file type option, SC_FILETYPE_AUTO leaves the type to the server
static EqString P4TypedCommand(const char* command, ESourceControlFileType fileType)
{
	if (fileType == SC_FILETYPE_BINARY)
		return EqString::Format("%s -t binary", command);
	return command;
}

//-------------------------------------------------------------------------------------------------
// Process runner
//-------------------------------------------------------------------------------------------------

#ifdef _WIN32

// runs a command line, writes 'input' to its stdin and collects stdout and stderr
static bool P4RunProcess(const char* cmdLine, const char* workDir, EqStringRef input, EqString& output, int& exitCode)
{
	SECURITY_ATTRIBUTES secAttrs{};
	secAttrs.nLength = sizeof(secAttrs);
	secAttrs.bInheritHandle = TRUE;

	HANDLE outRead = nullptr;
	HANDLE outWrite = nullptr;
	if (!CreatePipe(&outRead, &outWrite, &secAttrs, 0))
		return false;

	HANDLE inRead = nullptr;
	HANDLE inWrite = nullptr;
	if (!CreatePipe(&inRead, &inWrite, &secAttrs, 0))
	{
		CloseHandle(outRead);
		CloseHandle(outWrite);
		return false;
	}

	// only the child ends are inherited
	SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(inWrite, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOW startInfo{};
	startInfo.cb = sizeof(startInfo);
	startInfo.dwFlags = STARTF_USESTDHANDLES;
	startInfo.hStdInput = inRead;
	startInfo.hStdOutput = outWrite;
	startInfo.hStdError = outWrite;

	EqWString cmdLineW;
	AnsiUnicodeConverter(cmdLineW, cmdLine);

	EqWString workDirW;
	if (workDir && *workDir)
		AnsiUnicodeConverter(workDirW, workDir);

	// CreateProcess may modify the command line buffer
	PROCESS_INFORMATION procInfo{};
	const BOOL started = CreateProcessW(nullptr, cmdLineW.GetData(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
		workDirW.Length() ? workDirW.ToCString() : nullptr, &startInfo, &procInfo);
	const DWORD startError = started ? 0 : GetLastError();

	// read returns when all write ends are closed, so close ours
	CloseHandle(outWrite);
	CloseHandle(inRead);

	if (!started)
	{
		MsgWarning("p4: can not run '%s' (error %u)\n", cmdLine, startError);
		CloseHandle(outRead);
		CloseHandle(inWrite);
		return false;
	}

	// input is small (changelist spec), the child reads it before producing output
	if (input.Length())
	{
		DWORD bytesWritten = 0;
		WriteFile(inWrite, input.ToCString(), static_cast<DWORD>(input.Length()), &bytesWritten, nullptr);
	}

	// closed stdin also makes password prompts fail instead of waiting
	CloseHandle(inWrite);

	char readBuffer[1024];
	DWORD bytesRead = 0;
	while (ReadFile(outRead, readBuffer, sizeof(readBuffer), &bytesRead, nullptr) && bytesRead > 0)
		output.Append(readBuffer, static_cast<int>(bytesRead));

	WaitForSingleObject(procInfo.hProcess, INFINITE);

	DWORD processExitCode = 0;
	GetExitCodeProcess(procInfo.hProcess, &processExitCode);
	exitCode = static_cast<int>(processExitCode);

	CloseHandle(procInfo.hProcess);
	CloseHandle(procInfo.hThread);
	CloseHandle(outRead);

	return true;
}

static EqString P4QuoteArg(EqStringRef arg)
{
	// file paths can not contain quotes on Windows
	return EqString::Format("\"%s\"", arg.ToCString());
}

static int P4GetFileAccess(EqStringRef fullPath)
{
	EqWString pathW;
	AnsiUnicodeConverter(pathW, fullPath);

	const DWORD attributes = GetFileAttributesW(pathW.ToCString());
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return -1;

	return (attributes & FILE_ATTRIBUTE_READONLY) ? 0 : 1;
}

// p4.exe from PATH, otherwise from the Perforce installation directory (P4INSTROOT is set by the
// installer). Debugging from Visual Studio replaces PATH with the binaries directory.
static EqString P4FindExecutable()
{
	wchar_t pathW[MAX_PATH]{ 0 };
	if (SearchPathW(nullptr, L"p4.exe", nullptr, MAX_PATH, pathW, nullptr))
	{
		EqString path;
		AnsiUnicodeConverter(path, pathW);
		return path;
	}

	static const HKEY s_rootKeys[] = { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE };
	for (HKEY rootKey : s_rootKeys)
	{
		DWORD size = sizeof(pathW);
		if (RegGetValueW(rootKey, L"SOFTWARE\\Perforce\\Environment", L"P4INSTROOT", RRF_RT_REG_SZ, nullptr, pathW, &size) != ERROR_SUCCESS)
			continue;

		EqString installDir;
		AnsiUnicodeConverter(installDir, pathW);

		const EqString path = fnmPathCombine(installDir, "p4.exe");
		if (P4GetFileAccess(path) != -1)
			return path;
	}

	return "p4";
}

#else

static bool P4RunProcess(const char* cmdLine, const char* workDir, EqStringRef input, EqString& output, int& exitCode)
{
	int outPipe[2];
	if (pipe(outPipe) != 0)
		return false;

	int inPipe[2];
	if (pipe(inPipe) != 0)
	{
		close(outPipe[0]);
		close(outPipe[1]);
		return false;
	}

	const pid_t pid = fork();
	if (pid == 0)
	{
		dup2(inPipe[0], STDIN_FILENO);
		dup2(outPipe[1], STDOUT_FILENO);
		dup2(outPipe[1], STDERR_FILENO);
		close(inPipe[0]);
		close(inPipe[1]);
		close(outPipe[0]);
		close(outPipe[1]);

		if (workDir && *workDir && chdir(workDir) != 0)
			_exit(127);

		execl("/bin/sh", "sh", "-c", cmdLine, static_cast<char*>(nullptr));
		_exit(127);
	}

	close(inPipe[0]);
	close(outPipe[1]);

	if (pid < 0)
	{
		close(inPipe[1]);
		close(outPipe[0]);
		return false;
	}

	if (input.Length())
	{
		const ssize_t written = write(inPipe[1], input.ToCString(), input.Length());
		(void)written;
	}
	close(inPipe[1]);

	char readBuffer[1024];
	ssize_t bytesRead = 0;
	while ((bytesRead = read(outPipe[0], readBuffer, sizeof(readBuffer))) > 0)
		output.Append(readBuffer, static_cast<int>(bytesRead));
	close(outPipe[0]);

	int status = 0;
	waitpid(pid, &status, 0);
	exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

	return true;
}

static EqString P4QuoteArg(EqStringRef arg)
{
	EqString quoted("'");
	for (int i = 0; i < arg.Length(); ++i)
	{
		if (arg[i] == '\'')
			quoted.Append("'\\''");
		else
			quoted.Append(arg[i]);
	}
	quoted.Append('\'');
	return quoted;
}

static int P4GetFileAccess(EqStringRef fullPath)
{
	if (access(fullPath, F_OK) != 0)
		return -1;

	return access(fullPath, W_OK) == 0 ? 1 : 0;
}

// the shell finds p4 in PATH
static EqString P4FindExecutable()
{
	return "p4";
}

#endif // _WIN32

//-------------------------------------------------------------------------------------------------
// Output parsing
//-------------------------------------------------------------------------------------------------

static int P4PathHash(EqStringRef path)
{
	constexpr int MULTIPLIER = 37;

	int hash = 0;
	for (int i = 0; i < path.Length(); ++i)
	{
		char chr = path[i];
		if (chr == '/' || chr == '\\')
			chr = CORRECT_PATH_SEPARATOR;
		hash = MULTIPLIER * hash + static_cast<int>(CType::LowerChar(chr));
	}

	return hash;
}

static void P4ParseOutput(EqStringRef output, P4Result& result)
{
	int lineStart = 0;
	while (lineStart < output.Length())
	{
		int lineEnd = output.Find('\n', true, lineStart);
		if (lineEnd == -1)
			lineEnd = output.Length();

		const EqStringRef line = output.Mid(lineStart, lineEnd - lineStart).TrimChar('\r', false, true);
		lineStart = lineEnd + 1;

		const int prefixEnd = line.Find(':', true);
		if (prefixEnd == -1)
		{
			if (line.Length())
				result.messages.append(line);
			continue;
		}

		const EqStringRef prefix = line.Left(prefixEnd);
		const EqStringRef text = line.Mid(prefixEnd + 1, line.Length() - prefixEnd - 1).TrimSpaces(true, false);

		// tagged fields are 'info1: key value', nested levels are 'info2:'...
		if (prefix.Length() > 4 && !prefix.Left(4).Compare("info") && CType::IsDigit(prefix[4]))
		{
			const int keyEnd = text.Find(' ', true);

			P4Field& field = result.fields.append();
			if (keyEnd == -1)
			{
				field.key = text;
			}
			else
			{
				field.key = text.Left(keyEnd);
				field.value = text.Mid(keyEnd + 1, text.Length() - keyEnd - 1);
			}
			continue;
		}

		if (!prefix.Compare("exit"))
		{
			result.exitCode = atoi(text);
			continue;
		}

		result.messages.append(line);
	}
}

const P4Field* P4Result::FindField(EqStringRef key) const
{
	for (const P4Field& field : fields)
	{
		if (!field.key.Compare(key))
			return &field;
	}
	return nullptr;
}

bool P4Result::HasMessage(EqStringRef text) const
{
	for (const EqString& message : messages)
	{
		if (message.Find(text, false) != -1)
			return true;
	}
	return false;
}

bool P4Result::HasError() const
{
	for (const EqString& message : messages)
	{
		if (!message.Left(6).Compare("error:"))
			return true;
	}
	return false;
}

static EP4FileState P4StateFromOpenAction(EqStringRef action)
{
	if (!action.Compare("add") || !action.Compare("move/add") || !action.Compare("branch"))
		return P4_FILE_OPENED_ADD;

	if (!action.Compare("delete") || !action.Compare("move/delete"))
		return P4_FILE_OPENED_DELETE;

	return P4_FILE_OPENED_EDIT;
}

static int P4ChangeFromField(const P4Field* field)
{
	if (!field)
		return -1;
	if (!field->value.Compare("default"))
		return 0;
	return atoi(field->value);
}

// state of an fstat record
static P4FileInfo P4FileInfoFromRecord(const P4Field* action, const P4Field* change, const P4Field* headAction)
{
	P4FileInfo info;
	if (action)
	{
		info.state = P4StateFromOpenAction(action->value);
		info.change = P4ChangeFromField(change);
	}
	else if (headAction && (!headAction->value.Compare("delete") || !headAction->value.Compare("move/delete")))
	{
		// deleted at head revision, can be added again
		info.state = P4_FILE_NOT_IN_DEPOT;
	}
	else
	{
		info.state = P4_FILE_SYNCED;
	}
	return info;
}

static void P4PrintMessages(const P4Result& result)
{
	for (const EqString& message : result.messages)
		MsgWarning("p4: %s\n", message.ToCString());
}

//-------------------------------------------------------------------------------------------------
// Provider
//-------------------------------------------------------------------------------------------------

CSourceControlP4::CSourceControlP4(EqStringRef workDir, EqStringRef executable, EqStringRef defaultChangelistDesc, bool addNewFiles)
	: m_workDir(workDir)
	, m_executable(executable)
	, m_defaultChangelistDesc(defaultChangelistDesc)
	, m_addNewFiles(addNewFiles)
{
}

bool CSourceControlP4::RunCommand(EqStringRef args, P4Result& result, EqStringRef input)
{
	if (!m_executable.Length())
		m_executable = P4FindExecutable();

	EqString cmdLine(P4QuoteArg(m_executable));
	cmdLine.Append(' ');
	cmdLine.Append(P4_GLOBAL_OPTIONS);
	cmdLine.Append(args);

	DevMsg(DEVMSG_FS, "%s\n", cmdLine.ToCString());

	EqString output;
	result.started = P4RunProcess(cmdLine, m_workDir, input, output, result.exitCode);
	if (!result.started)
		return false;

	P4ParseOutput(output, result);
	return result.Succeeded();
}

// returns true and disables the provider when the server can not be used at all
bool CSourceControlP4::CheckConnectionError(const P4Result& result)
{
	if (!result.started)
	{
		Disable(EqString::Format("can not run '%s', set 'Executable' in the 'SourceControl p4' section of E2.CONFIG", m_executable.ToCString()));
		return true;
	}

	static constexpr const char* s_connectionErrors[] = {
		"Connect to server failed",
		"P4PASSWD",
		"session has expired",
		"please login",
		"Perforce password",
	};

	for (const char* errorText : s_connectionErrors)
	{
		if (!result.HasMessage(errorText))
			continue;

		P4PrintMessages(result);
		Disable("can not connect to the server, check P4PORT or run 'p4 login'");
		return true;
	}

	return false;
}

void CSourceControlP4::Disable(EqStringRef reason)
{
	if (m_failed)
		return;

	m_failed = true;
	MsgWarning("Source control (p4) is disabled: %s\n", reason.ToCString());
}

bool CSourceControlP4::EnsureConnected()
{
	if (m_failed)
		return false;

	if (m_connected)
		return true;

	P4Result infoResult;
	RunCommand("info", infoResult);
	if (CheckConnectionError(infoResult))
		return false;

	const P4Field* clientName = infoResult.FindField("clientName");
	const P4Field* clientRoot = infoResult.FindField("clientRoot");
	const P4Field* userName = infoResult.FindField("userName");
	if (!clientName || !userName || !clientName->value.Compare("*unknown*"))
	{
		P4PrintMessages(infoResult);
		Disable("no p4 client workspace for the working directory");
		return false;
	}

	// 'p4 info' does not require a login, check the ticket explicitly
	P4Result loginResult;
	RunCommand("login -s", loginResult);
	if (CheckConnectionError(loginResult))
		return false;

	if (!loginResult.Succeeded())
	{
		P4PrintMessages(loginResult);
		Disable("not logged in, run 'p4 login'");
		return false;
	}

	m_clientName = clientName->value;
	m_userName = userName->value;

	// 'null' root is used by clients with alternate roots only
	if (clientRoot && clientRoot->value.CompareCaseIns("null"))
	{
		EqString clientRootPath(clientRoot->value);
		fnmPathFixSeparators(clientRootPath);
		m_clientRoot = clientRootPath.TrimChar(CORRECT_PATH_SEPARATOR, false, true);
	}

	m_connected = true;
	MsgInfo("* Source control: p4 client '%s', user '%s'\n", m_clientName.ToCString(), m_userName.ToCString());

	return true;
}

EqStringRef CSourceControlP4::GetCurrentChangelistDesc() const
{
	const EqStringRef scopeDesc = GetScopeChangelistDesc();
	if (scopeDesc.Length())
		return scopeDesc;
	return m_defaultChangelistDesc;
}

// finds a pending changelist with the description or creates a new one
int CSourceControlP4::EnsureChangelist(EqStringRef desc)
{
	const int descHash = P4PathHash(desc);
	auto it = m_changelists.find(descHash);
	if (!it.atEnd())
		return *it;

	// trimmed copy, temp strings are recycled while parsing long outputs
	const EqString wantedDesc(desc.TrimSpaces());

	P4Result changesResult;
	RunCommand(EqString::Format("changes -s pending -l -c %s -u %s", P4QuoteArg(m_clientName).ToCString(), P4QuoteArg(m_userName).ToCString()), changesResult);
	if (CheckConnectionError(changesResult))
		return -1;

	// records start with 'change', the first description line comes as 'desc'
	int change = -1;
	int foundChange = -1;
	for (const P4Field& field : changesResult.fields)
	{
		if (!field.key.Compare("change"))
		{
			change = atoi(field.value);
			continue;
		}

		if (field.key.Compare("desc") || change <= 0)
			continue;

		if (!field.value.TrimSpaces().CompareCaseIns(wantedDesc))
		{
			foundChange = change;
			break;
		}
	}

	if (foundChange <= 0)
	{
		EqString spec;
		spec.AppendFmt("Change:\tnew\n\nClient:\t%s\n\nUser:\t%s\n\nStatus:\tnew\n\nDescription:\n\t%s\n",
			m_clientName.ToCString(), m_userName.ToCString(), wantedDesc.ToCString());

		P4Result createResult;
		RunCommand("change -i", createResult, spec);
		if (CheckConnectionError(createResult))
			return -1;

		// 'info: Change 1234 created.'
		for (const EqString& message : createResult.messages)
		{
			const int changePos = message.Find("Change ", true);
			if (changePos != -1 && message.Find("created", true) != -1)
			{
				foundChange = atoi(message.ToCString() + changePos + 7);
				break;
			}
		}

		if (foundChange <= 0)
		{
			P4PrintMessages(createResult);
			MsgWarning("p4: can not create changelist '%s'\n", wantedDesc.ToCString());
			return -1;
		}

		MsgInfo("p4: created changelist %d '%s'\n", foundChange, wantedDesc.ToCString());
	}

	m_changelists.insert(descHash, foundChange);
	return foundChange;
}

// runs '<command> -c <changelist> <fileArgs>' for the current changelist
bool CSourceControlP4::RunChangelistCommand(EqStringRef command, EqStringRef fileArgs, P4Result& result, EqStringRef input)
{
	const EqString desc(GetCurrentChangelistDesc());

	// a cached changelist may be submitted or deleted meanwhile, then look it up again once
	for (int attempt = 0; attempt < 2; ++attempt)
	{
		const int change = EnsureChangelist(desc);
		if (change <= 0)
			return false;

		result.fields.clear();
		result.messages.clear();
		result.exitCode = -1;
		result.started = false;

		RunCommand(EqString::Format("%s -c %d %s", command.ToCString(), change, fileArgs.ToCString()), result, input);
		if (CheckConnectionError(result))
			return false;

		const bool staleChange = result.HasMessage(EqString::Format("Change %d", change)) && result.HasError();
		if (!staleChange)
			break;

		auto staleIt = m_changelists.find(P4PathHash(desc));
		if (!staleIt.atEnd())
			m_changelists.remove(staleIt);
	}

	return result.Succeeded();
}

bool CSourceControlP4::IsUnderClientRoot(EqStringRef fullPath) const
{
	if (!m_clientRoot.Length())
		return true;

	if (fullPath.Length() <= m_clientRoot.Length())
		return false;

	if (fullPath[m_clientRoot.Length()] != CORRECT_PATH_SEPARATOR)
		return false;

	return !fullPath.Left(m_clientRoot.Length()).CompareCaseIns(m_clientRoot);
}

const P4KnownDir* CSourceControlP4::FindKnownDir(EqStringRef fullPath) const
{
	for (const P4KnownDir& dir : m_knownDirs)
	{
		if (fullPath.Length() <= dir.path.Length() || fullPath[dir.path.Length()] != CORRECT_PATH_SEPARATOR)
			continue;

		if (!fullPath.Left(dir.path.Length()).CompareCaseIns(dir.path))
			return &dir;
	}
	return nullptr;
}

P4FileInfo CSourceControlP4::GetCachedFileInfo(EqStringRef fullPath) const
{
	auto it = m_fileInfos.find(P4PathHash(fullPath));

	// a checked out directory has all its depot files cached, other files are not in the depot
	const P4KnownDir* knownDir = FindKnownDir(fullPath);
	if (knownDir && (it.atEnd() || it->generation < knownDir->generation))
	{
		P4FileInfo info;
		info.state = P4_FILE_NOT_IN_DEPOT;
		return info;
	}

	if (it.atEnd())
		return P4FileInfo();
	return *it;
}

void CSourceControlP4::SetFileInfo(EqStringRef fullPath, EP4FileState state, int change)
{
	P4FileInfo info;
	info.state = state;
	info.change = change;
	info.generation = m_cacheGeneration;
	m_fileInfos[P4PathHash(fullPath)] = info;
}

// cached state of a file in a checked out directory, otherwise asks the server
P4FileInfo CSourceControlP4::GetFileInfo(EqStringRef fullPath)
{
	if (FindKnownDir(fullPath))
		return GetCachedFileInfo(fullPath);
	return QueryFileInfo(fullPath);
}

P4FileInfo CSourceControlP4::QueryFileInfo(EqStringRef fullPath)
{
	P4FileInfo info;

	P4Result result;
	RunCommand(EqString::Format("fstat %s", P4QuoteArg(fullPath).ToCString()), result);
	if (CheckConnectionError(result))
		return info;

	if (result.FindField("depotFile"))
	{
		info = P4FileInfoFromRecord(result.FindField("action"), result.FindField("change"), result.FindField("headAction"));
	}
	else if (result.HasMessage("no such file"))
	{
		info.state = P4_FILE_NOT_IN_DEPOT;
	}
	else
	{
		// not under the client root or not in the client view
		info.state = P4_FILE_IGNORED;
	}

	SetFileInfo(fullPath, info.state, info.change);
	return info;
}

bool CSourceControlP4::OpenForEdit(EqStringRef fullPath, const P4FileInfo& info, ESourceControlFileType fileType)
{
	if (info.state == P4_FILE_OPENED_DELETE && !RevertKeepFile(fullPath))
		return false;

	P4Result result;
	if (!RunChangelistCommand(P4TypedCommand("edit", fileType), P4QuoteArg(fullPath), result))
	{
		P4PrintMessages(result);
		MsgWarning("p4: can not check out '%s'\n", fullPath.ToCString());
		return false;
	}

	SetFileInfo(fullPath, P4_FILE_OPENED_EDIT, EnsureChangelist(GetCurrentChangelistDesc()));
	DevMsg(DEVMSG_FS, "p4: checked out '%s'\n", fullPath.ToCString());

	return true;
}

// adds the file now, or when the changelist scope of the calling thread is popped
void CSourceControlP4::AddNewFile(EqStringRef fullPath, ESourceControlFileType fileType)
{
	if (!m_addNewFiles)
		return;

	if (DeferScopeAdd(fullPath, fileType))
		return;

	AddFile(fullPath, fileType);
}

bool CSourceControlP4::AddFile(EqStringRef fullPath, ESourceControlFileType fileType)
{
	P4Result result;
	if (!RunChangelistCommand(P4TypedCommand("add", fileType), P4QuoteArg(fullPath), result))
	{
		if (result.HasMessage("ignored file") || result.HasMessage("not in client view"))
		{
			SetFileInfo(fullPath, P4_FILE_IGNORED, -1);
			DevMsg(DEVMSG_FS, "p4: '%s' is ignored\n", fullPath.ToCString());
			return false;
		}

		P4PrintMessages(result);
		MsgWarning("p4: can not add '%s'\n", fullPath.ToCString());
		return false;
	}

	SetFileInfo(fullPath, P4_FILE_OPENED_ADD, EnsureChangelist(GetCurrentChangelistDesc()));
	DevMsg(DEVMSG_FS, "p4: added '%s'\n", fullPath.ToCString());

	return true;
}

// inside a changelist scope moves an opened file into the scope changelist, setting the file type
bool CSourceControlP4::ReopenToCurrentChangelist(EqStringRef fullPath, const P4FileInfo& info, ESourceControlFileType fileType)
{
	if (!GetScopeChangelistDesc().Length())
		return true;

	const int change = EnsureChangelist(GetCurrentChangelistDesc());
	if (change <= 0 || info.change == change)
		return true;

	P4Result result;
	if (!RunChangelistCommand(P4TypedCommand("reopen", fileType), P4QuoteArg(fullPath), result))
	{
		P4PrintMessages(result);
		return false;
	}

	SetFileInfo(fullPath, info.state, change);
	return true;
}

bool CSourceControlP4::RevertKeepFile(EqStringRef fullPath)
{
	P4Result result;
	RunCommand(EqString::Format("revert -k %s", P4QuoteArg(fullPath).ToCString()), result);
	if (CheckConnectionError(result))
		return false;

	if (!result.Succeeded())
	{
		P4PrintMessages(result);
		return false;
	}

	auto it = m_fileInfos.find(P4PathHash(fullPath));
	if (!it.atEnd())
		m_fileInfos.remove(it);

	return true;
}

bool CSourceControlP4::IsAvailable()
{
	CScopedMutex m(m_mutex);
	return EnsureConnected();
}

void CSourceControlP4::CheckoutDir(EqStringRef fullPath)
{
	CScopedMutex m(m_mutex);
	if (!EnsureConnected() || !IsUnderClientRoot(fullPath))
		return;

	const EqString dirPath(fullPath.TrimChar(CORRECT_PATH_SEPARATOR, false, true));
	const EqString dirWildcard(P4QuoteArg(EqString::Format("%s%c...", dirPath.ToCString(), CORRECT_PATH_SEPARATOR)));

	P4Result fstatResult;
	RunCommand(EqString::Format("fstat %s", dirWildcard.ToCString()), fstatResult);
	if (CheckConnectionError(fstatResult))
		return;

	const int change = EnsureChangelist(GetCurrentChangelistDesc());
	if (change <= 0)
		return;

	struct DirFile
	{
		EqString	path;
		P4FileInfo	info;
	};
	Array<DirFile> dirFiles{ PP_SL };

	// records start with 'depotFile'
	const P4Field* clientFile = nullptr;
	const P4Field* action = nullptr;
	const P4Field* actionChange = nullptr;
	const P4Field* headAction = nullptr;
	auto flushRecord = [&]() {
		if (clientFile)
		{
			DirFile& file = dirFiles.append();
			file.path = clientFile->value;
			file.info = P4FileInfoFromRecord(action, actionChange, headAction);
		}
		clientFile = action = actionChange = headAction = nullptr;
	};

	for (const P4Field& field : fstatResult.fields)
	{
		if (!field.key.Compare("depotFile"))
			flushRecord();
		else if (!field.key.Compare("clientFile"))
			clientFile = &field;
		else if (!field.key.Compare("action"))
			action = &field;
		else if (!field.key.Compare("change"))
			actionChange = &field;
		else if (!field.key.Compare("headAction"))
			headAction = &field;
	}
	flushRecord();

	bool needEdit = false;
	bool needReopen = false;
	for (const DirFile& file : dirFiles)
	{
		if (file.info.state == P4_FILE_SYNCED)
			needEdit = true;
		else if (file.info.state >= P4_FILE_OPENED_EDIT && file.info.change != change)
			needReopen = true;
	}

	bool edited = false;
	if (needEdit)
	{
		P4Result editResult;
		edited = RunChangelistCommand("edit", dirWildcard, editResult);
		if (!edited)
			P4PrintMessages(editResult);
	}

	bool reopened = false;
	if (needReopen)
	{
		P4Result reopenResult;
		reopened = RunChangelistCommand("reopen", dirWildcard, reopenResult);
		if (!reopened)
			P4PrintMessages(reopenResult);
	}

	// states stored before are outdated, files missing in the listing are not in the depot
	++m_cacheGeneration;

	int numCheckedOut = 0;
	for (const DirFile& file : dirFiles)
	{
		P4FileInfo info = file.info;
		if (info.state == P4_FILE_SYNCED && edited)
		{
			info.state = P4_FILE_OPENED_EDIT;
			info.change = change;
			++numCheckedOut;
		}
		else if (info.state >= P4_FILE_OPENED_EDIT && reopened)
		{
			info.change = change;
		}

		// a file failed to check out is asked again when it is written
		if (info.state == P4_FILE_SYNCED)
			continue;

		SetFileInfo(file.path, info.state, info.change);
	}

	P4KnownDir* knownDir = nullptr;
	for (P4KnownDir& dir : m_knownDirs)
	{
		if (!dir.path.CompareCaseIns(dirPath))
		{
			knownDir = &dir;
			break;
		}
	}

	if (!knownDir)
	{
		knownDir = &m_knownDirs.append();
		knownDir->path = dirPath;
	}
	knownDir->generation = m_cacheGeneration;

	DevMsg(DEVMSG_FS, "p4: checked out %d of %d depot files in '%s'\n", numCheckedOut, dirFiles.numElem(), dirPath.ToCString());
}

// adds files queued by a changelist scope with one command
void CSourceControlP4::AddFiles(ArrayCRef<EqString> fullPaths, ESourceControlFileType fileType)
{
	CScopedMutex m(m_mutex);
	if (!fullPaths.numElem() || !EnsureConnected())
		return;

	EqString fileList;
	for (const EqString& fullPath : fullPaths)
	{
		fileList.Append(fullPath);
		fileList.Append('\n');
	}

	// file names are read from stdin
	P4Result result;
	RunChangelistCommand(P4TypedCommand("-x - add", fileType), "", result, fileList);
	if (m_failed)
		return;

	const int change = EnsureChangelist(GetCurrentChangelistDesc());

	// files not reported are asked again when they are written
	for (const EqString& fullPath : fullPaths)
	{
		auto it = m_fileInfos.find(P4PathHash(fullPath));
		if (!it.atEnd())
			m_fileInfos.remove(it);
	}

	int numAdded = 0;
	for (const P4Field& field : result.fields)
	{
		if (field.key.Compare("clientFile"))
			continue;

		SetFileInfo(field.value, P4_FILE_OPENED_ADD, change);
		++numAdded;
	}

	// 'info: <path> - ignored file can't be added.'
	for (const EqString& message : result.messages)
	{
		const int ignoredPos = message.Find(" - ignored file", true);
		const int pathStart = message.Find(": ", true);
		if (ignoredPos != -1 && pathStart != -1 && pathStart < ignoredPos)
		{
			SetFileInfo(message.Mid(pathStart + 2, ignoredPos - pathStart - 2), P4_FILE_IGNORED, -1);
			continue;
		}

		if (!message.Left(6).Compare("error:") || !message.Left(8).Compare("warning:"))
			MsgWarning("p4: %s\n", message.ToCString());
	}

	DevMsg(DEVMSG_FS, "p4: added %d of %d files\n", numAdded, fullPaths.numElem());
}

// reverts files of a changelist scope that were checked out and written without changes
void CSourceControlP4::RevertUnchangedFiles(ArrayCRef<EqString> fullPaths)
{
	CScopedMutex m(m_mutex);
	if (!fullPaths.numElem() || !EnsureConnected())
		return;

	EqString fileList;
	for (const EqString& fullPath : fullPaths)
	{
		fileList.Append(fullPath);
		fileList.Append('\n');
	}

	// changed files are left opened and not reported
	P4Result result;
	RunCommand("-x - revert -a", result, fileList);
	if (CheckConnectionError(result))
		return;

	// records of reverted files: depotFile, clientFile (local path), oldAction, action 'reverted'
	int numReverted = 0;
	for (const P4Field& field : result.fields)
	{
		if (field.key.Compare("clientFile"))
			continue;

		SetFileInfo(field.value, P4_FILE_SYNCED, -1);
		++numReverted;
	}

	// warnings are files reverted outside the application meanwhile
	for (const EqString& message : result.messages)
	{
		if (!message.Left(6).Compare("error:"))
			MsgWarning("p4: %s\n", message.ToCString());
	}

	DevMsg(DEVMSG_FS, "p4: reverted %d unchanged of %d checked out files\n", numReverted, fullPaths.numElem());
}

void CSourceControlP4::PrepareFileWrite(EqStringRef fullPath, ESourceControlFileType fileType)
{
	CScopedMutex m(m_mutex);
	if (!EnsureConnected() || !IsUnderClientRoot(fullPath))
		return;

	const int fileAccess = P4GetFileAccess(fullPath);

	// missing files are added or checked out by FinishFileWrite
	if (fileAccess == -1)
		return;

	P4FileInfo info = GetCachedFileInfo(fullPath);

	// a read-only file can be reverted outside the application, never trust the cache for it
	const bool trustCache = (fileAccess == 1) && info.state != P4_FILE_UNKNOWN && info.state != P4_FILE_SYNCED;
	if (!trustCache)
		info = QueryFileInfo(fullPath);

	switch (info.state)
	{
		case P4_FILE_SYNCED:
		case P4_FILE_OPENED_DELETE:
			OpenForEdit(fullPath, info, fileType);
			break;
		case P4_FILE_OPENED_EDIT:
		case P4_FILE_OPENED_ADD:
			ReopenToCurrentChangelist(fullPath, info, fileType);
			break;
		case P4_FILE_NOT_IN_DEPOT:
			// existing file the application writes, ignored files are cached once 'p4 add' rejects them
			AddNewFile(fullPath, fileType);
			break;
		default:
			break;
	}

	// the file is written before the scope is popped, an unchanged one leaves the changelist
	if (GetCachedFileInfo(fullPath).state == P4_FILE_OPENED_EDIT)
		DeferScopeRevertUnchanged(fullPath);
}

void CSourceControlP4::FinishFileWrite(EqStringRef fullPath, ESourceControlFileType fileType)
{
	CScopedMutex m(m_mutex);
	if (!EnsureConnected() || !IsUnderClientRoot(fullPath))
		return;

	// the file did not exist before, a cached state is outdated unless its directory is checked out
	const P4FileInfo info = GetFileInfo(fullPath);
	switch (info.state)
	{
		case P4_FILE_NOT_IN_DEPOT:
			AddNewFile(fullPath, fileType);
			break;
		case P4_FILE_SYNCED:
		case P4_FILE_OPENED_DELETE:
			OpenForEdit(fullPath, info, fileType);
			break;
		case P4_FILE_OPENED_EDIT:
		case P4_FILE_OPENED_ADD:
			ReopenToCurrentChangelist(fullPath, info, fileType);
			break;
		default:
			break;
	}

	if (GetCachedFileInfo(fullPath).state == P4_FILE_OPENED_EDIT)
		DeferScopeRevertUnchanged(fullPath);
}

bool CSourceControlP4::RemoveFile(EqStringRef fullPath)
{
	CScopedMutex m(m_mutex);
	if (!EnsureConnected() || !IsUnderClientRoot(fullPath))
		return false;

	// a file removed before its scope is popped must not be added or reverted
	CancelScopeFile(fullPath);

	const P4FileInfo info = GetFileInfo(fullPath);
	switch (info.state)
	{
		case P4_FILE_OPENED_ADD:
			// not in depot yet, file is removed without source control
			RevertKeepFile(fullPath);
			return false;
		case P4_FILE_OPENED_EDIT:
			if (!RevertKeepFile(fullPath))
				return false;
			break;
		case P4_FILE_SYNCED:
			break;
		default:
			return false;
	}

	P4Result result;
	if (!RunChangelistCommand("delete", P4QuoteArg(fullPath), result))
	{
		P4PrintMessages(result);
		MsgWarning("p4: can not delete '%s'\n", fullPath.ToCString());
		return false;
	}

	SetFileInfo(fullPath, P4_FILE_OPENED_DELETE, EnsureChangelist(GetCurrentChangelistDesc()));
	DevMsg(DEVMSG_FS, "p4: deleted '%s'\n", fullPath.ToCString());

	// 'p4 delete' removes the workspace file, let the caller remove it otherwise
	return P4GetFileAccess(fullPath) == -1;
}

bool CSourceControlP4::RenameFile(EqStringRef oldFullPath, EqStringRef newFullPath)
{
	CScopedMutex m(m_mutex);
	if (!EnsureConnected() || !IsUnderClientRoot(oldFullPath) || !IsUnderClientRoot(newFullPath))
		return false;

	P4FileInfo info = GetFileInfo(oldFullPath);
	switch (info.state)
	{
		case P4_FILE_SYNCED:
			// 'p4 move' needs the file opened for edit
			if (!OpenForEdit(oldFullPath, info, SC_FILETYPE_AUTO))
				return false;
			break;
		case P4_FILE_OPENED_EDIT:
		case P4_FILE_OPENED_ADD:
			break;
		default:
			// renamed without source control, the new file is added by the caller
			CancelScopeFile(oldFullPath);
			return false;
	}

	P4Result result;
	const EqString fileArgs = EqString::Format("%s %s", P4QuoteArg(oldFullPath).ToCString(), P4QuoteArg(newFullPath).ToCString());
	if (!RunChangelistCommand("move", fileArgs, result))
	{
		P4PrintMessages(result);
		MsgWarning("p4: can not move '%s' to '%s'\n", oldFullPath.ToCString(), newFullPath.ToCString());
		return false;
	}

	const int change = EnsureChangelist(GetCurrentChangelistDesc());
	SetFileInfo(oldFullPath, P4_FILE_OPENED_DELETE, change);
	SetFileInfo(newFullPath, P4_FILE_OPENED_ADD, change);
	DevMsg(DEVMSG_FS, "p4: moved '%s' to '%s'\n", oldFullPath.ToCString(), newFullPath.ToCString());

	return true;
}

void CSourceControlP4::ResetCache()
{
	CScopedMutex m(m_mutex);
	m_fileInfos.clear();
	m_changelists.clear();
	m_knownDirs.clear();
}

void CSourceControlP4::PrintStatus() const
{
	CScopedMutex m(m_mutex);

	Msg("Source control: p4%s\n", m_failed ? " (disabled)" : (m_connected ? "" : " (not connected yet)"));
	if (m_connected)
	{
		Msg("  client: %s, user: %s\n", m_clientName.ToCString(), m_userName.ToCString());
		Msg("  client root: %s\n", m_clientRoot.Length() ? m_clientRoot.ToCString() : "(any)");
	}
	Msg("  default changelist: '%s'\n", m_defaultChangelistDesc.ToCString());
	Msg("  executable: %s\n", m_executable.Length() ? m_executable.ToCString() : "(not resolved yet)");
	Msg("  add new files: %s\n", m_addNewFiles ? "yes" : "no");
	Msg("  cached files: %d, changelists: %d, checked out directories: %d\n", m_fileInfos.size(), m_changelists.size(), m_knownDirs.numElem());
	for (const P4KnownDir& dir : m_knownDirs)
		Msg("    %s\n", dir.path.ToCString());
}
