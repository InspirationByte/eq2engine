///////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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
	CDPKFileWriter(const char* mountPath, int compression = 0, const char* encryptKey = nullptr);
	~CDPKFileWriter();

	bool					Begin(const char* fileName, ESearchPath searchPath = SP_ROOT);

	// adds data to the pack file
	uint					Add(IVirtualStream* fileData, const char* fileName, bool skipCompression = false);

#if 0
	// creates new package file and returns stream for writing
	IVirtualStream*			Create(const char* fileName, bool skipCompression = false);
	void					Close(IVirtualStream* virtStream);
#endif
	void					Flush();
	int						End();

	int						GetFileCount() const { return m_files.size(); }

protected:
	uint					WriteDataToPackFile(IVirtualStream* fileData, dpkfileinfo_t& pakInfo, bool skipCompression);

	char					m_mountPath[DPK_STRING_SIZE];
	IceKey					m_ice;

	dpkheader_t				m_header;
	COSFile					m_output;

	Array<CMemoryStream*>	m_openStreams{ PP_SL };
	Map<int, dpkfileinfo_t>	m_files{ PP_SL };
	int						m_compressionLevel{ 0 };
	bool					m_encrypted{ false };
};