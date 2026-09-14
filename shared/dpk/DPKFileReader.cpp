///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include <lz4.h>

#include "core/core_common.h"
#include "core/platform/OSFile.h"
#include "core/IFileSystem.h"
#include "DPKFileReader.h"
#include "DPKUtils.h"

// HACK: current and previous versions of DPKFileWriter had serious bug that allowed buffer overflow.
//		 This was fixed but extra 1024 bytes are kept for compatibility with those broken community-made EPK files 
//		 Consider removing this hack as soon as EPK version changes.
constexpr int DPK_BLOCK_DECOMPRESS_SIZE = DPK_BLOCK_MAXSIZE + 1024;

static Threading::CEqMutex s_dpkMutex;

struct CDPKFileStream::BlockInfo : dpkblock_t
{
	uint64 offset;
};

CDPKFileStream::CDPKFileStream(const char* filename, const DPKFileHdr& info, COSFile& osFile)
	: m_name(filename)
	, m_ice(0)
	, m_osFile(osFile)
	, m_info(info)
{
	bool hasCompressedBlocks = false;

	m_blockInfo.setNum(m_info.numBlocks);
	int64 startOffset = m_info.offset;
	for (auto [i, blockInfo] : arrayEnumerate(m_blockInfo))
	{
		const int result = m_osFile.ReadWithOffset(&blockInfo, sizeof(dpkblock_t), startOffset);
		ASSERT_MSG(result >= 0, "DPK offsets broke");

		// data offset
		blockInfo.offset = startOffset + sizeof(dpkblock_t);

		const bool compressed = (blockInfo.flags & DPKFILE_FLAG_COMPRESSED);
		hasCompressedBlocks |= compressed;

		ASSERT(blockInfo.size <= DPK_BLOCK_DECOMPRESS_SIZE);
		ASSERT(blockInfo.compressedSize <= DPK_BLOCK_DECOMPRESS_SIZE);

		startOffset += sizeof(dpkblock_t) + (compressed ? blockInfo.compressedSize : blockInfo.size);
	}

	m_blockData = PPAlloc(DPK_BLOCK_DECOMPRESS_SIZE);// s_dpkDataTls.blockData;
	m_tmpDecompressData = hasCompressedBlocks ? PPAlloc(DPK_BLOCK_DECOMPRESS_SIZE)/*s_dpkDataTls.tmpDecompressData*/ : nullptr;
}

CDPKFileStream::~CDPKFileStream()
{
	PPFree(m_blockData);
	PPFree(m_tmpDecompressData);
}

CBasePackageReader* CDPKFileStream::GetHostPackage() const
{ 
	return (CBasePackageReader*)m_host;
}

bool CDPKFileStream::DecodeBlock(int blockIdx)
{
	if (m_curBlockIdx == blockIdx)
		return true;

	const BlockInfo& newBlock = m_blockInfo[blockIdx];
	const bool encrypted = (newBlock.flags & DPKFILE_FLAG_ENCRYPTED);
	const bool compressed = (newBlock.flags & DPKFILE_FLAG_COMPRESSED);

	const int readSize = compressed ? newBlock.compressedSize : newBlock.size;
	ubyte* readMem = compressed ? (ubyte*)m_tmpDecompressData : (ubyte*)m_blockData;

	// read block data and decompress/decrypt if needed
	const int readBytes = m_osFile.ReadWithOffset(readMem, readSize, newBlock.offset);
	if (readBytes == -1)
		return false;

	ASSERT(readBytes == readSize);

	// decrypt first as it was encrypted last
	if (encrypted)
	{
		const int iceBlockSize = m_ice.blockSize();

		ubyte* iceTempBlock = (ubyte*)stackalloc(iceBlockSize);
		ubyte* tmpBlockPtr = readMem;

		int bytesLeft = readSize;

		// encrypt block by block
		while (bytesLeft > iceBlockSize)
		{
			m_ice.decrypt(tmpBlockPtr, iceTempBlock);

			// copy decrypted block
			memcpy(tmpBlockPtr, iceTempBlock, iceBlockSize);

			tmpBlockPtr += iceBlockSize;
			bytesLeft -= iceBlockSize;
		}
	}

	// then decompress
	if (compressed)
	{
		// decompress readMem
		const int decompressedSize = LZ4_decompress_safe((char*)readMem, (char*)m_blockData, newBlock.compressedSize, DPK_BLOCK_DECOMPRESS_SIZE);
		ASSERT_MSG(decompressedSize == newBlock.size, "unable to decompress DPK block %d (compressedSize: %d, decompressedSize: %d, blockSize: %d)", 
			blockIdx, newBlock.compressedSize, decompressedSize, newBlock.size);
	}

	m_curBlockIdx = blockIdx;

	return true;
}

