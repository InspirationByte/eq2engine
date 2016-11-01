///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include "DPKFileReader.h"
#include "platform/Platform.h"

#include <zlib.h>

#include "utils/strtools.h"
#include "DebugInterface.h"

#ifdef _WIN32
#include <direct.h>
#include <shlobj.h>
#include <malloc.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

CDPKFileReader::CDPKFileReader()
{
    m_searchPath = 0;
	m_dumpCount = 0;

	m_dpkFiles = NULL;

	memset( m_key, 0, sizeof(m_key) );
	memset(&m_header, 0, sizeof(m_header));
	memset(m_handles,DPK_HANDLE_INVALID,sizeof(m_handles));
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

char* CDPKFileReader::GetKey()
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

int	CDPKFileReader::FindFileIndex(const char* filename)
{
	char* pszNewString = (char*)stackalloc(strlen(filename)+1);
	RebuildFilePath((char*)filename, pszNewString);
	FixSlashes(pszNewString);
	xstrlwr(pszNewString);

	int strHash = StringToHash(pszNewString, true);

	//Msg("Requested file from package: %s\n",pszNewString);

    for (int i = 0; i < m_header.numFiles;i++)
    {
		const dpkfileinfo_t& file = m_dpkFiles[i];

		if( file.filenameHash == strHash )
			return i;
    }

    return -1;
}

int CDPKFileReader::GetSearchPath()
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
	m_tempPath = getenv("TEMP") + _Es("/_pktmp0");

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

	Msg("Package '%s' loading OK\n", m_packageName.c_str());

    // Let set the file info data origin
    fseek(dpkFile, m_header.fileInfoOffset, SEEK_SET);

	m_dpkFiles = new dpkfileinfo_t[m_header.numFiles];
	fread( m_dpkFiles, sizeof(dpkfileinfo_t), m_header.numFiles, dpkFile );

    fclose(dpkFile);

    return true;
}

const char* CDPKFileReader::GetPackageFilename()
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

