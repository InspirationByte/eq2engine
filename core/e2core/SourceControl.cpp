//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: File system source control provider selection and changelist scopes
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"
#include "core/ConCommand.h"
#include "utils/KeyValues.h"
#include "FileSystem.h"
#include "SourceControlP4.h"
#include "SourceControl.h"

DECLARE_CMD(fs_sc_status, "Prints file system source control status", CV_UNREGISTERED)
{
	CFileSystem* fileSystem = static_cast<CFileSystem*>(g_fileSystem.GetInstancePtr());
	ISourceControl* sourceControl = fileSystem->GetSourceControl();
	if (!sourceControl)
	{
		Msg("Source control is not enabled\n");
		return;
	}
	sourceControl->PrintStatus();
}

DECLARE_CMD(fs_sc_reset, "Drops cached source control file states", CV_UNREGISTERED)
{
	CFileSystem* fileSystem = static_cast<CFileSystem*>(g_fileSystem.GetInstancePtr());
	ISourceControl* sourceControl = fileSystem->GetSourceControl();
	if (sourceControl)
		sourceControl->ResetCache();
}

//-------------------------------------------------------------------------------------------------

struct SCChangelistScope
{
	EqString		desc;
	Array<EqString>	pendingAdds{ PP_SL };		// SC_FILETYPE_AUTO
	Array<EqString>	pendingBinaryAdds{ PP_SL };	// SC_FILETYPE_BINARY
	Array<EqString>	pendingRemoves{ PP_SL };
	Array<EqString>	pendingReverts{ PP_SL };
	int				mergedDepth{ 0 };			// scopes with the same description pushed into this one

	Array<EqString>& GetPendingAdds(ESourceControlFileType fileType)
	{
		return (fileType == SC_FILETYPE_BINARY) ? pendingBinaryAdds : pendingAdds;
	}
};

// changelist scopes are per thread so that loader threads do not write into a scope of another thread
static thread_local Array<SCChangelistScope> s_changelistScopes{ PP_SL };

void ISourceControl::PushChangelist(EqStringRef description)
{
	EqString desc = ExpandDesc(description);

	// work split into parts that open the same scope inside an outer one is sent by the outer scope
	if (s_changelistScopes.numElem() && !s_changelistScopes.back().desc.CompareCaseIns(desc))
	{
		++s_changelistScopes.back().mergedDepth;
		return;
	}

	SCChangelistScope& scope = s_changelistScopes.append();
	scope.desc = std::move(desc);
}

void ISourceControl::PopChangelist()
{
	ASSERT_MSG(s_changelistScopes.numElem(), "ISourceControl::PopChangelist - no changelist scope");
	if (!s_changelistScopes.numElem())
		return;

	SCChangelistScope& scope = s_changelistScopes.back();
	if (scope.mergedDepth > 0)
	{
		--scope.mergedDepth;
		return;
	}

	// the scope stays current while its files are added
	if (scope.pendingAdds.numElem())
		AddFiles(scope.pendingAdds, SC_FILETYPE_AUTO);

	if (scope.pendingBinaryAdds.numElem())
		AddFiles(scope.pendingBinaryAdds, SC_FILETYPE_BINARY);

	if (scope.pendingRemoves.numElem())
		RemoveFiles(scope.pendingRemoves);

	if (scope.pendingReverts.numElem())
		RevertUnchangedFiles(scope.pendingReverts);

	s_changelistScopes.popBack();

	// release memory so thread exit does not leave allocations behind
	if (!s_changelistScopes.numElem())
		s_changelistScopes.clear(true);
}

EqStringRef ISourceControl::GetScopeChangelistDesc()
{
	if (!s_changelistScopes.numElem())
		return EqStringRef();
	return s_changelistScopes.back().desc;
}

static void SCAppendUniquePath(Array<EqString>& paths, EqStringRef fullPath)
{
	for (const EqString& path : paths)
	{
		if (!path.CompareCaseIns(fullPath))
			return;
	}
	paths.append(fullPath);
}

static void SCRemovePath(Array<EqString>& paths, EqStringRef fullPath)
{
	for (int i = 0; i < paths.numElem(); ++i)
	{
		if (paths[i].CompareCaseIns(fullPath))
			continue;

		paths.removeIndex(i);
		break;
	}
}

bool ISourceControl::DeferScopeAdd(EqStringRef fullPath, ESourceControlFileType fileType)
{
	if (!s_changelistScopes.numElem())
		return false;

	// the last requested file type wins
	SCChangelistScope& scope = s_changelistScopes.back();
	SCRemovePath(scope.pendingAdds, fullPath);
	SCRemovePath(scope.pendingBinaryAdds, fullPath);
	SCAppendUniquePath(scope.GetPendingAdds(fileType), fullPath);
	return true;
}