// reads data from virtual stream
VSSize CDPKFileStream::Read(void* dest, VSSize count, VSSize size)
{
	const int fileRemainingBytes = m_info.size - m_curPos;
	const int bytesToRead = min(static_cast<int>(count * size), fileRemainingBytes);

	if (bytesToRead <= 0)
		return 0;

	// read blocks if any
	if (m_info.numBlocks)
	{
		int bytesToReadCnt = bytesToRead;
		ubyte* destBuf = (ubyte*)dest;

		int curPos = m_curPos;

		// in case if user requested data more that one block size
		do
		{
			// decode block
			const int blockOffset = curPos % DPK_BLOCK_MAXSIZE;
			const int curBlockIdx = curPos / DPK_BLOCK_MAXSIZE;
			const bool decoded = DecodeBlock(curBlockIdx);
			ASSERT_MSG(decoded, "DPK DecodeBlock fail");

			const int blockRemainingBytes = m_blockInfo[curBlockIdx].size - blockOffset;
			const int blockBytesToRead = min(bytesToReadCnt, blockRemainingBytes);

			// read the data from block
			memcpy(destBuf, (ubyte*)m_blockData + blockOffset, blockBytesToRead);

			destBuf += blockBytesToRead;
			curPos += blockBytesToRead;

			bytesToReadCnt -= blockBytesToRead;

		} while (bytesToReadCnt > 0);

		m_curPos = curPos;
	}
	else
	{
		// read file straight
		const int64 readBytes = m_osFile.ReadWithOffset(dest, bytesToRead, m_info.offset + m_curPos);
		m_curPos += readBytes;
		return static_cast<VSSize>(readBytes / size);
	}

	return static_cast<VSSize>(bytesToRead / size);
}

// writes data to virtual stream
VSSize CDPKFileStream::Write(const void *src, VSSize count, VSSize size)
{
	ASSERT_FAIL("CDPKFileStream does not support WRITE OPS");
	return 0;
}

// seeks pointer to position
VSSize CDPKFileStream::Seek(int64 nOffset, EFileStreamSeek seekType)
{
	int newOfs = m_curPos;
	switch (seekType)
	{
		case FS_SEEK_SET:
		{
			newOfs = static_cast<int>(nOffset);
			break;
		}
		case FS_SEEK_CUR:
		{
			newOfs += static_cast<int>(nOffset);
			break;
		}
		case FS_SEEK_END:
		{
			newOfs = m_info.size + static_cast<int>(nOffset);
			break;
		}
	}

	if (newOfs < 0)
	{
		m_curPos = 0;
		return -1;
	}

	if (static_cast<uint32>(newOfs) > m_info.size)
	{
		m_curPos = m_info.size;
		return -1;
	}

	m_curPos = newOfs;

	return static_cast<VSSize>(m_curPos);
}

// returns current pointer position
VSSize CDPKFileStream::Tell() const
{
	return static_cast<VSSize>(m_curPos);
}

