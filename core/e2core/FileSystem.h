//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Filesystem
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IFileSystem.h"
#include "core/platform/OSFile.h"

class CBasePackageReader;
struct FSSearchPathInfo;

//------------------------------------------------------------------------------
// File stream
//------------------------------------------------------------------------------

class CFile : public IFileStream
{
	friend class CFileSystem;

public:
	CFile(const char* fileName, COSFile&& file);

	VSSize				Seek(int64 pos, EFileStreamSeek seekType );
	VSSize				Tell() const;
	VSSize				Read(void *dest, VSSize count, VSSize size);
	VSSize				Write(const void *src, VSSize count, VSSize size);

    bool				Flush();

	uint32				GetCRC32();
	VSSize				GetSize();

	EFileStreamType	GetType() const { return FS_TYPE_FILE; }

	const char*			GetName() const { return m_name; }
protected:
	EqString			m_name;
	COSFile				m_osFile;
};

//------------------------------------------------------------------------------
// Filesystem base
//------------------------------------------------------------------------------

class CFileSystem : public IFileSystem
{
	friend class CFile;
	friend class CDPKFileReader;
	friend class CZipFileReader;
public:
								 CFileSystem();
								~CFileSystem();

    //Initialization of filesystem
    bool						Init( bool editorMode );
	void						Shutdown();

	//------------------------------------------------------------
	// Directory stuff
	//------------------------------------------------------------

	// something like working directory
	void						SetBasePath(const char* path);
	const char*					GetBasePath() const				{return m_basePath.ToCString();}

    // Returns current game path
    const char*					GetCurrentGameDirectory() const;

    // Returns current engine data path
    const char*					GetCurrentDataDirectory() const;

	// adds directory for file search
	void						AddSearchPath(const char* pathId, const char* pszDir);
	void						RemoveSearchPath(const char* pathId);

	void						Rename(const char* oldNameOrPath, const char* newNameOrPath, ESearchPath search) const;

    //Directory operations
	bool						DirExist(const char* dirname, ESearchPath search) const;
    void						MakeDir(const char* dirname, ESearchPath search ) const;
    void						RemoveDir(const char* dirname, ESearchPath search ) const;

	//------------------------------------------------------------
	// File operations
	//------------------------------------------------------------

    IFileStreamPtr				Open(const char* filename, int openFlags, int searchFlags = -1 );

	EqString					FindFilePath(const char* filename, int searchFlags = -1) const;
	bool						FileCopy(const char* filename, const char* dest_file, bool overWrite, ESearchPath search);
	bool						FileExist(const char* filename, int searchFlags = -1) const;
	void						FileRemove(const char* filename, ESearchPath search ) const;

	// The next ones are deprecated and will be removed

    ubyte*						GetFileBuffer(const char* filename, VSSize* filesize = 0, int searchFlags = -1);
	VSSize						GetFileSize(const char* filename, int searchFlags = -1);
	uint32						GetFileCRC32(const char* filename, int searchFlags = -1);

	//------------------------------------------------------------
	// Packages
	//------------------------------------------------------------

	bool						SetAccessKey(const char* accessKey);

	// adds package to file system as another layer, acts just like AddSearchPath
	// NOTE: packageName must be root-level package file
	bool						AddPackage(const char* packageName, ESearchPath type, const char* mountPath = nullptr);
	void						RemovePackage(const char* packageName);

	// opens package for further reading. Does not add package as FS layer
	IPackFileReaderPtr			OpenPackage(const char* packageName, int searchFlags = -1);

	//------------------------------------------------------------
	// Locator
	//------------------------------------------------------------

	// opens directory for search props
	const char*					FindFirst(const char* wildcard, FSFindData** findData, int searchPaths = -1, int dirIndex = -1);
	const char*					FindNext(FSFindData* findData) const;
	void						FindClose(FSFindData* findData);
	bool						FindIsDirectory(FSFindData* findData) const;
	int							FindGetDirIndex(FSFindData* findData) const;

	//-------------------------
	bool						IsInitialized() const { return m_isInit; }

protected:

	bool						InitNextPath(FSFindData* findData) const;

	EqString					GetAbsolutePath(ESearchPath search, const char* dirOrFileName) const;
	EqString					GetSearchPath(ESearchPath search, int directoryId = -1) const;

	using SPWalkFunc = EqFunction<bool(const EqString& filePath, ESearchPath searchPath, const FSSearchPathInfo& spInfo, int spFlags)>;
	bool						WalkOverSearchPaths(int searchFlags, const char* fileName, const SPWalkFunc& func) const;

	EqString					m_basePath;			// base prepended path
    EqString					m_dataDir;			// Used to load engine data
	EqString					m_accessKey;

	Array<FSSearchPathInfo>		m_directories{ PP_SL };		// mod data, for fall back
    Array<IPackFileReaderPtr>	m_fsPackages{ PP_SL };		// package serving as FS layers

	Array<FSFindData*>			m_findDatas{ PP_SL };

	bool						m_isInit{ false };
};

