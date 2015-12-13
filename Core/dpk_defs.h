///////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk) definitions
//////////////////////////////////////////////////////////////////////////////////

#ifndef DPK_DEFS_H
#define DPK_DEFS_H

#include "dktypes.h"

#define DPK_VERSION					4
#define DPK_SIGNATURE				MCHAR4('E','Q','P','K')
#define DPK_MAX_FILENAME_LENGTH		260

#define DPK_DEFAULT_EXTENSION		".eqpak"

enum EFileFlags
{
	DPKFILE_FLAG_COMPRESSED			= (1 << 0),
	DPKFILE_FLAG_ENCRYPTED			= (1 << 1),
	DPKFILE_FLAG_NAME_ENCRYPTED		= (1 << 2),
};

//---------------------------

// data package header
struct dpkheader_s
{
public:
	int		signature;

	uint8	version;
	uint8	compressionLevel;

	int		numFiles;
	uint64	fileInfoOffset;
};

ALIGNED_TYPE(dpkheader_s, 2) dpkheader_t;

//---------------------------

// data package file info
struct dpkfileinfo_s
{
	//char	filename[DPK_MAX_FILENAME_LENGTH];
	int		filenameHash;

	uint64	offset;
	long	size;				// The real file size
	long	compressedSize;		// The compressed file size
	
	short	flags;
};

ALIGNED_TYPE(dpkfileinfo_s, 2) dpkfileinfo_t;

#endif //DPK_DEFS_H