// returns memory allocated for this stream
VSSize CDPKFileStream::GetSize()
{
	return static_cast<VSSize>(m_info.size);
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

bool CDPKFileReader::FileExist(const char* filename) const
{
	return FindFileIndex(filename) != -1;
}

int	CDPKFileReader::FindFileIndex(const char* filename) const
{
	const int nameHash = DPK_FilenameHash(filename, m_version);
	auto it = m_fileIndices.find(nameHash);
	if (!it.atEnd())
		return it.value();

    return -1;
}

bool CDPKFileReader::CheckValidHeader(const dpkheader_t& header, const char* packageName)
{
	if (header.signature != DPK_SIGNATURE)
	{
		MsgError("'%s' is not a Data Pack File\n", packageName);
		return false;
	}

	if (header.version != DPK_VERSION && header.version != DPK_PREV_VERSION)
	{
		MsgError("package '%s' has wrong version\n", packageName);
		return false;
	}
	return true;
}

bool CDPKFileReader::InitPackage(const char *filename, const char* name, const char* mountPath /*= nullptr*/)
{
	m_packagePath.Empty();

	if(!m_osFile.Open(filename, COSFile::OPEN_EXIST | COSFile::READ | COSFile::WITH_OFFSET))
		return false;

	m_name = name ? name : filename;
	m_packagePath = filename;

	return InitPackageInternal(0, mountPath);
}

bool CDPKFileReader::InitPackageInternal(const VSSize startOffset, const char* mountPath /*= nullptr*/)
{
	dpkheader_t header{};
	m_osFile.ReadWithOffset(&header, sizeof(dpkheader_t), startOffset);
	if (!CheckValidHeader(header, m_packagePath))
		return false;

	// read mount path
	char dpkMountPath[DPK_STRING_SIZE]{};
	m_osFile.ReadWithOffset(dpkMountPath, DPK_STRING_SIZE, startOffset + sizeof(dpkheader_t));

	m_version = header.version;
	m_mountPath = mountPath ? mountPath : dpkMountPath;

	fnmPathFixSeparators(m_mountPath);

	DevMsg(DEVMSG_FS, "Package '%s' loading OK\n", m_packagePath.ToCString());

	// read file table
	Array<dpkfileinfo_t> fileInfos(PP_SL);
	fileInfos.setNum(header.numFiles);

	const int result = m_osFile.ReadWithOffset(fileInfos.ptr(), sizeof(dpkfileinfo_t) * header.numFiles, startOffset + header.fileInfoOffset);
	ASSERT_MSG(result >= 0, "DPK offsets broke");

	m_dpkFiles.setNum(header.numFiles);
	for (auto [i, dstInfo] : arrayEnumerate(m_dpkFiles))
	{
		const dpkfileinfo_t& info = fileInfos[i];

		m_fileIndices.insert(info.filenameHash, i);
		dstInfo.offset = startOffset + info.offset;	// relocate package in case of opening EPK inside EPK
		dstInfo.size = info.size;
		dstInfo.crc = info.crc;
		dstInfo.numBlocks = info.numBlocks;
		dstInfo.flags = info.flags;
	}

	// ASSERT_MSG(header.numFiles == m_fileIndices.size(), "Programmer warning: hash collisions in %s, %d files out of %d", m_packageName.ToCString(), m_fileIndices.size(), header.numFiles);

	return true;
}

bool CDPKFileReader::OpenEmbeddedPackage(CBasePackageReader* target, const char* filename)
{
	// find file in DPK filename list
	const int dpkFileIndex = FindFileIndex(filename);

	if (dpkFileIndex == -1)
		return false;

	const DPKFileHdr& fileInfo = m_dpkFiles[dpkFileIndex];

	// file must be flat-written in order to be able to read as package
	if (fileInfo.flags & (DPKFILE_FLAG_COMPRESSED | DPKFILE_FLAG_ENCRYPTED))
		return false;

	// we don't support ZIP files yet
	// though it should not be a problem to have them
	if (target->GetType() == PACKAGE_READER_DPK)
	{
		CDPKFileReader* targetDPKReader = (CDPKFileReader*)target;
		targetDPKReader->m_name = filename;
		targetDPKReader->m_packagePath = m_packagePath;

		COSFile& osFile = targetDPKReader->m_osFile;
		if (!osFile.Open(m_packagePath, COSFile::OPEN_EXIST | COSFile::READ | COSFile::WITH_OFFSET))
		{
			ASSERT_FAIL("CDPKFileReader::OpenEmbeddedPackage FATAL ERROR - failed to open package file");
			return false;
		}

		targetDPKReader->InitPackageInternal(fileInfo.offset, nullptr);
		return true;
	}

	return false;
}

IFileStreamPtr CDPKFileReader::Open(const char* filename, int modeFlags)
{
	if (modeFlags & (COSFile::APPEND | COSFile::WRITE))
	{
		ASSERT_FAIL("Archived files only can open for reading!\n");
		return nullptr;
	}

	// find file in DPK filename list
	const int dpkFileIndex = FindFileIndex(filename);
	if (dpkFileIndex == -1)
		return nullptr;

	const DPKFileHdr& fileInfo = m_dpkFiles[dpkFileIndex];

	CRefPtr<CDPKFileStream> newStream = CRefPtr_new(CDPKFileStream, filename, fileInfo, m_osFile);
	newStream->m_host = this;
	newStream->m_ice.set((unsigned char*)m_key.ToCString());

	return IFileStreamPtr(newStream);
}

IFileStreamPtr CDPKFileReader::Open(int fileIndex, int modeFlags)
{
	if (modeFlags & (COSFile::APPEND | COSFile::WRITE))
	{
		ASSERT_FAIL("Archived files only can open for reading!\n");
		return nullptr;
	}

	if (fileIndex == -1)
		return nullptr;

	const DPKFileHdr& fileInfo = m_dpkFiles[fileIndex];

	CRefPtr<CDPKFileStream> newStream = CRefPtr_new(CDPKFileStream, EqString::Format("dpkFile%d", fileIndex), fileInfo, m_osFile);
	newStream->m_host = this;
	newStream->m_ice.set((unsigned char*)m_key.ToCString());
	return IFileStreamPtr(newStream);
}