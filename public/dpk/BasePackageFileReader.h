///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IFilePackageReader.h"

class CBasePackageReader;

class CBasePackageFileStream : public IFile
{
public:
	virtual CBasePackageReader* GetHostPackage() const = 0;
};

//--------------------------------------------------

class CBasePackageReader : public IFilePackageReader
{
public:
	virtual bool	InitPackage(const char* filename, const char* mountPath = nullptr) = 0;

	const char*		GetPackageFilename() const	{ return m_packageName.ToCString(); }
	int				GetSearchPath() const		{ return m_searchPath; };
	virtual void	SetSearchPath(int search)	{ m_searchPath = search; };

	virtual void	SetKey(const char* key)		{ m_key = key; }

protected:

	EqString		m_packagePath;
	EqString		m_packageName;
	EqString		m_mountPath;
	EqString		m_key;

	int				m_searchPath{ -1 };
};