///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include <lz4hc.h>

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/cmd_pacifier.h"

#include "utils/KeyValues.h"

#include "DPKFileWriter.h"

#define DPK_WRITE_BLOCK (8*1024*1024)

// Fixes slashes in the directory name
static void DPK_RebuildFilePath(const char *str, char *newstr)
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

static void DPK_FixSlashes(EqString& str)
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

static bool FileExists(const char* szPath)
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesA(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
	struct stat st;
	if (stat(szPath, &st) == 0)
		return !(st.st_mode & S_IFDIR);

	return false;
#endif // _WIN32
}


static ubyte* LoadFileBuffer(const char* filename, long* fileSize)
{
	FILE* file = fopen(filename, "rb");

	if (!file)
	{
		MsgError("Can't open file '%s'\n", filename);
		return nullptr;
	}

	fseek(file, 0, SEEK_END);
	long filelen = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (!filelen)
		return nullptr;

	// allocate file in the hunk
	ubyte* fileBuf = (ubyte*)PPAlloc(filelen);

	fread(fileBuf, 1, filelen, file);
	fclose(file);

	*fileSize = filelen;

	return fileBuf;
}

//---------------------------------------------

CDPKFileWriter::CDPKFileWriter() 
	: m_ice(0)
{
	m_file = nullptr;
	memset(&m_header, 0, sizeof(m_header));
	memset( m_mountPath, 0, sizeof(m_mountPath) );

	m_compressionLevel = 0;
	m_encryption = 0;
}

CDPKFileWriter::~CDPKFileWriter()
{
	// Write file infos
	for(int i = 0; i < m_files.numElem();i++)
		delete m_files[i];

	m_files.clear();
}

void CDPKFileWriter::SetCompression( int compression )
{
	m_compressionLevel = compression;
}

void CDPKFileWriter::SetEncryption( int type, const char* key )
{
	m_encryption = type;

	if(type && key)
		m_ice.set((const unsigned char*)key);
}

bool CDPKFileWriter::BuildAndSave( const char* fileName )
{
	if (m_files.numElem() == 0)
	{
		MsgError("No files added to package '%s'!\n", fileName);
		return false;
	}

	m_header.version = DPK_VERSION;
	m_header.signature = DPK_SIGNATURE;
	m_header.fileInfoOffset = 0;
	m_header.compressionLevel = m_compressionLevel;
	m_header.numFiles = 0;

	FILE* dpkFile = fopen(fileName, "wb" );

	if( !dpkFile )
	{
		MsgError("Cannot create '%s'!\n", fileName);
		return false;
	}

	m_file = dpkFile;

	SavePackage();

	fclose(m_file);

	return true;
}

void CDPKFileWriter::SetMountPath( const char* path )
{
	strncpy(m_mountPath, path, DPK_STRING_SIZE);
	m_mountPath[DPK_STRING_SIZE - 1] = '\0';

	FixSlashes(m_mountPath);
	xstrlwr( m_mountPath );
}

bool CDPKFileWriter::AddFile(const char* fileName, int nameHash)
{
	if (!FileExists(fileName))
	{
		MsgWarning("File '%s' does not exist, cannot add\n", fileName);
		return false;
	}

	dpkfilewinfo_t* newInfo = PPNew dpkfilewinfo_t;
	newInfo->fileName = fileName;
	memset(&newInfo->pkinfo, 0, sizeof(newInfo->pkinfo));

	newInfo->pkinfo.filenameHash = nameHash;

	m_files.append(newInfo);
	m_header.numFiles++;
	return true;
}

bool CDPKFileWriter::AddFile( const char* fileName, const char* targetFilename)
{
	EqString writeableFileName(_Es(targetFilename).LowerCase());
	DPK_FixSlashes(writeableFileName);

	return AddFile(fileName, StringToHash(writeableFileName, true));
}

int CDPKFileWriter::AddDirectory(const char* wildcard, const char* targetDir, bool recursive)
{
	EqString targetDirTrimmed(targetDir);
	targetDirTrimmed = targetDirTrimmed.TrimChar('/');
	targetDirTrimmed = targetDirTrimmed.TrimChar('.');

	EqString nonWildcardFolder(wildcard);
	const int wildcardStart = nonWildcardFolder.Find("*");
	if (wildcardStart != -1)
	{
		nonWildcardFolder = nonWildcardFolder.Left(wildcardStart).TrimChar('/');
	}

	int fileCount = 0;

#ifdef _WIN32
	WIN32_FIND_DATAA wfd;
	HANDLE hFile = FindFirstFileA(wildcard, &wfd);
	while(hFile && FindNextFileA(hFile, &wfd))
	{
		if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(!stricmp("..", wfd.cFileName) || !stricmp(".", wfd.cFileName))
				continue;

			if(recursive)
				fileCount += AddDirectory(EqString::Format("%s/%s/*", nonWildcardFolder.ToCString(), wfd.cFileName), EqString::Format("%s/%s", targetDirTrimmed.ToCString(), wfd.cFileName), true);
		}
		else
		{
			if (AddFile(EqString::Format("%s/%s", nonWildcardFolder.ToCString(), wfd.cFileName), EqString::Format("%s/%s", targetDirTrimmed.ToCString(), wfd.cFileName)))
				++fileCount;
		}
	}

	FindClose(hFile);
