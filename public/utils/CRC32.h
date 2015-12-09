#ifndef CRC32_H
#define CRC32_H

/*
===============================================================================

	Calculates a checksum for a block of data
	using the CRC-32.

===============================================================================
*/

#include "Platform.h"

void CRC32_InitChecksum( unsigned long &crcvalue );

void CRC32_Update( unsigned long &crcvalue, const char data );
void CRC32_UpdateChecksum( unsigned long &crcvalue, const void *data, int length );

void CRC32_FinishChecksum( unsigned long &crcvalue );

unsigned long CRC32_BlockChecksum( const void *data, int length );

#endif // CRC32_H