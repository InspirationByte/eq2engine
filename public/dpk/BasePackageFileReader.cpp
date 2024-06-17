///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "BasePackageFileReader.h"
#include "DPKUtils.h"

#include "dpk/DPKFileReader.h"
#include "dpk/ZipFileReader.h"

CBasePackageReaderPtr CBasePackageReader::CreateReaderByExtension(const char* packageName)
{
	const EqString fileExt = fnmPathExtractExt(packageName);
	CBasePackageReaderPtr reader = nullptr;

	// allow zip files to be loaded
	if (fileExt == "zip")
		reader = CBasePackageReaderPtr(CRefPtr_new(CZipFileReader));
	else
		reader = CBasePackageReaderPtr(CRefPtr_new(CDPKFileReader));

	return reader;
}

bool CBasePackageReader::GetInternalFileName(EqString& pkgFileName, const char* fileName) const
{
	EqString fullFilename = EqStringRef(fileName).LowerCase();
	fnmPathFixSeparators(fullFilename);

	if (m_mountPath.Length())
	{
		const int mountPathPos = fullFilename.Find(m_mountPath.ToCString());
		if (mountPathPos > 0 || mountPathPos == -1)
			return false;
	}

	// replace
	pkgFileName = fullFilename.Right(fullFilename.Length() - m_mountPath.Length() - 1);
	DPK_FixSlashes(pkgFileName);
	return true;
}
