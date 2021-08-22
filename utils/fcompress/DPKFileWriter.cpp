///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include "DPKFileWriter.h"
#include <math.h>
#include <malloc.h>

#include "utils/strtools.h"
#include "core/DebugInterface.h"
#include "core/cmd_pacifier.h"

#include <zlib.h>

#define DPK_WRITE_BLOCK (8*1024*1024)

// Fixes slashes in the directory name
void DPK_RebuildFilePath(const char *str, char *newstr)
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

#ifndef ANDROID

CDPKFileWriter::CDPKFileWriter() 
	: m_ice(0)
{
	m_file = NULL;
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

ubyte* LoadFileBuffer(const char* filename, long* fileSize)
{
	FILE* file = fopen(filename, "rb");

	if(!file)
	{
		MsgError("Can't open file '%s'\n",filename);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long filelen = ftell(file);
	fseek(file, 0, SEEK_SET);

	// allocate file in the hunk
	ubyte* fileBuf = (ubyte*)malloc(filelen);

	fread(fileBuf, 1, filelen, file);
	fclose(file);

	*fileSize = filelen;

	return fileBuf;
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

bool CDPKFileWriter::BuildAndSave( const char* fileNamePrefix )
{
	m_header.version = DPK_VERSION;
	m_header.signature = DPK_SIGNATURE;
	m_header.fileInfoOffset = 0;
	m_header.compressionLevel = m_compressionLevel;
	m_header.numFiles = 0;

	EqString fileName = fileNamePrefix;

	FILE* dpkFile = fopen( fileName.ToCString(), "wb" );

	if( !dpkFile )
	{
		MsgError("Cannot create package file!\n");
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

bool FileExists( const char* szPath )
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributesA(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif // _WIN32
}

bool CDPKFileWriter::AddFile( const char* fileName )
{
	if( !FileExists(fileName) )
	{
		MsgWarning("Cannot add file '%s' because it's don't exists\n", fileName);
		return false;
	}

	dpkfilewinfo_t* newInfo = new dpkfilewinfo_t;
	memset(newInfo,0,sizeof(dpkfilewinfo_t));

	{
		// assign the filename and fix path separators
		newInfo->fileName = _Es(fileName).LowerCase();
		DPK_FixSlashes(newInfo->fileName);

		//Msg("adding file: '%s'\n", newInfo->fileName.ToCString());

		newInfo->pkinfo.filenameHash = StringToHash(newInfo->fileName.ToCString(), true );
		//Msg("DPK: %s = %u\n", newInfo->fileName.ToCString(), newInfo->pkinfo.filenameHash);
	}

	m_files.append(newInfo);
	m_header.numFiles++;

	return true;
}

void CDPKFileWriter::AddDirectory(const char* filename_to_add, bool bRecurse)
{
	EqString dir_name(filename_to_add);
	EqString dirname(dir_name + "/*.*");

	Msg("Adding path '%s' to package:\n",filename_to_add);

#ifdef _WIN32
	WIN32_FIND_DATAA wfd;
	HANDLE hFile;

	hFile = FindFirstFileA(dirname.ToCString(), &wfd);
	if(hFile != NULL)
	{
		while(1)
		{
			if(!FindNextFileA(hFile, &wfd))
				break;

			if(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if(!stricmp("..",wfd.cFileName) || !stricmp(".",wfd.cFileName))
					continue;

				if(bRecurse)
					AddDirectory(EqString::Format("%s/%s",dir_name.ToCString(),wfd.cFileName).ToCString(), true );
			}
			else
			{
				EqString filename(dir_name + "/" + wfd.cFileName);

				AddFile(filename.ToCString());
			}
		}

		FindClose(hFile);
	}
#endif // _WIN32
}

void CDPKFileWriter::AddIgnoreCompressionExtension( const char* extension )
{
	Msg("Ignoring extension '%s' for compression\n", extension);
	m_ignoreCompressionExt.append(extension);
}

bool CDPKFileWriter::CheckCompressionIgnored(const char* extension) const
{
	for(int i = 0; i < m_ignoreCompressionExt.numElem(); i++)
	{
		if(!stricmp(m_ignoreCompressionExt[i].ToCString(), extension))
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

	ubyte* tmpBlockData = (ubyte*)malloc( DPK_WRITE_BLOCK );

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

	free(tmpBlockData);
	fclose(dpkTempDataFile);

	return true;
}

void CDPKFileWriter::ProcessFile(FILE* output, dpkfilewinfo_t* info)
{
	dpkfileinfo_t* pkInfo = &info->pkinfo;

	bool compressionEnabled = (m_compressionLevel > 0) && !CheckCompressionIgnored(info->fileName.Path_Extract_Ext().ToCString());

	long _fileSize = 0;
	ubyte* _filedata = LoadFileBuffer(info->fileName.ToCString(), &_fileSize);

	if (!_filedata)
	{
		MsgError("Failed to open file '%s' while adding to archive\n", info->fileName.ToCString());
		return;
	}

	// set the size and offset in the file bigfile
	pkInfo->size = _fileSize;
	pkInfo->offset = m_header.fileInfoOffset;

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
				blockInfo.size = min(blockInfo.size, _fileSize - srcOffset);

			int status = Z_DATA_ERROR;

			// compress to tmpBlock
			if(compressionEnabled)
			{
				unsigned long compressedSize = sizeof(tmpBlock);
				memset(tmpBlock, 0, compressedSize);
				status = compress2(tmpBlock, &compressedSize, srcBlock, blockInfo.size, m_compressionLevel);

				if (status == Z_OK)
				{
					blockInfo.flags |= DPKFILE_FLAG_COMPRESSED;
					blockInfo.compressedSize = compressedSize;
				}
				else
				{
					// failure of compression means that this block still needs to be written to temp
					// compressedSize remains 0
					memcpy(tmpBlock, srcBlock, blockInfo.size);
				}
			}
			else
				memcpy(tmpBlock, srcBlock, blockInfo.size);

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
		pkInfo->numBlocks = numBlocks;
	}
	else
	{
		fwrite(_filedata, _fileSize, 1, output);
		m_header.fileInfoOffset += _fileSize;
	}

	// we're done with this file
	free(_filedata);
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

	for(int i = 0; i < m_files.numElem();i++)
	{
		UpdatePacifier((float)i / (float)m_files.numElem());

		dpkfilewinfo_t* fileInfo = m_files[i];
		ProcessFile(dpk_temp_data, fileInfo);
	}

	fclose(dpk_temp_data);

	EndPacifier();
	Msg("OK\n");

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

#endif // !ANDROID