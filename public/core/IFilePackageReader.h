#pragma once

class IVirtualStream;
using IFile = IVirtualStream; // pretty same

class IFilePackageReader
{
public:
	virtual ~IFilePackageReader() = default;

	virtual IFile*	Open(const char* filename, int openFlags) = 0;
	virtual void	Close(IFile* fp) = 0;
	virtual bool	FileExists(const char* filename) const = 0;
};