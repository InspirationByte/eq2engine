///////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include <lz4.h>

#include "core/core_common.h"
#include "FileSystem.h"
#include "DPKFileReader.h"

static Threading::CEqMutex s_dpkMutex;

// Fixes slashes in the directory name
static void DPK_RebuildFilePath(const char* str, char* newstr)
{
	char* pnewstr = newstr;
	char cprev = 0;

	while (*str)
	{
		while (cprev == *str && (cprev == CORRECT_PATH_SEPARATOR || cprev == INCORRECT_PATH_SEPARATOR))
			str++;

		*pnewstr = *str;
		pnewstr++;

		cprev = *str;
		str++;
	}

	*pnewstr = 0;
}

void DPK_FixSlashes(EqString& str)
{
	char* tempStr = (char*)stackalloc(str.Length() + 1);
	memset(tempStr, 0, str.Length());

	DPK_RebuildFilePath(str.ToCString(), tempStr);

	char* ptr = tempStr;
	while (*ptr)
	{
		if (*ptr == '\\')
			*ptr = '/';

		ptr++;
	}

	str.Assign(tempStr);
}

void encryptDecrypt(ubyte* buffer, int size, int hash)
{
	char* key = (char*)&hash;

	while (size--)
		buffer[size] = buffer[size] ^ key[size % 4];
}

//-----------------------------------------------------------------------------------------------------------------------

CDPKFileStream::CDPKFileStream(const char* filename, const dpkfileinfo_t& info, COSFile&& osFile)
	: m_ice(0), m_osFile(std::move(osFile))
{
	//m_dbgFilename = filename;
	m_info = info;
	m_curPos = 0;

	m_curBlockIdx = -1;

	bool hasCompressedBlocks = false;

	// read all block headers
	m_osFile.Seek(m_info.offset, COSFile::ESeekPos::SET);

	m_blockInfo.resize(m_info.numBlocks);
	for (int i = 0; i < m_info.numBlocks; i++)
	{
		dpkblock_t hdr;
		m_osFile.Read(&hdr, sizeof(dpkblock_t));
		
		dpkblock_info_t& block = m_blockInfo.append();
		block.flags = hdr.flags;
		block.offset = m_osFile.Tell();
		block.compressedSize = hdr.compressedSize;
		block.size = hdr.size;

		// skip block contents
		const int readSize = (block.flags & DPKFILE_FLAG_COMPRESSED) ? hdr.compressedSize : hdr.size;
		m_osFile.Seek(readSize, COSFile::ESeekPos::CURRENT);

		hasCompressedBlocks = hasCompressedBlocks || (block.flags & DPKFILE_FLAG_COMPRESSED);
	}

	m_blockData = malloc(DPK_BLOCK_MAXSIZE);
	m_tmpDecompressData = hasCompressedBlocks ? malloc(DPK_BLOCK_MAXSIZE + 128) : nullptr;
}


CDPKFileStream::~CDPKFileStream()
{
	free(m_blockData);
	free(m_tmpDecompressData);
}

CBasePackageReader* CDPKFileStream::GetHostPackage() const
{ 
	return (CBasePackageReader*)m_host;
}

void CDPKFileStream::DecodeBlock(int blockIdx)
{
	if (m_curBlockIdx == blockIdx)
		return;

	m_curBlockIdx = blockIdx;

	dpkblock_info_t& curBlock = m_blockInfo[blockIdx];
			
	m_osFile.Seek(curBlock.offset, COSFile::ESeekPos::SET);

	const int readSize = (curBlock.flags & DPKFILE_FLAG_COMPRESSED) ? curBlock.compressedSize : curBlock.size;

	// FIXME: rework it so we don't have to malloc all the time as it affects performance
	ubyte* readMem = (curBlock.flags & DPKFILE_FLAG_COMPRESSED) ? (ubyte*)m_tmpDecompressData : (ubyte*)m_blockData;

	// read block data and decompress/decrypt if needed
	m_osFile.Read(readMem, readSize);

	// decrypt first as it was encrypted last
	if (curBlock.flags & DPKFILE_FLAG_ENCRYPTED)
	{
		int iceBlockSize = m_ice.blockSize();

		ubyte* iceTempBlock = (ubyte*)stackalloc(iceBlockSize);
		ubyte* tmpBlockPtr = readMem;

		int bytesLeft = readSize;

		// encrypt block by block
		while (bytesLeft > iceBlockSize)
		{
			m_ice.decrypt(tmpBlockPtr, iceTempBlock);

			// copy encrypted block
			memcpy(tmpBlockPtr, iceTempBlock, iceBlockSize);

			tmpBlockPtr += iceBlockSize;
			bytesLeft -= iceBlockSize;
		}
	}

	// then decompress
	if (curBlock.flags & DPKFILE_FLAG_COMPRESSED)
	{
		// decompress readMem to 'm_blockData'
		unsigned long destLen = LZ4_decompress_safe((const char*)readMem, (char*)m_blockData, curBlock.compressedSize, DPK_BLOCK_MAXSIZE);
		ASSERT(destLen == curBlock.size);
	}
}

