//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include "DPKFileWriter.h"

#include <zlib.h>

#include "utils/strtools.h"
#include "core/DebugInterface.h"
#include "core/cmd_pacifier.h"

#pragma todo("linux code")

CDPKFileWriter::CDPKFileWriter()
{
	m_file = NULL;
	memset(&m_header, 0, sizeof(m_header));
	memset( m_key, 0, sizeof(m_key) );
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

	if(key)
	{
		memset( m_key, 0, sizeof(m_key));

		int len = strlen(key);

		if(len > sizeof(m_key))
		{
			MsgWarning("Too long key, truncated from %d to %d characters\n", len, sizeof(m_key));
			len = sizeof(m_key);
		}

		memcpy( m_key, key, len );
	}
}

bool CDPKFileWriter::BuildAndSave( const char* fileNamePrefix )
{
	m_header.version = DPK_VERSION;
	m_header.signature = DPK_SIGNATURE;
	m_header.fileInfoOffset = 0;
	m_header.compressionLevel = m_compressionLevel;
	m_header.numFiles = 0;

	EqString fileName = fileNamePrefix;

	FILE* dpkFile = fopen( fileName.c_str(), "wb" );

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
	strncpy(m_mountPath, path, DPK_MAX_FILENAME_LENGTH);
	FixSlashes(m_mountPath);
	strlwr( m_mountPath );
}

bool FileExists( const char* szPath )
{
#ifdef _WIN32
	DWORD dwAttrib = GetFileAttributes(szPath);

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
		strncpy(newInfo->filename, fileName, DPK_MAX_FILENAME_LENGTH);
		FixSlashes(newInfo->filename);
		strlwr( newInfo->filename );

		char fillMountedFilename[DPK_MAX_FILENAME_LENGTH];
		strcpy(fillMountedFilename, m_mountPath);

		char correctPathSep[2] = {CORRECT_PATH_SEPARATOR, '\0'};

		int len = strlen(fillMountedFilename);

		if( len > 0 && fillMountedFilename[len-1] != CORRECT_PATH_SEPARATOR )
			strcat(fillMountedFilename, correctPathSep);

		strcat(fillMountedFilename, newInfo->filename);

		//Msg("Add file '%s'\n", fillMountedFilename);

		newInfo->pkinfo.filenameHash = StringToHash( fillMountedFilename, true );
	}

	m_files.append(newInfo);
	m_header.numFiles++;

	return true;
}

void CDPKFileWriter::AddDirecory(const char* filename_to_add, bool bRecurse)
{
	EqString dir_name(filename_to_add);
	EqString dirname(dir_name + "/*.*");

	Msg("Adding path '%s' to package:\n",filename_to_add);

#ifdef _WIN32
	WIN32_FIND_DATAA wfd;
	HANDLE hFile;

	hFile = FindFirstFileA(dirname.c_str(), &wfd);
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
					AddDirecory( varargs("%s/%s",dir_name.c_str(),wfd.cFileName), true );
			}
			else
			{
				EqString filename(dir_name + "/" + wfd.cFileName);

				AddFile(filename.c_str());
			}
		}

		FindClose(hFile);
	}
#endif // _WIN32
}

#define DPK_WRITE_BLOCK (8*1024*1024)

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
	m_header.fileInfoOffset = sizeof(dpkheader_t);

	for(int i = 0; i < m_files.numElem();i++)
	{
		UpdatePacifier((float)i / (float)m_files.numElem());

		dpkfilewinfo_t* fileInfo = m_files[i];
		dpkfileinfo_t* pkInfo = &m_files[i]->pkinfo;

		long filesize = 0;
		ubyte* _filedata = LoadFileBuffer( fileInfo->filename, &filesize );

		fileInfo->size = filesize;

		if( _filedata )
		{
			int status = Z_DATA_ERROR;

			if( m_compressionLevel > 0 )
			{
				unsigned long compressed_size = filesize + 128;
				ubyte* _compressedData = (ubyte*)malloc(compressed_size);

				memset(_compressedData, 0, compressed_size);

				status = compress2(_compressedData, &compressed_size, (ubyte*)_filedata,filesize, m_compressionLevel);

				if( status == Z_OK )
				{
					/*
					if( m_encryption == 2 )
					{
						ubyte* encData = NULL;
						compressed_size = encryptBlock(&encData, _compressedData, compressed_size, m_key);

						free(_compressedData);

						_compressedData = encData;

						pkInfo->flags |= DPKFILE_FLAG_ENCRYPTED;
					}*/

					// compress & kill =)
					fwrite(_compressedData, compressed_size, 1, dpk_temp_data);

					pkInfo->offset = m_header.fileInfoOffset;
					pkInfo->size = filesize;
					pkInfo->compressedSize = compressed_size;

					// set compression flag
					pkInfo->flags |= DPKFILE_FLAG_COMPRESSED;

					m_header.fileInfoOffset += compressed_size;
					
					// free loaded file
					free( _compressedData );
					free( _filedata );

					// flush our results always
					fflush(dpk_temp_data);

					continue;
				}
			}

			long fileEncSize = filesize;

			/*
			if( m_encryption == 2 )
			{
				ubyte* encData = NULL;
				fileEncSize = encryptBlock(&encData, _filedata, filesize, m_key);

				free(_filedata);

				_filedata = encData;

				pkInfo->flags |= DPKFILE_FLAG_ENCRYPTED;
			}*/

			// write & kill =)
			fwrite(_filedata, fileEncSize, 1, dpk_temp_data);

			pkInfo->offset = m_header.fileInfoOffset;
			pkInfo->size = filesize;
			pkInfo->compressedSize = fileEncSize;

			// not compressed, ignore flags
			m_header.fileInfoOffset += fileEncSize;

			free( _filedata );

			// flush our results always
			fflush(dpk_temp_data);
		}
		else
		{
			MsgError("Failed to open file '%s' while adding to archive\n", fileInfo->filename);
		}
	}

	fclose(dpk_temp_data);

	EndPacifier();
	Msg("OK\n");

	m_header.numFiles = m_files.numElem(); 

	// Write header
	fwrite(&m_header,sizeof(m_header),1,m_file);

	// Write temp file to main file
	WriteFiles();

	// Write file infos
	for(int i = 0; i < m_files.numElem();i++)
	{
		fwrite( &m_files[i]->pkinfo, sizeof(dpkfileinfo_t), 1, m_file );
		delete m_files[i];
	}

	m_files.clear();

	Msg("Total files written: %d\n", m_header.numFiles);

	remove("fcompress_temp.tmp");

	return true;
}
