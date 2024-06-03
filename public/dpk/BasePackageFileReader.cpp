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

CBasePackageReader* CBasePackageReader::CreateReaderByExtension(const char* packageName)
{
	const EqString fileExt(_Es(packageName).Path_Extract_Ext());
	CBasePackageReader* reader = nullptr;

	// allow zip files to be loaded
	if (fileExt == "zip")
		reader = PPNew CZipFileReader();
	else
		reader = PPNew CDPKFileReader();

	return reader;
}

bool CBasePackageReader::GetInternalFileName(EqString& pkgFileName, const char* fileName) const
{
	EqString fullFilename(fileName);
	fullFilename = fullFilename.LowerCase();
	fullFilename.Path_FixSlashes();

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
