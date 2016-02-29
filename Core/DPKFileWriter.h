///////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#ifndef DPKFILEWRITER_H
#define DPKFILEWRITER_H

#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "platform/Platform.h"
#include "dpk_defs.h"
#include "utils/DkList.h"

struct dpkfilewinfo_t
{
	dpkfileinfo_t	pkinfo;
	char			filename[DPK_MAX_FILENAME_LENGTH];
	long			size;
};

ALIGNED_TYPE(dpkfileinfo_s, 2) dpkfileinfo_t;

class CDPKFileWriter
{
public:
							CDPKFileWriter();
							~CDPKFileWriter();

	void					SetCompression( int compression );
	void					SetEncryption( int type, const char* key );
	void					SetMountPath( const char* path );

	bool					AddFile( const char* fileName );
	void					AddDirecory( const char* directoryname, bool bRecurse );

	bool					BuildAndSave( const char* fileNamePrefix );

protected:

	bool					WriteFiles();
	bool					SavePackage();

	FILE*					m_file;
	dpkheader_t				m_header;

	char					m_mountPath[DPK_MAX_FILENAME_LENGTH];

	DkList<dpkfilewinfo_t*>	m_files;

	int						m_compressionLevel;
	int						m_encryption;
	uint32					m_key[4];
};

#endif // DPK_FILE_WRITER_H
