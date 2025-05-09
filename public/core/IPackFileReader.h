#pragma once

class IFileStream;
using IFileStreamPtr = CRefPtr<IFileStream>;

using IFileStream = IFileStream; // pretty same
using IFileStreamPtr = IFileStreamPtr; // pretty same

class IPackFileReader : public RefCountedObject<IPackFileReader>
{
public:
	virtual ~IPackFileReader() = default;

	virtual const char* GetName() const = 0;

	virtual IFileStreamPtr	Open(const char* filename, int openFlags) = 0;
	virtual IFileStreamPtr	Open(int fileIndex, int openFlags) = 0;

	virtual bool		FileExist(const char* filename) const = 0;
	virtual int			FindFileIndex(const char* filename) const = 0;
};

using IPackFileReaderPtr = CRefPtr<IPackFileReader>;