#else
	static_assert("need implementation of CDPKFileWriter::AddDirectory")
#endif // _WIN32

	return fileCount;
}

void CDPKFileWriter::AddIgnoreCompressionExtension( const char* extension )
{
	m_ignoreCompressionExt.append(extension);
}

void CDPKFileWriter::AddKeyValueFileExtension(const char* extension)
{
	m_keyValueFileExt.append(extension);
}

bool CDPKFileWriter::CheckCompressionIgnored(const char* extension) const
{
	for(int i = 0; i < m_ignoreCompressionExt.numElem(); i++)
	{
		if(!stricmp(m_ignoreCompressionExt[i], extension))
			return true;
	}

	return false;
}

bool CDPKFileWriter::CheckIsKeyValueFile(const char* extension) const
{
	for (int i = 0; i < m_keyValueFileExt.numElem(); i++)
	{
		if (!stricmp(m_keyValueFileExt[i], extension))
			return true;
	}

	return false;
}

bool CDPKFileWriter::WriteFiles()
{
	FILE* dpkTempDataFile = fopen("fcompress_temp.tmp", "rb");

	if(!dpkTempDataFile)
	{
		MsgError("Can't open temporary file for read!\n");
		return false;
	}

	StartPacifier("Making package file: ");

	fseek(dpkTempDataFile, 0, SEEK_END);
	long filesize = ftell( dpkTempDataFile );
	fseek(dpkTempDataFile, 0, SEEK_SET);

	ubyte* tmpBlockData = (ubyte*)PPAlloc( DPK_WRITE_BLOCK );

	long cur_pos = 0;
	while(cur_pos < filesize)
	{
		UpdatePacifier((float)cur_pos / (float)filesize);

		long bytes_to_read = DPK_WRITE_BLOCK;

		if(cur_pos+bytes_to_read > filesize)
			bytes_to_read = filesize - cur_pos;

		fread( tmpBlockData, bytes_to_read, 1, dpkTempDataFile );
		fwrite( tmpBlockData, bytes_to_read, 1, m_file );

		cur_pos += bytes_to_read;
	}

	EndPacifier();

	Msg("OK\n");

	PPFree(tmpBlockData);
	fclose(dpkTempDataFile);

	return true;
}

