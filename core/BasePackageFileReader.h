///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Zip package file (zip)
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEPACKAGEFILEREADER_H
#define BASEPACKAGEFILEREADER_H

#include "utils/IVirtualStream.h"
#include "utils/eqstring.h"
#include "utils/eqthread.h"

class CBasePackageFileReader;

class CBasePackageFileStream : public IVirtualStream
{
public:
	virtual CBasePackageFileReader* GetHostPackage() const = 0;
};

//--------------------------------------------------

class CBasePackageFileReader
{
public:
	CBasePackageFileReader(Threading::CEqMutex&	mutex) :
		m_FSMutex(mutex)
	{
	}

	virtual ~CBasePackageFileReader() {}

	virtual bool					InitPackage(const char* filename, const char* mountPath = nullptr) = 0;

	virtual IVirtualStream*			Open(const char* filename, const char* mode) = 0;
	virtual void					Close(IVirtualStream* fp) = 0;
	virtual bool					FileExists(const char* filename) const = 0;

	const char*						GetPackageFilename() const { return m_packageName.ToCString(); }
	int								GetSearchPath() const { return m_searchPath; };
	virtual void					SetSearchPath(int search) { m_searchPath = search; };

	virtual void					SetKey(const char* key) { m_key = key; }

protected:

	EqString				m_packagePath;
	EqString				m_packageName;
	EqString				m_mountPath;
	EqString				m_key;

	int						m_searchPath;

	Threading::CEqMutex&	m_FSMutex;
};

#endif // BASEPACKAGEFILEREADER_H