// reads data from virtual stream
size_t CDPKFileStream::Read(void* dest, size_t count, size_t size)
{
	const size_t fileRemainingBytes = m_info.size - m_curPos;
	const int bytesToRead = min(count * size, fileRemainingBytes);

	if (bytesToRead == 0)
		return 0;

	// read blocks if any
	if (m_info.numBlocks)
	{
		//Msg("READ for %u from blocks: %d of %d\n", m_info.filenameHash, bytesToRead, m_info.size);

		int bytesToReadCnt = bytesToRead;
		ubyte* destBuf = (ubyte*)dest;

		int curPos = m_curPos;

		// in case if user requested data more that one block size
		do
		{
			// decode block
			const int blockOffset = curPos % DPK_BLOCK_MAXSIZE;
			const int curBlockIdx = curPos / DPK_BLOCK_MAXSIZE;
			DecodeBlock(curBlockIdx);

			const int blockRemainingBytes = m_blockInfo[curBlockIdx].size - blockOffset;
			const int blockBytesToRead = min(bytesToReadCnt, blockRemainingBytes);

			//Msg("Block %d: read at %d (%d) - %d of %d\n", curBlockIdx, curPos, blockOffset, blockBytesToRead, m_blockInfo.size);

			// read the data from block
			memcpy(destBuf, (ubyte*)m_blockData + blockOffset, blockBytesToRead);

			destBuf += blockBytesToRead;
			curPos += blockBytesToRead;

			bytesToReadCnt -= blockBytesToRead;
			
		} while (bytesToReadCnt);

		m_curPos = curPos;

		return bytesToRead / size;
	}
	else
	{
		m_osFile.Seek(m_info.offset + m_curPos, COSFile::ESeekPos::SET);

		// read file straight
		m_osFile.Read(dest, bytesToRead);

		m_curPos += bytesToRead;

		// return number of read elements
		return bytesToRead / size;
	}
}

// writes data to virtual stream
size_t CDPKFileStream::Write(const void *src, size_t count, size_t size)
{
	ASSERT_FAIL("CDPKFileStream does not support WRITE OPS");
	return 0;
}

// seeks pointer to position
int	CDPKFileStream::Seek(long nOffset, EVirtStreamSeek seekType)
{
	int newOfs = m_curPos;

	switch (seekType)
	{
		case VS_SEEK_SET:
		{
			newOfs = nOffset;
			break;
		}
		case VS_SEEK_CUR:
		{
			newOfs += nOffset;
			break;
		}
		case VS_SEEK_END:
		{
			newOfs = m_info.size + nOffset;
			break;
		}
	}


	if (newOfs < 0)
	{
		m_curPos = 0;
		return -1;
	}

	if (newOfs > m_info.size)
	{
		m_curPos = m_info.size;
		return -1;
	}

	m_curPos = newOfs;
	return 0;
}

// fprintf analog
void CDPKFileStream::Print(const char* fmt, ...)
{
	ASSERT_FAIL("CDPKFileStream does not support WRITE OPS");
}

// returns current pointer position
long CDPKFileStream::Tell() const
{
	return m_curPos;
}

// returns memory allocated for this stream
long CDPKFileStream::GetSize()
{
	return m_info.size;
}

// flushes stream from memory
bool CDPKFileStream::Flush()
{
	return false;
}

// returns CRC32 checksum of stream
uint32 CDPKFileStream::GetCRC32()
{
	return m_info.crc;
}

//-----------------------------------------------------------------------------------------------------------------------
// DPK host
//-----------------------------------------------------------------------------------------------------------------------

CDPKFileReader::CDPKFileReader()
{
    m_searchPath = 0;
	m_dpkFiles = nullptr;

	memset(&m_header, 0, sizeof(m_header));
}

CDPKFileReader::~CDPKFileReader()
{
	for(int i = 0; i < m_openFiles.numElem(); i++)
	{
		Close( m_openFiles[0] );
	}

	delete [] m_dpkFiles;
}

bool CDPKFileReader::FileExists(const char* filename) const
{
	return FindFileIndex(filename) != -1;
}

