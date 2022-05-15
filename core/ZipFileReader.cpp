///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////


#include "ZipFileReader.h"
#include "FileSystem.h"

#include "core/DebugInterface.h"

#include "utils/strtools.h"
#include "utils/CRC32.h"

extern void DPK_FixSlashes(EqString& str);

CZipFileStream::CZipFileStream(unzFile zip) : m_zipHandle(zip)
{
	unzGetCurrentFileInfo(m_zipHandle, &m_finfo, NULL, 0, NULL, 0, NULL, 0);
}

CZipFileStream::~CZipFileStream()
{
}

CBasePackageFileReader* CZipFileStream::GetHostPackage() const
{
	return (CBasePackageFileReader*)m_host;
}

// reads data from virtual stream
size_t CZipFileStream::Read(void *dest, size_t count, size_t size)
{
	return unzReadCurrentFile(m_zipHandle, dest, count*size);
}

// writes data to virtual stream
size_t CZipFileStream::Write(const void *src, size_t count, size_t size)
{
	ASSERT_MSG(false, "CZipFileStream does not support WRITE OPS");
	return 0;
}

// seeks pointer to position
int	CZipFileStream::Seek(long nOffset, VirtStreamSeek_e seekType)
{
	int newOfs = 0;
	char dummy[32*1024];

	switch (seekType)
	{
		case VS_SEEK_SET:
		{
			newOfs = nOffset;
			break;
		}
		case VS_SEEK_CUR:
		{
			newOfs = Tell() + nOffset;
			break;
		}
		case VS_SEEK_END:
		{
			newOfs = GetSize() + nOffset;
			break;
		}
	}

	// it has to be reopened
	// slow!!!
	unzCloseCurrentFile(m_zipHandle);
	unzOpenCurrentFile(m_zipHandle);

	// Skip until the desired offset is reached
	while (newOfs)
	{
		int len = newOfs;
		if (len > sizeof(dummy))
			len = sizeof(dummy);

		int numRead = unzReadCurrentFile(m_zipHandle, dummy, len);
		if (numRead <= 0)
			break;

		newOfs -= numRead;
	}

	return 0;
}

// fprintf analog
void CZipFileStream::Print(const char* fmt, ...)
{
	ASSERT_MSG(false, "CZipFileStream does not support WRITE OPS");
}

// returns current pointer position
long CZipFileStream::Tell() const
{
	return unztell(m_zipHandle);
}

// returns memory allocated for this stream
long CZipFileStream::GetSize()
{
	return m_finfo.uncompressed_size;
}

// flushes stream from memory
int	CZipFileStream::Flush()
{
	return 0;
}

// returns CRC32 checksum of stream
uint32 CZipFileStream::GetCRC32()
{
	return m_finfo.crc;
}

//-----------------------------------------------------------------------------------------------------------------------
// ZIP host
//-----------------------------------------------------------------------------------------------------------------------

CZipFileReader::CZipFileReader(Threading::CEqMutex& fsMutex) : CBasePackageFileReader(fsMutex)
{

}

CZipFileReader::~CZipFileReader()
{
	for (int i = 0; i < m_openFiles.numElem(); i++)
	{
		Close(m_openFiles[i]);
	}
}

