///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/platform/OSFile.h"

#include "minizip/unzip.h"
#include "ZipFileReader.h"
#include "DPKUtils.h"

static Threading::CEqMutex s_zipMutex;

//-----------------------------------------------

CZipFileStream::CZipFileStream(const char* fileName, uintptr_t zf, CZipFileReader* host)
	: m_name(fileName)
	, m_zipHandle(zf)
	, m_host(host)
{
	unz_file_info fileInfo;
	unzGetCurrentFileInfo(reinterpret_cast<unzFile>(m_zipHandle), &fileInfo, nullptr, 0, nullptr, 0, nullptr, 0);

	m_uncompressedSize = fileInfo.uncompressed_size;
	m_crc = fileInfo.crc;
}

CZipFileStream::~CZipFileStream()
{
	unzCloseCurrentFile(reinterpret_cast<unzFile>(m_zipHandle));
	unzClose(reinterpret_cast<unzFile>(m_zipHandle));
}

CBasePackageReader* CZipFileStream::GetHostPackage() const
{
	return (CBasePackageReader*)m_host;
}

// reads data from virtual stream
VSSize CZipFileStream::Read(void *dest, VSSize count, VSSize size)
{
	if (count <= 0 || size <= 0)
		return 0;

	return unzReadCurrentFile(reinterpret_cast<unzFile>(m_zipHandle), dest, count * size);
}

// writes data to virtual stream
VSSize CZipFileStream::Write(const void *src, VSSize count, VSSize size)
{
	ASSERT_FAIL("CZipFileStream does not support WRITE OPS");
	return 0;
}

// seeks pointer to position
VSSize CZipFileStream::Seek(int64 nOffset, EVirtStreamSeek seekType)
{
	int newOfs = 0;
	static char dummy[32*1024];

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
	unzCloseCurrentFile(reinterpret_cast<unzFile>(m_zipHandle));
	unzOpenCurrentFile(reinterpret_cast<unzFile>(m_zipHandle));

	// Skip until the desired offset is reached
	while (newOfs)
	{
		int len = newOfs;
		if (len > sizeof(dummy))
			len = sizeof(dummy);

		const int numRead = unzReadCurrentFile(reinterpret_cast<unzFile>(m_zipHandle), dummy, len);
		if (numRead <= 0)
			break;

		newOfs -= numRead;
	}

	return 0;
}

// returns current pointer position
VSSize CZipFileStream::Tell() const
{
	return unztell(reinterpret_cast<unzFile>(m_zipHandle));
}

//-----------------------------------------------------------------------------------------------------------------------
// ZIP host
//-----------------------------------------------------------------------------------------------------------------------

bool CZipFileReader::InitPackage(const char* filename, const char* mountPath/* = nullptr*/)
{
	char path[2048];
	m_packagePath = filename;

	// perform test
	unzFile zip = unzOpen(m_packagePath);
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
	for (uLong i = 0; i < ugi.number_entry; ++i)
	{
		unz_file_info ufi;
		unzGetCurrentFileInfo(zip, &ufi, path, sizeof(path), nullptr, 0, nullptr, 0);
		if (ufi.uncompressed_size > COMPRESSED_FILE_BIG_FILE_SIZE_THRESHOLD && float(ufi.compressed_size) / float(ufi.uncompressed_size) < COMPRESSION_RATIO_WARNING_THRESHOLD)
		{
			warnAboutCompression = true;
		}

		EqString zipFilePath = path;
		DPK_FixSlashes(zipFilePath);
		const int nameHash = StringId24(zipFilePath, true);

		ZFileInfo& zf = *m_files.insert(nameHash);
		zf.fileName = zipFilePath;
		unzGetFilePos(zip, (unz_file_pos*)&zf.filePos);

		unzGoToNextFile(zip);
	}

	if(warnAboutCompression)
		MsgWarning("WARNING: Highly compressed ZIP archive (%s) may reduce performance and loading speeds\n", filename);

	// if custom mount path provided, use it
	if (mountPath)
	{
		m_mountPath = mountPath;
		fnmPathFixSeparators(m_mountPath);
	}
	else if (unzLocateFile(zip, "dpkmount", 2) == UNZ_OK)
	{
		if (unzOpenCurrentFile(zip) == UNZ_OK)
		{
			// read contents
			CZipFileStream mountFile(filename, reinterpret_cast<uintptr_t>(zip), this);

			memset(path, 0, sizeof(path));
			mountFile.Read(path, mountFile.GetSize(), 1);

			m_mountPath = path;
			fnmPathFixSeparators(m_mountPath);
		}
	}

	unzClose(zip);

	return true;
}

IFilePtr CZipFileReader::Open(const char* filename, int modeFlags)
{
	if (modeFlags & (COSFile::APPEND | COSFile::WRITE))
	{
		ASSERT_FAIL("Archived files only can open for reading!\n");
		return nullptr;
	}

	const int nameHash = StringId24(filename, true);
	unzFile zipFileHandle = reinterpret_cast<unzFile>(GetZippedFile(nameHash));
	if (!zipFileHandle)
		return nullptr;

	if (unzOpenCurrentFile(zipFileHandle) != UNZ_OK)
	{
		unzClose(zipFileHandle);
		return nullptr;
	}

	CRefPtr<CZipFileStream> newStream = CRefPtr_new(CZipFileStream, filename, reinterpret_cast<uintptr_t>(zipFileHandle), this);

	return IFilePtr(newStream);
}

IFilePtr CZipFileReader::Open(int fileIndex, int modeFlags)
{
	if (modeFlags & (COSFile::APPEND | COSFile::WRITE))
	{
		ASSERT_FAIL("Archived files only can open for reading!\n");
		return nullptr;
	}

	unzFile zipFileHandle = reinterpret_cast<unzFile>(GetZippedFile(fileIndex));
	if (!zipFileHandle)
		return nullptr;

	if (unzOpenCurrentFile(zipFileHandle) != UNZ_OK)
	{
		unzClose(zipFileHandle);
		return nullptr;
	}

	CRefPtr<CZipFileStream> newStream = CRefPtr_new(CZipFileStream, EqString::Format("zipFile%d", fileIndex), reinterpret_cast<uintptr_t>(zipFileHandle), this);
	return IFilePtr(newStream);
}

bool CZipFileReader::FileExists(const char* filename) const
{
	const int nameHash = StringId24(filename, true);
	unzFile test = reinterpret_cast<unzFile>(GetZippedFile(nameHash));
	if(test)
		unzClose(test);

	return test != nullptr;
}

int CZipFileReader::FindFileIndex(const char* filename) const
{
	const int nameHash = StringId24(filename, true);
	auto it = m_files.find(nameHash);
	if (!it.atEnd())
		return it.key();
	return -1;
}

uintptr_t CZipFileReader::GetZippedFile(int nameHash) const
{
	auto it = m_files.find(nameHash);
	if (!it.atEnd())
	{
		const ZFileInfo& file = *it;
		unzFile zipFile = unzOpen(m_packagePath);

		if (unzGoToFilePos(zipFile, (unz_file_pos*)&file.filePos) != UNZ_OK)
		{
			unzClose(zipFile);
			return 0;
		}

		return reinterpret_cast<uintptr_t>(zipFile);
	}

	return 0;
}