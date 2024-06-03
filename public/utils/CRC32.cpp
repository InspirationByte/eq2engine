/*
   CRC-32
   Copyright (C) 1995-1998 Mark Adler
*/

#include "core/common_types.h"
#include "CRC32.h"

#define CRC32_INIT_VALUE	0xffffffffL
#define CRC32_XOR_VALUE		0xffffffffL

void CRC32_InitChecksum( uint32 &crcvalue )
{
	crcvalue = CRC32_INIT_VALUE;
}

void CRC32_Update( uint32 &crcvalue, const char data )
{
	crcvalue = crc32_detail::crctable[ ( crcvalue ^ data ) & 0xff ] ^ ( crcvalue >> 8 );
}

void CRC32_UpdateChecksum( uint32 &crcvalue, const void *data, size_t length )
{
	uint32 crc;
	const unsigned char *buf = (const unsigned char *) data;

	crc = crcvalue;

	while( length-- )
	{
		crc = crc32_detail::crctable[ ( crc ^ ( *buf++ ) ) & 0xff ] ^ ( crc >> 8 );
	}

	crcvalue = crc;
}

void CRC32_FinishChecksum( uint32 &crcvalue )
{
	crcvalue ^= CRC32_XOR_VALUE;
}

uint32 CRC32_BlockChecksum( const void *data, size_t length )
{
	uint32 crc;

	CRC32_InitChecksum( crc );
	CRC32_UpdateChecksum( crc, data, length );
	CRC32_FinishChecksum( crc );
	return crc;
}
