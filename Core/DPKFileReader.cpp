///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include "DPKFileReader.h"
#include "platform/Platform.h"

#include "math/math_common.h"

#ifdef _WIN32
#define ZLIB_WINAPI
#endif // _WIN32

#include <zlib.h>

#include "utils/strtools.h"
#include "DebugInterface.h"

#include "IVirtualStream.h"
#include "utils/CRC32.h"

#ifdef _WIN32
#include <direct.h>
#include <shlobj.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

void encryptDecrypt(ubyte* buffer, int size, int hash)
{
	char* key = (char*)&hash;

	while (size--)
		buffer[size] = buffer[size] ^ key[size % 4];
}

//-----------------------------------------------------------------------------------------------------------------------

CDPKFileStream::CDPKFileStream(const dpkfileinfo_t& info, FILE* fp)
{
	m_handle = fp;
	m_info = info;
	m_curPos = 0;
	memset(&m_blockInfo, 0, sizeof(m_blockInfo));

	// decode first block automatically
	if (!m_info.numBlocks)
		fseek(m_handle, m_info.offset, SEEK_SET);
}


CDPKFileStream::~CDPKFileStream()
{
}

void CDPKFileStream::DecodeBlock(int blockIdx)
{
	int curBlockIdx = (float(m_curPos) / float(m_info.size)) * m_info.numBlocks;

	if (curBlockIdx == blockIdx && m_blockInfo.size != 0)
		return;

	ubyte tmpBlock[DPK_BLOCK_MAXSIZE];

	// find the block
	fseek(m_handle, m_info.offset, SEEK_SET);

	// seek to the block
	for(int i = 0; i < m_info.numBlocks; i++)
	{
		dpkblock_t blockHdr;
		fread(&blockHdr, 1, sizeof(blockHdr), m_handle);

		int readSize = (blockHdr.flags & DPKFILE_FLAG_COMPRESSED) ? blockHdr.compressedSize : blockHdr.size;

		Msg("DecodeBlock %d size = %d\n", i, readSize);

		if (i == blockIdx)
		{
			ubyte* readMem = (blockHdr.flags & DPKFILE_FLAG_COMPRESSED) ? tmpBlock : m_blockData;

			// read block and decompress/decrypt
			fread(readMem, 1, readSize, m_handle);

			// decrypt first as it was encrypted last
			if (blockHdr.flags & DPKFILE_FLAG_ENCRYPTED)
			{
				// decrypt
				encryptDecrypt(readMem, readSize, m_info.filenameHash);
			}

			// then decompress
			if (blockHdr.flags & DPKFILE_FLAG_COMPRESSED)
			{
				// so our readMem is already 'tmpBlock', so we only have to uncompress it to 'm_blockData'
				
				unsigned long destLen = blockHdr.size;
				int status = uncompress(m_blockData, &destLen, tmpBlock, blockHdr.compressedSize);

				if (status != Z_OK)
					ASSERTMSG(false, varargs("Cannot decompress file block - %d!\n", status));
				else
					ASSERT(destLen == blockHdr.size);
			}

			// done. Keep block for further purposes
			m_blockInfo = blockHdr;
			return;
		}

		// skip
		fseek(m_handle, readSize, SEEK_CUR);
	}
}