void ISourceControl::DeferScopeRevertUnchanged(EqStringRef fullPath)
{
	if (!s_changelistScopes.numElem())
		return;

	SCAppendUniquePath(s_changelistScopes.back().pendingReverts, fullPath);
}

bool ISourceControl::DeferScopeRemove(EqStringRef fullPath)
{
	if (!s_changelistScopes.numElem())
		return false;

	SCAppendUniquePath(s_changelistScopes.back().pendingRemoves, fullPath);
	return true;
}

void ISourceControl::CancelScopeFile(EqStringRef fullPath)
{
	if (!s_changelistScopes.numElem())
		return;

	SCChangelistScope& scope = s_changelistScopes.back();
	SCRemovePath(scope.pendingAdds, fullPath);
	SCRemovePath(scope.pendingBinaryAdds, fullPath);
	SCRemovePath(scope.pendingRemoves, fullPath);
	SCRemovePath(scope.pendingReverts, fullPath);
}

void ISourceControl::CancelScopeRemove(EqStringRef fullPath)
{
	if (!s_changelistScopes.numElem())
		return;

	SCRemovePath(s_changelistScopes.back().pendingRemoves, fullPath);
}

void ISourceControl::SetStatus(ESourceControlStatus status, const char* error)
{
	if (Atomic::Exchange(m_status, static_cast<int>(status)) == status && !error)
		return;

	if (m_statusCallback)
		m_statusCallback(status, error);
}

EqString ISourceControl::ExpandDesc(EqStringRef description)
{
	EqString desc(description);

	int found = 0;
	do {
		found = desc.ReplaceSubstr("$APP_NAME$", g_eqCore->GetApplicationName(), false, found);
	} while (found != -1);

	return desc;
}

//-------------------------------------------------------------------------------------------------

static bool IsSourceControlApplication(const KVSection* scConfig)
{
	const KVSection* appsSec = scConfig ? scConfig->FindSection("Applications") : nullptr;

	// enabled for every application unless the list is given
	if (!appsSec)
		return true;

	const EqStringRef appName = g_eqCore->GetApplicationName();
	for (EqStringRef name : appsSec->Values<EqStringRef>())
	{
		if (!name.CompareCaseIns(appName))
			return true;
	}

	return false;
}

ISourceControlPtr ISourceControl::Create(const KVSection& coreConfig, const KVSection& fsConfig, EqStringRef workDir)
{
	EqStringRef providerName;
	if (!fsConfig.Get("UseSourceControl").GetValues(providerName) || !providerName.Length())
		return nullptr;

	if (g_cmdLine->Find("-nosourcecontrol") != -1)
	{
		MsgInfo("* Source control: disabled by -nosourcecontrol\n");
		return nullptr;
	}

	// 'SourceControl <name> { ... }' section of the provider
	const KVSection* scConfig = nullptr;
	for (const KVSection& section : coreConfig.Keys("SourceControl"))
	{
		EqStringRef name;
		section.GetValues(name);
		if (!name.CompareCaseIns(providerName))
		{
			scConfig = &section;
			break;
		}
	}

	if (!IsSourceControlApplication(scConfig))
		return nullptr;

	ISourceControlPtr sourceControl;
	if (!providerName.CompareCaseIns("p4"))
	{
		EqStringRef changelistDesc = "+auto $APP_NAME$";
		EqStringRef executable;
		bool addNewFiles = true;
		int connectTimeout = 5;
		int commandTimeout = 15;
		if (scConfig)
		{
			scConfig->Get("DefaultChangelistDesc").GetValues(changelistDesc);
			scConfig->Get("AddNewFiles").GetValues(addNewFiles);
			scConfig->Get("Executable").GetValues(executable);
			scConfig->Get("ConnectTimeout").GetValues(connectTimeout);
			scConfig->Get("CommandTimeout").GetValues(commandTimeout);
		}

		sourceControl = ISourceControlPtr(CRefPtr_new(CSourceControlP4, workDir, executable, ExpandDesc(changelistDesc), addNewFiles,
			max(connectTimeout, 1), max(commandTimeout, 1)));
	}
	else
	{
		MsgError("Unknown source control provider '%s'\n", providerName.ToCString());
		return nullptr;
	}

	MsgInfo("* Source control: %s\n", sourceControl->GetName());

	if (!fs_sc_status.IsRegistered())
		ConCommandBase::Register(&fs_sc_status);

	if (!fs_sc_reset.IsRegistered())
		ConCommandBase::Register(&fs_sc_reset);

	return sourceControl;
}
