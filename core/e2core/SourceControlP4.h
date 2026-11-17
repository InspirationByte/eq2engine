//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Perforce source control provider (p4 command line client)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "SourceControl.h"

enum EP4FileState : int
{
	P4_FILE_UNKNOWN = 0,
	P4_FILE_IGNORED,		// outside the client root or view, or ignored by P4IGNORE
	P4_FILE_NOT_IN_DEPOT,	// can be added
	P4_FILE_SYNCED,			// in depot, not opened
	P4_FILE_OPENED_EDIT,
	P4_FILE_OPENED_ADD,
	P4_FILE_OPENED_DELETE,
};

struct P4FileInfo
{
	EP4FileState	state{ P4_FILE_UNKNOWN };
	int				change{ -1 };	// changelist the file is opened in, 0 is the default changelist
	int				generation{ 0 };	// cache generation the state was stored in
};

// directory checked out by CheckoutDir, its depot files are cached
struct P4KnownDir
{
	EqString		path;
	int				generation{ 0 };	// states stored before this generation are outdated
};

// file written, removed or renamed while offline, reconciled when the provider is online again
struct P4OfflineFile
{
	EqString		path;
	EqString		changelistDesc;
};

struct P4Field
{
	EqString	key;
	EqString	value;
};

// parsed output of 'p4 -s -ztag'
struct P4Result
{
	Array<P4Field>	fields{ PP_SL };	// tagged 'infoN: key value' lines
	Array<EqString>	messages{ PP_SL };	// 'info:', 'warning:' and 'error:' lines
	int				exitCode{ -1 };
	bool			started{ false };

	const P4Field*	FindField(EqStringRef key) const;
	bool			HasMessage(EqStringRef text) const;
	bool			HasError() const;
	bool			Succeeded() const { return started && exitCode == 0 && !HasError(); }
};

class CSourceControlP4 : public ISourceControl
{
public:
	CSourceControlP4(EqStringRef workDir, EqStringRef executable, EqStringRef defaultChangelistDesc, bool addNewFiles, int connectTimeout, int commandTimeout);
	~CSourceControlP4();

	const char*		GetName() const override { return "p4"; }
	bool			IsAvailable() override;
	ESourceControlStatus SetOnline(bool online) override;

	void			CheckoutDir(EqStringRef fullPath) override;
	void			PrepareFileWrite(EqStringRef fullPath, ESourceControlFileType fileType) override;
	void			FinishFileWrite(EqStringRef fullPath, ESourceControlFileType fileType) override;
	bool			RemoveFile(EqStringRef fullPath) override;
	bool			RenameFile(EqStringRef oldFullPath, EqStringRef newFullPath) override;

	void			ResetCache() override;
	void			PrintStatus() const override;

protected:
	void			AddFiles(ArrayCRef<EqString> fullPaths, ESourceControlFileType fileType) override;
	void			RevertUnchangedFiles(ArrayCRef<EqString> fullPaths) override;
	void			RemoveFiles(ArrayCRef<EqString> fullPaths) override;

private:
	bool			RunCommand(EqStringRef args, P4Result& result, EqStringRef input = nullptr, int maxWait = 0);
	bool			RunChangelistCommand(EqStringRef command, EqStringRef fileArgs, P4Result& result, EqStringRef input = nullptr);
	bool			RunChangelistCommand(EqStringRef desc, EqStringRef command, EqStringRef fileArgs, P4Result& result, EqStringRef input);
	bool			CheckConnectionError(const P4Result& result);

	bool			EnsureConnected();
	EqStringRef		GetCurrentChangelistDesc() const;
	int				EnsureChangelist(EqStringRef desc);
	bool			IsUnderClientRoot(EqStringRef fullPath) const;

	const P4KnownDir*	FindKnownDir(EqStringRef fullPath) const;
	P4FileInfo		GetCachedFileInfo(EqStringRef fullPath) const;
	void			SetFileInfo(EqStringRef fullPath, EP4FileState state, int change);
	P4FileInfo		QueryFileInfo(EqStringRef fullPath);
	P4FileInfo		GetFileInfo(EqStringRef fullPath);

	bool			OpenForEdit(EqStringRef fullPath, const P4FileInfo& info, ESourceControlFileType fileType);
	bool			AddFile(EqStringRef fullPath, ESourceControlFileType fileType);
	void			AddNewFile(EqStringRef fullPath, ESourceControlFileType fileType);
	bool			ReopenToCurrentChangelist(EqStringRef fullPath, const P4FileInfo& info, ESourceControlFileType fileType);
	bool			RevertKeepFile(EqStringRef fullPath);

	// stops running commands until SetOnline(true), reason is passed to the status callback
	void			GoOffline(EqStringRef reason);
	void			TrackOfflineFile(EqStringRef fullPath);
	void			PrepareOfflineFileWrite(EqStringRef fullPath);

	// operations while connected, the public ones handle the offline state after them
	void			PrepareConnectedFileWrite(EqStringRef fullPath, ESourceControlFileType fileType);
	void			FinishConnectedFileWrite(EqStringRef fullPath, ESourceControlFileType fileType);
	bool			RemoveConnectedFile(EqStringRef fullPath);
	bool			RenameConnectedFile(EqStringRef oldFullPath, EqStringRef newFullPath);
	void			ReconcileOfflineFiles();

	mutable Threading::CEqMutex	m_mutex;
	Map<int, P4FileInfo>		m_fileInfos{ PP_SL };
	Map<int, int>				m_changelists{ PP_SL };		// description hash to changelist number
	Array<P4KnownDir>			m_knownDirs{ PP_SL };
	Map<int, P4OfflineFile>		m_offlineFiles{ PP_SL };	// path hash to file
	int							m_cacheGeneration{ 0 };

	EqString		m_workDir;
	EqString		m_executable;	// found by the first command when not configured
	EqString		m_defaultChangelistDesc;
	EqString		m_clientName;
	EqString		m_clientRoot;
	EqString		m_userName;

	int				m_connectTimeout{ 5 };		// net.maxwait seconds of the connection check
	int				m_commandTimeout{ 15 };		// net.maxwait seconds of other commands

	bool			m_connected{ false };
	bool			m_offline{ false };
	bool			m_addNewFiles{ true };
};