// reads data from virtual stream
size_t CDPKFileStream::Read(void* dest, size_t count, size_t size)
{
	const int fileRemainingBytes = m_info.size - m_curPos;
	const int bytesToRead = min(count*size, fileRemainingBytes);

	// read blocks if any
	if (m_info.numBlocks)
	{
		Msg("READ for %u from blocks: %d of %d\n", m_info.filenameHash, bytesToRead, m_info.size);

		int bytesToReadCnt = bytesToRead;
		ubyte* destBuf = (ubyte*)dest;

		int curPos = m_curPos;

		// in case if user requested data more that one block size
		do
		{
			// decode block
			int curBlockIdx = (float(curPos) / float(m_info.size)) * m_info.numBlocks;	// FIXME: DO NOT USE FLOATING POINT NUMBERS!
			DecodeBlock(curBlockIdx);

			// after decode set position to satisfy conditions
			m_curPos = curPos;

			// calculate offsed within the block
			const int blockOffset = curPos % DPK_BLOCK_MAXSIZE;

			const int blockRemainingBytes = m_blockInfo.size - blockOffset;
			const int blockBytesToRead = min(bytesToRead, blockRemainingBytes);

			Msg("Block %d: read at %d (%d) - %d of %d\n", curBlockIdx, m_curPos, blockOffset, blockBytesToRead, m_blockInfo.size);

			// read the data from block
			memcpy(destBuf, m_blockData+blockOffset, blockBytesToRead);

			destBuf += blockBytesToRead;
			curPos += blockBytesToRead;

			bytesToReadCnt -= blockBytesToRead;
			
		} while (bytesToReadCnt);

		m_curPos = curPos;

		return bytesToRead / size;
	}
	else
	{
		// read file straight
		fread(dest, 1, bytesToRead, m_handle);

		// return number of read elements
		return bytesToRead / size;
	}
}

// writes data to virtual stream
size_t CDPKFileStream::Write(const void *src, size_t count, size_t size)
{
	ASSERTMSG(false, "CDPKFileStream does not support WRITE OPS");
	return 0;
}

// seeks pointer to position
int	CDPKFileStream::Seek(long nOffset, VirtStreamSeek_e seekType)
{
	uint32 startOffset = m_info.offset;
	uint32 newOfs = m_curPos;

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
		return -1;

	// set the virtual offset
	m_curPos = newOfs;

	Msg("DPK %u seek => %d / %d\n", m_info.filenameHash, newOfs, m_info.size);

	ASSERTMSG(newOfs <= m_info.size, varargs("CDPKFileStream::Seek - %u illegal seek => %d while file max is %d\n", m_info.filenameHash, newOfs, m_info.size));

	if(m_info.numBlocks)
		return 0;

	return fseek(m_handle, startOffset + newOfs, SEEK_SET);
}

// fprintf analog
void CDPKFileStream::Print(const char* fmt, ...)
{
	ASSERTMSG(false, "CDPKFileStream does not support WRITE OPS");
}

// returns current pointer position
long CDPKFileStream::Tell()
{
	return m_curPos;
}

// returns memory allocated for this stream
long CDPKFileStream::GetSize()
{
	return m_info.size;
}

// flushes stream from memory
int	CDPKFileStream::Flush()
{
	// do nothing...
	return 0;
}

// returns stream type
VirtStreamType_e CDPKFileStream::GetType()
{
	return VS_TYPE_FILE_PACKAGE;
}

// returns CRC32 checksum of stream
uint32 CDPKFileStream::GetCRC32()
{
	long pos = Tell();
	long fSize = GetSize();

	ubyte* pFileData = (ubyte*)malloc(fSize + 16);

	Read(pFileData, 1, fSize);

	Seek(pos, VS_SEEK_SET);

	uint32 nCRC = CRC32_BlockChecksum(pFileData, fSize);

	free(pFileData);

	return nCRC;
}

//-----------------------------------------------------------------------------------------------------------------------
// DPK file reader code
//-----------------------------------------------------------------------------------------------------------------------

