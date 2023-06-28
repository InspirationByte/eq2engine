///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

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

	bool		Begin(const char* fileName);
	uint		Add(IVirtualStream* fileData, const int nameHash, bool skipCompression);
	void		Flush();
	int			End();

	int			GetFileCount() const { return m_files.numElem(); }

protected:
	char					m_mountPath[DPK_STRING_SIZE];
	IceKey					m_ice;

	dpkheader_t				m_header;
	COSFile					m_output;

	Array<dpkfileinfo_t>	m_files{ PP_SL };
	int						m_compressionLevel{ 0 };
	bool					m_encrypted{ false };
};