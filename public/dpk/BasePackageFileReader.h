///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IFilePackageReader.h"

enum EPackageReaderType
{
	PACKAGE_READER_DPK = 0,
	PACKAGE_READER_ZIP,
};

class CBasePackageReader;

class IPackFileStream : public IFile
{
public:
	virtual CBasePackageReader* GetHostPackage() const = 0;
};

//--------------------------------------------------

class CBasePackageReader : public IFilePackageReader
{
public:
	static CBasePackageReader*	CreateReaderByExtension(const char* packageName);

	virtual bool				InitPackage(const char* filename, const char* mountPath = nullptr) = 0;
	virtual bool				OpenEmbeddedPackage(CBasePackageReader* target, const char* filename) { return false; }

	const char*					GetPackageFilename() const	{ return m_packagePath; }
	int							GetSearchPath() const		{ return m_searchPath; };
	void						SetSearchPath(int search)	{ m_searchPath = search; };
	void						SetKey(const char* key)		{ m_key = key; }

	virtual EPackageReaderType	GetType() const = 0;

	bool						GetInternalFileName(EqString& packageFilename, const char* fileName) const;

protected:

	EqString		m_packagePath;
	EqString		m_mountPath;
	EqString		m_key;
	int				m_searchPath{ -1 };
};