CDPKFileReader::CDPKFileReader(Threading::CEqMutex& mutex) : m_FSMutex(mutex)
{
    m_searchPath = 0;
	m_dpkFiles = nullptr;

	memset(m_key, 0, sizeof(m_key));
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

void CDPKFileReader::SetKey(const char* key)
{
	memset( m_key, 0, sizeof(m_key));

	int len = strlen(key);

	if(len > sizeof(m_key))
		len = sizeof(m_key);

	memcpy( m_key, key, len );
}

char* CDPKFileReader::GetKey() const
{
	return (char*)m_key;
}

// Fixes slashes in the directory name
static void RebuildFilePath( char *str, char *newstr )
{
	char* pnewstr = newstr;
	char cprev = 0;

    while ( *str )
    {
		while(cprev == *str && (cprev == CORRECT_PATH_SEPARATOR))
			str++;

		*pnewstr = *str;
		pnewstr++;

		cprev = *str;
        str++;
    }

	*pnewstr = 0;
}

int	CDPKFileReader::FindFileIndex(const char* filename) const
{
	char* pszNewString = (char*)stackalloc(strlen(filename)+1);
	RebuildFilePath((char*)filename, pszNewString);
	FixSlashes(pszNewString);
	xstrlwr(pszNewString);

	int strHash = StringToHash(pszNewString, true);

	Msg("DPK: %s = %u\n", pszNewString, strHash);

    for (int i = 0; i < m_header.numFiles; i++)
    {
		const dpkfileinfo_t& file = m_dpkFiles[i];

		if( file.filenameHash == strHash )
			return i;
    }

    return -1;
}

int CDPKFileReader::GetSearchPath() const
{
    return m_searchPath;
}

void CDPKFileReader::SetSearchPath(int search)
{
    m_searchPath = search;
}

bool CDPKFileReader::SetPackageFilename(const char *filename)
{
	delete [] m_dpkFiles;

    m_packageName = filename;
	m_tempPath = getenv("TEMP") + _Es("/_pktmp_") + _Es(filename).Path_Strip_Ext();

    FILE* dpkFile = fopen(m_packageName.c_str(),"rb");

    // Now fill the header data and create object table
    if (!dpkFile)
    {
		ErrorMsg(varargs("Cannot open package '%s'\n", m_packageName.c_str()));
		return false;
	}

	fread(&m_header,sizeof(dpkheader_t),1,dpkFile);

    if (m_header.signature != DPK_SIGNATURE)
    {
		MsgError("'%s' is not a package!!!\n", m_packageName.c_str());

		fclose(dpkFile);
        return false;
    }

    if (m_header.version != DPK_VERSION)
    {
		MsgError("package '%s' has wrong version!!!\n", m_packageName.c_str());

		fclose(dpkFile);
        return false;
    }

	DevMsg(DEVMSG_FS, "Package '%s' loading OK\n", m_packageName.c_str());

    // Let set the file info data origin
    fseek(dpkFile, m_header.fileInfoOffset, SEEK_SET);

	m_dpkFiles = new dpkfileinfo_t[m_header.numFiles];
	fread( m_dpkFiles, sizeof(dpkfileinfo_t), m_header.numFiles, dpkFile );

    fclose(dpkFile);

    return true;
}

const char* CDPKFileReader::GetPackageFilename() const
{
	return m_packageName.c_str();
}

void CDPKFileReader::DumpPackage(PACKAGE_DUMP_MODE mode)
{
    if (!m_header.numFiles == 0)
    {
        Msg("Before doing dumping you need to set package name!\n");
        return;
    }

#if 0

	 PACKAGE DUMP DISABLED
    if (mode == PACKAGE_INFO)
    {
        for (int i = 0; i < m_hDPKFileInfos.numElem();i++)
        {
            Msg("File: %s\n",m_hDPKFileInfos[i]->m_pszFileName);
            Msg("  Compressed size: %d\n",m_hDPKFileInfos[i]->m_iFileCompressedSize);
            Msg("  Data start: %d\n",m_hDPKFileInfos[i]->m_iFileStart);
            Msg("  Size: %d\n",m_hDPKFileInfos[i]->m_iFileSize);
        }

        Msg("Total files count: %i\n",m_pHdr->m_iFileCount);
    }



    else if (mode == PACKAGE_FILES)
    {
        Msg("Unpacking '%s'\n", m_szPackageName.GetData());

        for (int i = 0; i < m_hDPKFileInfos.numElem();i++)
        {
            FILE *dpkData = fopen(m_szPackageName.GetData(),"rb");
            fseek(dpkData,m_hDPKFileInfos[i]->m_iFileStart,SEEK_SET);

            int hierarchy_size = UTIL_GetNumSlashes(m_hDPKFileInfos[i]->m_pszFileName);
            for (int j = 1;j < hierarchy_size;j++)
            {
                EqString dirName = UTIL_GetDirectoryNameEx(m_hDPKFileInfos[i]->m_pszFileName,j);
#ifdef _WIN32
                mkdir(dirName.GetData());
#else
                mkdir(dirName.GetData(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
            }

            if (!(m_hDPKFileInfos[i]->m_nFlags & DPK_FLAG_COMPRESSED))
            {
                // Simply write and continue
                unsigned long size = m_hDPKFileInfos[i]->m_iFileCompressedSize;
                unsigned char* _data = (unsigned char* )PPAlloc(size);

                // Read compressed data from package
                fread(_data,size,1,dpkData);

                fclose(dpkData);

                FILE *writefile = fopen(m_hDPKFileInfos[i]->m_pszFileName,"wb");
                if (writefile)
                {
                    Msg("Writing file '%s'\n",m_hDPKFileInfos[i]->m_pszFileName);
                    fwrite(_data,size,1,dpkData);
                    fclose(writefile);
                }

                PPFree(_data);

                continue;
            }

            unsigned long compressed_size = m_hDPKFileInfos[i]->m_iFileCompressedSize;
            unsigned char* _compressedData = (unsigned char*)PPAlloc(compressed_size);

            // Read compressed data from package
            fread(_compressedData,compressed_size,1,dpkData);

            fclose(dpkData);

            unsigned long realSize = m_hDPKFileInfos[i]->m_iFileSize + 250;
            unsigned char* _UncompressedData = (unsigned char*)PPAlloc(realSize);

            int status = uncompress(_UncompressedData,&realSize,_compressedData,compressed_size);

			PPFree(_compressedData);

            if (status == Z_OK)
            {
                FILE *writefile = fopen(m_hDPKFileInfos[i]->m_pszFileName,"wb");
                if (writefile)
                {
                    Msg("Writing file '%s'\n",m_hDPKFileInfos[i]->m_pszFileName);

                    fwrite(_UncompressedData,realSize,1,dpkData);
                    fclose(writefile);
                }
            }
            else
            {
                Msg("Cannot decompress file (%d)!\n",status);
            }

            PPFree(_UncompressedData);
        }

    }
#endif // 0
}

CDPKFileStream* CDPKFileReader::Open(const char* filename,const char* mode)
{
	//Msg("io::dpktryopen(%s)\n", filename);

	if( m_header.numFiles == 0 )
	{
		MsgError("Package is not open!\n");
		return NULL;
	}

	// check for write access
	if(strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'))
	{
		MsgError("DPK only can open for reading!\n");
		return nullptr;
	}

	// find file in DPK filename list
	int dpkFileIndex = FindFileIndex(filename);

	if (dpkFileIndex == -1)
		return nullptr;

	dpkfileinfo_t& fileInfo = m_dpkFiles[dpkFileIndex];

	FILE* file = fopen(m_packageName.c_str(), mode);

	if (!file)
	{
		ASSERTMSG(false, "CDPKFileReader::Open FATAL ERROR - failed to open package file");
		return nullptr;
	}

	CDPKFileStream* newStream = new CDPKFileStream(fileInfo, file);
	newStream->m_host = this;

	{
		Threading::CScopedMutex m(m_FSMutex);
		m_openFiles.append(newStream);
	}

	return newStream;
}

void CDPKFileReader::Close(CDPKFileStream* fp)
{
    if (!fp)
        return;

	Threading::CScopedMutex m(m_FSMutex);

    if(m_openFiles.fastRemove( fp ))
	{
		fclose(fp->m_handle);
		delete fp;
	}
}