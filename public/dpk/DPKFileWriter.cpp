///////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include <lz4hc.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "DPKFileWriter.h"

//---------------------------------------------

CDPKFileWriter::CDPKFileWriter(const char* mountPath, int compression, const char* encryptKey)
	: m_ice(0)
{
	memset(m_mountPath, 0, sizeof(m_mountPath));

	strncpy(m_mountPath, mountPath, DPK_STRING_SIZE);
	m_mountPath[DPK_STRING_SIZE - 1] = 0;
	FixSlashes(m_mountPath);
	xstrlwr(m_mountPath);

	m_compressionLevel = compression;
	if(encryptKey && *encryptKey)
	{
		if (strlen(encryptKey) == m_ice.keySize())
		{
			m_ice.set((const unsigned char*)encryptKey);
		}
		else
		{
			MsgError("CDPKFileWriter error - encryptKey size must be %d but only got %d", m_ice.keySize(), strlen(encryptKey));
		}
	}
}

CDPKFileWriter::~CDPKFileWriter()
{
	ASSERT(m_output.IsOpen() == false);
}

bool CDPKFileWriter::Begin(const char* fileName)
{
	ASSERT(m_output.IsOpen() == false);
	if (m_output.IsOpen())
		return false;

	if (!m_output.Open(g_fileSystem->GetAbsolutePath(SP_ROOT, fileName), COSFile::WRITE))
	{
		MsgError("Cannot create '%s'!\n", fileName);
		return false;
	}

	memset(&m_header, 0, sizeof(m_header));
	m_header.version = DPK_VERSION;
	m_header.signature = DPK_SIGNATURE;
	m_header.fileInfoOffset = 0;
	m_header.compressionLevel = m_compressionLevel;
	m_header.numFiles = m_files.numElem();

	m_output.Write(&m_header, sizeof(m_header));
	m_output.Write(m_mountPath, DPK_STRING_SIZE);

	return true;
}

void CDPKFileWriter::Flush()
{
	if (!m_output.IsOpen())
		return;

	m_output.Flush();
}

int CDPKFileWriter::End()
{
	if (!m_output.IsOpen())
	{
		ASSERT(m_output.IsOpen() == true);
		return 0;
	}

	m_header.fileInfoOffset = m_output.Tell();
	m_header.numFiles = m_files.numElem();

	// overwrite as we have updated it
	m_output.Seek(0, COSFile::ESeekPos::SET);
	m_output.Write(&m_header, sizeof(m_header));
	m_output.Seek(m_header.fileInfoOffset, COSFile::ESeekPos::SET);

	// write file infos
	m_output.Write(m_files.ptr(), m_files.numElem() * sizeof(dpkfileinfo_t));
	m_output.Close();

	const int numFiles = m_files.numElem();
	m_files.clear(true);

	return numFiles;
}

uint CDPKFileWriter::Add(IVirtualStream* fileData, const int nameHash, bool skipCompression)
{
	dpkfileinfo_t& pakInfo = m_files.append();

	// set the size and offset in the file bigfile
	pakInfo.filenameHash = nameHash;
	pakInfo.offset = m_output.Tell();
	pakInfo.size = fileData->GetSize();
	pakInfo.crc = fileData->GetCRC32();

	const bool doCompression = (m_compressionLevel > 0) && !skipCompression;

	// compressed and encrypted files has to be put into blocks
	// uncompressed files are bypassing blocks
	if (skipCompression && !m_encrypted)
	{
		Array<ubyte*> tmpBuffer(PP_SL);
		tmpBuffer.resize(pakInfo.size);
		fileData->Read(tmpBuffer.ptr(), 1, pakInfo.size);

		m_output.Write(tmpBuffer.ptr(), pakInfo.size);
		return pakInfo.size;
	}

	uint packedSize = 0;
	pakInfo.numBlocks = 0;

	// temporary block for both compression and encryption 
	// twice the size
	ubyte readBlockData[DPK_BLOCK_MAXSIZE];
	ubyte tmpBlockData[DPK_BLOCK_MAXSIZE];

	// write blocks
	dpkblock_t blockInfo;
	while (true)
	{
		memset(&blockInfo, 0, sizeof(dpkblock_t));

		// get block offset
		const int srcOffset = (int)pakInfo.numBlocks * DPK_BLOCK_MAXSIZE;
		const int srcSize = min(DPK_BLOCK_MAXSIZE, ((int)pakInfo.size - srcOffset));

		if (srcSize <= 0)
			break; // EOF

		blockInfo.size = srcSize;
		fileData->Read(readBlockData, 1, srcSize);

		int compressedSize = -1;

		// try compressing
		if (doCompression)
		{
			memset(tmpBlockData, 0, sizeof(tmpBlockData));
			compressedSize = LZ4_compress_HC((const char*)readBlockData, (char*)tmpBlockData, srcSize, sizeof(tmpBlockData), m_compressionLevel);
		}

		// compressedSize could be -1 which means buffer overlow (or uneffective)
		if (compressedSize > 0)
		{
			blockInfo.flags |= DPKFILE_FLAG_COMPRESSED;
			blockInfo.compressedSize = compressedSize;
			packedSize += compressedSize;
		}
		else
		{
			memcpy(tmpBlockData, readBlockData, srcSize);
			packedSize += srcSize;
		}

		const int tmpBlockSize = (blockInfo.flags & DPKFILE_FLAG_COMPRESSED) ? blockInfo.compressedSize : srcSize;

		// encrypt tmpBlock
		if (m_encrypted)
		{
			blockInfo.flags |= DPKFILE_FLAG_ENCRYPTED;

			const int iceBlockSize = m_ice.blockSize();

			ubyte* iceTempBlock = (ubyte*)stackalloc(iceBlockSize);
			ubyte* tmpBlockPtr = tmpBlockData;

			int bytesLeft = tmpBlockSize;

			// encrypt block by block
			while (bytesLeft > iceBlockSize)
			{
				m_ice.encrypt(tmpBlockPtr, iceTempBlock);

				// copy encrypted block
				memcpy(tmpBlockPtr, iceTempBlock, iceBlockSize);

				tmpBlockPtr += iceBlockSize;
				bytesLeft -= iceBlockSize;
			}
		}

		// write header and data
		m_output.Write(&blockInfo, sizeof(blockInfo));
		m_output.Write(tmpBlockData, tmpBlockSize);

		++pakInfo.numBlocks;

		// small block size indicates last block
		if (srcSize < DPK_BLOCK_MAXSIZE)
			break;
	}

	return packedSize;
}