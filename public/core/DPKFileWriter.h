///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
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
#include "utils/eqstring.h"
#include "utils/IceKey.h"

#define DPK_STRING_SIZE 255

struct dpkfilewinfo_t
{
	dpkfileinfo_t	pkinfo;
	EqString		fileName;
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

	void					AddIgnoreCompressionExtension( const char* extension );

	bool					BuildAndSave( const char* fileNamePrefix );

protected:

	void					ProcessFile(FILE* output, dpkfilewinfo_t* info);

	bool					WriteFiles();
	bool					SavePackage();

	bool					CheckCompressionIgnored(const char* extension) const;

	FILE*					m_file;
	dpkheader_t				m_header;

	char					m_mountPath[DPK_STRING_SIZE];

	DkList<dpkfilewinfo_t*>	m_files;
	DkList<EqString>		m_ignoreCompressionExt;

	int						m_compressionLevel;
	int						m_encryption;

	IceKey					m_ice;
	
};

#endif // DPK_FILE_WRITER_H
