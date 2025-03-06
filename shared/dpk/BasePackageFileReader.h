///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IPackFileReader.h"

enum EPackageType
{
	PACKAGE_READER_FLAT = 0,
	PACKAGE_READER_DPK,
	PACKAGE_READER_ZIP,
};

class CBasePackageReader;
using CBasePackageReaderPtr = CRefPtr<CBasePackageReader>;

class IPackFileStream : public IFile
{
public:
	virtual CBasePackageReader* GetHostPackage() const = 0;
};

//--------------------------------------------------

class CBasePackageReader : public IPackFileReader
{
public:
	static CBasePackageReaderPtr	CreateReaderByExtension(const char* packageName);

	virtual EPackageType	GetType() const = 0;
	const char*				GetName() const	{ return m_packagePath; }

	virtual bool			InitPackage(const char* filename, const char* mountPath = nullptr) = 0;
	virtual bool			OpenEmbeddedPackage(CBasePackageReader* target, const char* filename) { return false; }


	int						GetSearchPath() const		{ return m_searchPath; };
	void					SetSearchPath(int search)	{ m_searchPath = search; };
	void					SetKey(const char* key)		{ m_key = key; }

	bool					GetInternalFileName(EqString& packageFilename, const char* fileName) const;

protected:

	EqString		m_packagePath;
	EqString		m_mountPath;
	EqString		m_key;
	int				m_searchPath{ -1 };
};

//--------------------------------------------------