dpkhandle_t CDPKFileReader::DecompressFile( int fileIndex )
{
	if(fileIndex == -1)
		return DPK_HANDLE_INVALID;

	dpkhandle_t handle = DPK_HANDLE_INVALID;
	for( int i = 0; i < DPKX_MAX_HANDLES; i++ )
	{
		if(m_handles[i] == -1)
		{
			handle = i;
			break;
		}
	}

#ifdef _WIN32
    mkdir(m_tempPath.c_str());
#else
    mkdir(m_tempPath.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

	m_handles[handle] = fileIndex;
	m_dumpCount++;

	dpkfileinfo_t& fileInfo = m_dpkFiles[fileIndex];

	FILE* dpkFile = fopen( m_packageName.c_str(), "rb" );

	if(!dpkFile)
	{
		MsgError("Failed to open package file '%s' for opening file, is something happened here?\n", m_packageName.c_str());
		return DPK_HANDLE_INVALID;
	}

    long compressed_size = fileInfo.compressedSize;
    ubyte* _compressedData = (ubyte*)malloc(compressed_size);

    // Read compressed data from package
	fseek(dpkFile, fileInfo.offset, SEEK_SET);
    fread(_compressedData, compressed_size, 1, dpkFile);
    fclose(dpkFile);
	/*
	if(fileInfo.flags & DPKFILE_FLAG_ENCRYPTED)
	{
		ubyte* encData = NULL;
		compressed_size = decryptBlock(&encData, _compressedData, compressed_size, m_key);

		//InfoMsg((char*)encData);

		free(_compressedData);

		_compressedData = encData;
	}*/

	char path[2048];
	sprintf(path, "%s/_eqtmp%i", m_tempPath.c_str(), handle);

	if(fileInfo.flags & DPKFILE_FLAG_COMPRESSED)
	{
		unsigned long realSize = fileInfo.size + 128;
		ubyte* _UncompressedData = (ubyte*)malloc(realSize);

		int status = uncompress(_UncompressedData, &realSize, _compressedData, compressed_size);

		if (status == Z_OK)
		{
			FILE* writefile = fopen(path,"wb");
			if (writefile)
			{
				fwrite( _UncompressedData, realSize, 1, writefile );
				fclose( writefile );
			}
		}
		else
		{
			MsgError("cannot decompress file '%d', if encrypted - key might be invalid!\n",status);
		}

		free(_UncompressedData);
	}
	else
	{
		FILE* writefile = fopen(path,"wb");
		if (writefile)
		{
			fwrite( _compressedData,  fileInfo.size, 1, writefile );
			fclose( writefile );
		}
	}

	// free memory
	free(_compressedData);

	return handle;
}

DPKFILE* CDPKFileReader::Open(const char* filename,const char* mode)
{
	//Msg("io::dpktryopen(%s)\n", filename);

	if( m_header.numFiles == 0 )
	{
		MsgError("Package is not open!\n");
		return NULL;
	}

    // NOTENOTE: For now we cannot write directly to DPK!
	if( !stricmp(mode,"wb") || !stricmp(mode,"a") || !stricmp(mode,"w+") || !stricmp(mode,"wt") || !stricmp(mode,"r+") )
	{
		MsgError("DPK only can open for reading!\n");
		return NULL;
	}

	int dpkFileIndex = FindFileIndex(filename);

	if (dpkFileIndex == -1)
		return NULL;

	dpkfileinfo_t& fileInfo = m_dpkFiles[dpkFileIndex];

	dpkhandle_t handle = DPK_HANDLE_INVALID;

	if(	(fileInfo.flags & DPKFILE_FLAG_COMPRESSED) ||
		(fileInfo.flags & DPKFILE_FLAG_ENCRYPTED))
	{
		//Msg("decomp/decrypt file '%s' (%d)\n", filename, dpkFileIndex);
		handle = DecompressFile(dpkFileIndex);

		if(handle == DPK_HANDLE_INVALID)
			return NULL;
	}

	DPKFILE* file = new DPKFILE;
	file->handle = DPK_HANDLE_INVALID;

	if( (fileInfo.flags & DPKFILE_FLAG_COMPRESSED) ||
		(fileInfo.flags & DPKFILE_FLAG_ENCRYPTED) )
	{
		file->handle = handle;

		char path[2048];
		sprintf(path, "%s/_eqtmp%i",m_tempPath.c_str(),handle);

		file->file = fopen( path, mode );
	}
	else
		file->file = fopen( m_packageName.c_str(), mode );

    if (!file->file)
    {
        delete file;
        return NULL;
    }

	file->info = &fileInfo;

	// direct streaming
	if(	!(fileInfo.flags & DPKFILE_FLAG_COMPRESSED) &&
		!(fileInfo.flags & DPKFILE_FLAG_ENCRYPTED))
		fseek( file->file, fileInfo.offset, SEEK_SET );

	m_openFiles.append( file );

    return file;
}

void CDPKFileReader::Close( DPKFILE *fp )
{
    if (!fp)
        return;

    if(m_openFiles.remove( fp ))
	{
		fclose(fp->file);

		if( (fp->info->flags & DPKFILE_FLAG_COMPRESSED) ||
			fp->info->flags & DPKFILE_FLAG_ENCRYPTED)
		{
			char path[2048];
			sprintf( path, "%s/tmp_%i", m_tempPath.c_str(), fp->handle );

			// try to remove file
			remove(path);
			rmdir(m_tempPath.c_str());

			m_handles[fp->handle] = DPK_HANDLE_INVALID;
			m_dumpCount--;
		}

		delete fp;
	}
}

long CDPKFileReader::Seek( DPKFILE *fp, long pos, int seekType )
{
    if (!fp)
        return 0;

	if(	(fp->info->flags & DPKFILE_FLAG_COMPRESSED) ||
		(fp->info->flags & DPKFILE_FLAG_ENCRYPTED))
	{
		return fseek(fp->file,pos,seekType);
	}
	else
	{
		if(seekType == SEEK_END)
			return fseek(fp->file, fp->info->offset + fp->info->size + pos, SEEK_SET);
		else
			return fseek(fp->file, fp->info->offset + pos, seekType);
	}
}

long CDPKFileReader::Tell( DPKFILE *fp )
{
    if (!fp)
        return 0;

	if(	(fp->info->flags & DPKFILE_FLAG_COMPRESSED) ||
		(fp->info->flags & DPKFILE_FLAG_ENCRYPTED))
		return ftell(fp->file);

	return ftell(fp->file) - fp->info->offset;
}

int	CDPKFileReader::Eof( DPKFILE* fp )
{
	if(Tell(fp) >= fp->info->size)
		return 1;

	return 0;
}

size_t CDPKFileReader::Read( void *dest, size_t count, size_t size, DPKFILE *fp )
{
    if (!fp)
        return 0;

	if(	(fp->info->flags & DPKFILE_FLAG_COMPRESSED) ||
		(fp->info->flags & DPKFILE_FLAG_ENCRYPTED))
	{
		return fread(dest, count, size, fp->file);
	}
	else
	{
		long bytesToRead = count*size;

		long curPos = Tell(fp);

		if(curPos+bytesToRead > fp->info->size)
			bytesToRead -= (curPos+bytesToRead)-fp->info->size;

		if(bytesToRead <= 0)
			return 0;

		return fread(dest, 1, bytesToRead, fp->file);
	}
}

char* CDPKFileReader::Gets( char *dest, int destSize, DPKFILE *fp)
{
    if (!fp)
        return NULL;

	if(	(fp->info->flags & DPKFILE_FLAG_COMPRESSED) ||
		(fp->info->flags & DPKFILE_FLAG_ENCRYPTED))
	{
		return fgets( dest, destSize, fp->file );
	}
	else
	{
		if ( Tell(fp) >= fp->info->size )
			return NULL;

		return fgets( dest, destSize, fp->file );
	}
}
