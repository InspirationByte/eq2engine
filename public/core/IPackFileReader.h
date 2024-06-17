#pragma once

class IVirtualStream;
using IVirtualStreamPtr = CRefPtr<IVirtualStream>;

using IFile = IVirtualStream; // pretty same
using IFilePtr = IVirtualStreamPtr; // pretty same

class IPackFileReader : public RefCountedObject<IPackFileReader>
{
public:
	virtual ~IPackFileReader() = default;

	virtual const char* GetName() const = 0;

	virtual IFilePtr	Open(const char* filename, int openFlags) = 0;
	virtual IFilePtr	Open(int fileIndex, int openFlags) = 0;

	virtual bool		FileExists(const char* filename) const = 0;
	virtual int			FindFileIndex(const char* filename) const = 0;
};

using IPackFileReaderPtr = CRefPtr<IPackFileReader>;