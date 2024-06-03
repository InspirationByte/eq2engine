///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Dark package file (dpk) definitions
//////////////////////////////////////////////////////////////////////////////////

#pragma once

constexpr int DPK_VERSION		= 8;
constexpr int DPK_PREV_VERSION	= 7;
constexpr int DPK_SIGNATURE		= MAKECHAR4('E', 'Q', 'P', 'K');

constexpr int DPK_BLOCK_MAXSIZE	= (8 * 1024);
constexpr int DPK_STRING_SIZE	= 255;

constexpr EqStringRef s_dpkPackageDefaultExt = "epk";

enum EDPKFileFlags : int
{
	DPKFILE_FLAG_COMPRESSED			= (1 << 0),
	DPKFILE_FLAG_ENCRYPTED			= (1 << 1),
};

static bool DPK_IsBlockFile(int flags)
{
	return flags & (DPKFILE_FLAG_COMPRESSED | DPKFILE_FLAG_ENCRYPTED);
}

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

struct dpkblock_s
{
	uint32	size;
	uint32	compressedSize;
	short	flags;
};
ALIGNED_TYPE(dpkblock_s, 2) dpkblock_t;

// data package file info
struct dpkfileinfo_s
{
	int		filenameHash;

	uint64	offset;
	uint32	size;				// The real file size
	uint32	crc;

	short	numBlocks;			// number of blocks
	short	flags;
};
ALIGNED_TYPE(dpkfileinfo_s, 2) dpkfileinfo_t;
