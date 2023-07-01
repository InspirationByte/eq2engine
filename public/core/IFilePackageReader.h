#pragma once

class IVirtualStream;
using IVirtualStreamPtr = CRefPtr<IVirtualStream>;

using IFile = IVirtualStream; // pretty same
using IFilePtr = IVirtualStreamPtr; // pretty same

class IFilePackageReader
{
public:
	virtual ~IFilePackageReader() = default;

	virtual IFilePtr	Open(const char* filename, int openFlags) = 0;
	virtual bool		FileExists(const char* filename) const = 0;
};