bool CZipFileReader::InitPackage(const char* filename, const char* mountPath/* = nullptr*/)
{
	char path[2048];

	m_packageName = filename;

	m_packagePath = ((CFileSystem*)g_fileSystem)->GetAbsolutePath(SP_ROOT, filename);

	// perform test
	unzFile zip = GetNewZipHandle();
	if (!zip)
	{
		MsgError("Cannot open Zip package '%s'\n", m_packagePath.ToCString());
		return false;
	}

	// add files
	unz_global_info ugi;
	unzGetGlobalInfo(zip, &ugi);

	const float COMPRESSION_RATIO_WARNING_THRESHOLD = 0.75f;
	const int COMPRESSED_FILE_BIG_FILE_SIZE_THRESHOLD = 8 * 1024 * 1024;
	bool warnAboutCompression = false;

	// hash all file names and positions
	for (int i = 0; i < ugi.number_entry; i++)
	{
		unz_file_info ufi;
		unzGetCurrentFileInfo(zip, &ufi, path, sizeof(path), NULL, 0, NULL, 0);
		if (ufi.uncompressed_size > COMPRESSED_FILE_BIG_FILE_SIZE_THRESHOLD && float(ufi.compressed_size) / float(ufi.uncompressed_size) < COMPRESSION_RATIO_WARNING_THRESHOLD)
		{
			warnAboutCompression = true;
		}

		zfileinfo_t zf;
		zf.filename = path;
		DPK_FixSlashes(zf.filename);
		unzGetFilePos(zip, &zf.pos);
	
		const int nameHash = StringToHash(zf.filename.ToCString(), true);
		m_files.insert(nameHash, zf);

		unzGoToNextFile(zip);
	}

	if(warnAboutCompression)
		MsgWarning("WARNING: Highly compressed ZIP archive (%s) may reduce performance and loading speeds\n", filename);

	// if custom mount path provided, use it
	if (mountPath)
	{
		m_mountPath = mountPath;
		m_mountPath.Path_FixSlashes();
	}
	else if (unzLocateFile(zip, "dpkmount", 2) == UNZ_OK)
	{
		if (unzOpenCurrentFile(zip) == UNZ_OK)
		{
			// read contents
			CZipFileStream mountFile(zip);

			memset(path, 0, sizeof(path));
			mountFile.Read(path, mountFile.GetSize(), 1);

			m_mountPath = path;
			m_mountPath.Path_FixSlashes();
		}
	}

	unzClose(zip);

	return true;
}

IVirtualStream* CZipFileReader::Open(const char* filename, const char* mode)
{
	// check for write access
	if (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'))
	{
		MsgError("DPK only can open for reading!\n");
		return nullptr;
	}

	unzFile zipFileHandle = GetZippedFile(filename);

	if (!zipFileHandle)
		return nullptr;

	if (unzOpenCurrentFile(zipFileHandle) != UNZ_OK)
	{
		unzClose(zipFileHandle);
		return nullptr;
	}

	CZipFileStream* newStream = PPNew CZipFileStream(zipFileHandle);
	newStream->m_host = this;

	{
		Threading::CScopedMutex m(m_FSMutex);
		m_openFiles.append(newStream);
	}

	return newStream;
}

void CZipFileReader::Close(IVirtualStream* fp)
{
	if (!fp)
		return;

	CZipFileStream* fsp = (CZipFileStream*)fp;

	{
		Threading::CScopedMutex m(m_FSMutex);
		if (!m_openFiles.fastRemove(fsp))
			return;
	}

	unzCloseCurrentFile(fsp->m_zipHandle);
	unzClose(fsp->m_zipHandle);
	delete fsp;
}

bool CZipFileReader::FileExists(const char* filename) const
{
	unzFile test = GetZippedFile(filename);
	if(test)
		unzClose(test);

	return test != nullptr;
}

unzFile CZipFileReader::GetNewZipHandle() const
{
	return unzOpen(m_packagePath.ToCString());
}

unzFile	CZipFileReader::GetZippedFile(const char* filename) const
{
	EqString fullFilename(filename);
	fullFilename = fullFilename.LowerCase();
	fullFilename.Path_FixSlashes();

	if (m_mountPath.Length())
	{
		int mountPathPos = fullFilename.Find(m_mountPath.ToCString());

		if (mountPathPos != 0)
			return nullptr;
	}

	// replace
	EqString pkgFileName = fullFilename.Right(fullFilename.Length() - m_mountPath.Length() - 1);
	DPK_FixSlashes(pkgFileName);

	const int nameHash = StringToHash(pkgFileName.ToCString(), true);

	//Msg("Request file '%s' %d\n", pkgFileName.ToCString(), strHash);

	auto it = m_files.find(nameHash);
	if (it != m_files.end())
	{
		const zfileinfo_t& file = *it;
		unzFile zipFile = GetNewZipHandle();

		if (unzGoToFilePos(zipFile, (unz_file_pos*)&file.pos) != UNZ_OK)
		{
			unzClose(zipFile);
			return nullptr;
		}

		return zipFile;
	}

	return nullptr;
}