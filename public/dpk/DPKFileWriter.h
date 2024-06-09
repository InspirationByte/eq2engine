///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data Pack File writer
//////////////////////////////////////////////////////////////////////////////////

/* TODO
		- Implement thread-save file adding
		- Iinterface for file stream creation instead of putting ready data through Add(...)
*/

#pragma once
#include "dpk/dpk_defs.h"
#include "utils/IceKey.h"

#include "core/platform/OSFile.h"

class IVirtualStream;

class CDPKFileWriter
{
public:
	CDPKFileWriter(const char* mountPath, int compression = 0, const char* encryptKey = nullptr, bool skipPacking = false);
	~CDPKFileWriter();

	bool					Begin(const char* fileName, ESearchPath searchPath = SP_ROOT);

	// adds data to the pack file
	uint					Add(IVirtualStream* fileData, const char* fileName, int packageFlags = 0xff);

#if 0
	// creates new package file and returns stream for writing
	IVirtualStream*			Create(const char* fileName, bool skipCompression = false);
	void					Close(IVirtualStream* virtStream);
#endif
	void					Flush();
	int						End(bool storeFileList = false);

	int						GetFileCount() const { return m_files.size(); }

protected:
	uint					WriteDataToPackFile(IVirtualStream* fileData, dpkfileinfo_t& pakInfo, int packageFlags = 0xff);

	struct FileInfo
	{
		dpkfileinfo_t	pakInfo;
		EqString		fileName;
	};


	char					m_mountPath[DPK_STRING_SIZE];
	IceKey					m_ice;

	dpkheader_t				m_header;
	COSFile					m_output;

	EqString				m_packFileName;
	ESearchPath				m_packFilePath;

	Array<CMemoryStream*>	m_openStreams{ PP_SL };
	Map<int, FileInfo>		m_files{ PP_SL };

	int						m_compressionLevel{ 0 };
	bool					m_encrypted{ false };
	bool					m_skipPacking{ false };
};