float CDPKFileWriter::ProcessFile(FILE* output, dpkfilewinfo_t* info)
{
	dpkfileinfo_t& fInfo = info->pkinfo;

	const EqString fileExt = info->fileName.Path_Extract_Ext();
	const bool compressionEnabled = (m_compressionLevel > 0) && !CheckCompressionIgnored(fileExt);

	long _fileSize = 0;
	ubyte* _filedata = nullptr;

	bool loadRawFile = true;
	CMemoryStream kvTempStream;

	if(CheckIsKeyValueFile(fileExt))
	{
		// TODO: convert key-values file and store it (maybe uncompressed)
		KVSection* sectionFile = KV_LoadFromFile(info->fileName, SP_ROOT);
		if (sectionFile)
		{
			kvTempStream.Open(nullptr, VS_OPEN_WRITE, 16 * 1024);

			KV_WriteToStreamBinary(&kvTempStream, sectionFile);
			_filedata = kvTempStream.GetBasePointer();
			_fileSize = kvTempStream.Tell();

			delete sectionFile;

			MsgInfo("Converted key-values file to binary: %s\n", info->fileName.ToCString());
			loadRawFile = false;
		}
	}
	
	if(loadRawFile)
	{
		_filedata = LoadFileBuffer(info->fileName, &_fileSize);
	}

	if (!_filedata)
	{
		MsgError("Failed to open file '%s' while adding to archive\n", info->fileName.ToCString());
		return 0.0f;
	}

	// set the size and offset in the file bigfile
	fInfo.size = _fileSize;
	fInfo.offset = m_header.fileInfoOffset;

	// compute CRC
	{
		fInfo.crc = CRC32_BlockChecksum(_filedata, _fileSize);
	}

	float sizeReduction = 0.0f;

	// compressed and encrypted files has to be put into blocks
	// uncompressed files are bypassing blocks
	if (compressionEnabled || m_encryption > 0)
	{
		// temporary block for both compression and encryption 
		// twice the size
		ubyte tmpBlock[DPK_BLOCK_MAXSIZE + 128];

		// allocate block headers
		dpkblock_t blockInfo;

		int numBlocks = 0;

		// write blocks
		while(true)
		{
			memset(&blockInfo, 0, sizeof(dpkblock_t));

			// get block offset
			const int srcOffset = numBlocks*DPK_BLOCK_MAXSIZE;
			const ubyte* srcBlock = _filedata+srcOffset;

			blockInfo.size = DPK_BLOCK_MAXSIZE;

			// if size is greater than remaining, we know that this is last block
			if (blockInfo.size > _fileSize - srcOffset)
				blockInfo.size = min<uint32>(blockInfo.size, (uint32)(_fileSize - srcOffset));

			if (blockInfo.size == 0)
				break;

			// compress to tmpBlock
			if(compressionEnabled)
			{
				unsigned long compressedSize = sizeof(tmpBlock);
				memset(tmpBlock, 0, compressedSize);
				
				compressedSize = LZ4_compress_HC((const char*)srcBlock, (char*)tmpBlock, blockInfo.size, compressedSize, m_compressionLevel);

				blockInfo.flags |= DPKFILE_FLAG_COMPRESSED;
				blockInfo.compressedSize = compressedSize;

				sizeReduction += compressedSize / blockInfo.size;
			}
			else
			{
				memcpy(tmpBlock, srcBlock, blockInfo.size);
			}

			int tmpBlockSize = (blockInfo.flags & DPKFILE_FLAG_COMPRESSED) ? blockInfo.compressedSize : blockInfo.size;

			// encrypt tmpBlock
			if(m_encryption > 0)
			{
				int iceBlockSize = m_ice.blockSize();

				ubyte* iceTempBlock = (ubyte*)stackalloc(iceBlockSize);
				ubyte* tmpBlockPtr = tmpBlock;

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

				blockInfo.flags |= DPKFILE_FLAG_ENCRYPTED;
			}

			// write header and data
			fwrite(&blockInfo, sizeof(blockInfo), 1, output);
			fwrite(tmpBlock, 1, tmpBlockSize, output);

			// increment file info offset in the main header
			m_header.fileInfoOffset += sizeof(blockInfo) + tmpBlockSize;

			numBlocks++;

			// small block size indicates last block
			if (blockInfo.size < DPK_BLOCK_MAXSIZE)
				break;
		}

		// set the number of blocks
		fInfo.numBlocks = numBlocks;

		if(numBlocks > 0)
			sizeReduction /= (float)numBlocks;
	}
	else
	{
		fwrite(_filedata, _fileSize, 1, output);
		m_header.fileInfoOffset += _fileSize;
	}

	// we're done with this file
	if(kvTempStream.GetBasePointer() != _filedata)
		PPFree(_filedata);

	return sizeReduction;
}

bool CDPKFileWriter::SavePackage()
{
	// create temporary file
	FILE* dpk_temp_data = fopen("fcompress_temp.tmp", "wb");
	if(!dpk_temp_data)
	{
		MsgError("Can't create temporary file for write!\n");
		Msg("Press any key to exit...");
		getchar();
		exit(0);
	}

	StartPacifier("Compressing files, this may take a while: ");

	// write fileinfo to the end of main file
	m_header.fileInfoOffset = sizeof(dpkheader_t) + DPK_STRING_SIZE;

	float compressionRatio = 0.0f;

	for(int i = 0; i < m_files.numElem();i++)
	{
		UpdatePacifier((float)i / (float)m_files.numElem());

		dpkfilewinfo_t* fileInfo = m_files[i];
		const float sizeReduction = ProcessFile(dpk_temp_data, fileInfo);

		compressionRatio += sizeReduction;
	}

	compressionRatio /= (float)m_files.numElem();

	fclose(dpk_temp_data);

	EndPacifier();
	Msg("Compression is %.2f %%\n", compressionRatio * 100.0f);

	m_header.numFiles = m_files.numElem();

	// Write header
	fwrite(&m_header,sizeof(m_header),1,m_file);

	// Write mount path
	fwrite(m_mountPath, DPK_STRING_SIZE, 1, m_file);

	// Write temp file to main file
	WriteFiles();

	// Write file infos
	for(int i = 0; i < m_files.numElem();i++)
	{
		fwrite( &m_files[i]->pkinfo, sizeof(dpkfileinfo_t), 1, m_file );
		fflush(m_file);
		delete m_files[i];
	}

	m_files.clear();

	Msg("Total files written: %d\n", m_header.numFiles);

	remove("fcompress_temp.tmp");

	return true;
}