int	CDPKFileReader::FindFileIndex(const char* filename) const
{
	EqString fullFilename(filename);
	fullFilename = fullFilename.LowerCase();
	fullFilename.Path_FixSlashes();

	// check if mount path is not a root path
	// or it does not exist
	const int mountPathPos = fullFilename.Find(m_mountPath.ToCString());
	if (mountPathPos > 0 || mountPathPos == -1)
	{
		return -1;
	}

	// convert to DPK filename
	EqString pkgFileName = fullFilename.Right(fullFilename.Length() - m_mountPath.Length() - 1);
	DPK_FixSlashes(pkgFileName);

	const int nameHash = StringToHash(pkgFileName.ToCString(), true);

	auto it = m_fileIndices.find(nameHash);
	if (it != m_fileIndices.end())
	{
		return it.value();
	}

    return -1;
}

bool CDPKFileReader::InitPackage(const char *filename, const char* mountPath /*= nullptr*/)
{
	SAFE_DELETE_ARRAY(m_dpkFiles);

    m_packageName = filename;
	m_packagePath = ((CFileSystem*)g_fileSystem.GetInstancePtr())->GetAbsolutePath(SP_ROOT, filename);

    FILE* dpkFile = fopen(m_packagePath.ToCString(),"rb");

    // Now fill the header data and create object table
    if (!dpkFile)
    {
		MsgError("Cannot open package '%s'\n", m_packagePath.ToCString());
		return false;
	}

	fread(&m_header,sizeof(dpkheader_t),1,dpkFile);

    if (m_header.signature != DPK_SIGNATURE)
    {
		MsgError("'%s' is not a package!!!\n", m_packageName.ToCString());

		fclose(dpkFile);
        return false;
    }

    if (m_header.version != DPK_VERSION)
    {
		MsgError("package '%s' has wrong version!!!\n", m_packageName.ToCString());

		fclose(dpkFile);
        return false;
    }

	// read mount path
	char dpkMountPath[DPK_STRING_SIZE];
	fread(dpkMountPath, DPK_STRING_SIZE, 1, dpkFile);

	// if custom mount path provided, use it
	if(mountPath)
		m_mountPath = mountPath;
	else
		m_mountPath = dpkMountPath;
	
	m_mountPath.Path_FixSlashes();

	DevMsg(DEVMSG_FS, "Package '%s' loading OK\n", m_packageName.ToCString());

    // Let set the file info data origin
    fseek(dpkFile, m_header.fileInfoOffset, SEEK_SET);

	m_dpkFiles = PPNew dpkfileinfo_t[m_header.numFiles];
	fread( m_dpkFiles, sizeof(dpkfileinfo_t), m_header.numFiles, dpkFile );

	for (int i = 0; i < m_header.numFiles; i++)
	{
		m_fileIndices.insert(m_dpkFiles[i].filenameHash, i);
	}

    fclose(dpkFile);

    return true;
}

IVirtualStream* CDPKFileReader::Open(const char* filename, int modeFlags)
{
	//Msg("io::dpktryopen(%s)\n", filename);

	if( m_header.numFiles == 0 )
	{
		MsgError("Package is not open!\n");
		return nullptr;
	}

	if (modeFlags & (COSFile::APPEND | COSFile::WRITE))
	{
		ASSERT_FAIL("Archived files only can open for reading!\n");
		return nullptr;
	}

	// find file in DPK filename list
	int dpkFileIndex = FindFileIndex(filename);

	if (dpkFileIndex == -1)
		return nullptr;

	dpkfileinfo_t& fileInfo = m_dpkFiles[dpkFileIndex];

	COSFile osFile;
	if (!osFile.Open(m_packagePath.ToCString(), COSFile::OPEN_EXIST | COSFile::READ))
	{
		ASSERT_FAIL("CDPKFileReader::Open FATAL ERROR - failed to open package file");
		return nullptr;
	}

	CDPKFileStream* newStream = PPNew CDPKFileStream(filename, fileInfo, std::move(osFile));
	newStream->m_host = this;
	newStream->m_ice.set((unsigned char*)m_key.ToCString());

	{
		Threading::CScopedMutex m(s_dpkMutex);
		m_openFiles.append(newStream);
	}

	return newStream;
}

void CDPKFileReader::Close(IVirtualStream* fp)
{
    if (!fp)
        return;

	CDPKFileStream* fsp = (CDPKFileStream*)fp;

	{
		Threading::CScopedMutex m(s_dpkMutex);
		if(!m_openFiles.fastRemove(fsp))
			return;
	}

	delete fsp;
}