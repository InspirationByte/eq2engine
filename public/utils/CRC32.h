#ifndef CRC32_H
#define CRC32_H

/*
===============================================================================

	Calculates a checksum for a block of data
	using the CRC-32.

===============================================================================
*/

#include "Platform.h"

void CRC32_InitChecksum( uint32 &crcvalue );

void CRC32_Update( uint32 &crcvalue, const char data );
void CRC32_UpdateChecksum( uint32 &crcvalue, const void *data, int length );

void CRC32_FinishChecksum( uint32 &crcvalue );

uint32 CRC32_BlockChecksum( const void *data, int length );

#endif // CRC32_H
