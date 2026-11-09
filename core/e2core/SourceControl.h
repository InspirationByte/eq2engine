//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: File system source control provider interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct KVSection;

class ISourceControl;
using ISourceControlPtr = CRefPtr<ISourceControl>;

// Source control provider used by CFileSystem to check out, add, delete and move files
// it writes. All paths are absolute full paths with platform separators.
class ISourceControl : public RefCountedObject<ISourceControl>
{
public:
	// Creates the provider selected by 'UseSourceControl' of the FileSystem config section.
	// Returns nullptr when source control is not configured or not enabled for this application.
	static ISourceControlPtr Create(const KVSection& coreConfig, const KVSection& fsConfig, EqStringRef workDir);

	// replaces '$APP_NAME$' with the application name
	static EqString		ExpandDesc(EqStringRef description);

	// Changelist scopes of the calling thread (see SourceControlScope in IFileSystem.h).
	// Files added inside a scope are added by one command when the scope is popped, checked out
	// files written inside a scope are reverted then if their contents did not change.
	void				PushChangelist(EqStringRef description);
	void				PopChangelist();

	virtual const char*	GetName() const = 0;

	// connects to the server on the first call, false when the provider failed
	// (no server, no login) and is disabled for the session
	virtual bool		IsAvailable() = 0;

	// checks out all depot files of the directory recursively and caches the states of its files,
	// so that writing many files there does not query the server per file
	virtual void		CheckoutDir(EqStringRef fullPath) = 0;

	// called before a file is opened for writing: checks out the file if it is in the depot
	virtual void		PrepareFileWrite(EqStringRef fullPath) = 0;

	// called after a file was created: adds the file if it is not in the depot
	virtual void		FinishFileWrite(EqStringRef fullPath) = 0;

	// returns true when the provider removed the file, false to remove it without source control
	virtual bool		RemoveFile(EqStringRef fullPath) = 0;

	// returns true when the provider renamed the file, false to rename it without source control
	virtual bool		RenameFile(EqStringRef oldFullPath, EqStringRef newFullPath) = 0;

	// drops cached file states (after changes made outside the application)
	virtual void		ResetCache() = 0;
	virtual void		PrintStatus() const = 0;

protected:
	// description of the innermost changelist scope of the calling thread, empty when there is none
	static EqStringRef	GetScopeChangelistDesc();

	// queues the file to be added when the innermost scope is popped, false when there is no scope
	static bool			DeferScopeAdd(EqStringRef fullPath);

	// queues the checked out file to be reverted if unchanged when the innermost scope is popped
	static void			DeferScopeRevertUnchanged(EqStringRef fullPath);

	// removes the file from the add and revert queues of the innermost scope
	static void			CancelScopeFile(EqStringRef fullPath);

	// adds files queued by a scope, called before the scope is popped
	virtual void		AddFiles(ArrayCRef<EqString> fullPaths) = 0;

	// reverts files queued by a scope whose contents match the depot, called before the scope is popped
	virtual void		RevertUnchangedFiles(ArrayCRef<EqString> fullPaths) = 0;
};
