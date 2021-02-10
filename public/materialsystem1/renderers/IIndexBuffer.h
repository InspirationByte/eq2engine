//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer interface declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef IINDEXBUFFER_H
#define IINDEXBUFFER_H

#include "ShaderAPI_defs.h"
#include "core/ppmem.h"

class IIndexBuffer
{
public:
	virtual ~IIndexBuffer() {}

	PPMEM_MANAGED_OBJECT();

	// returns index size
	virtual int8			GetIndexSize() = 0;

	// returns index count
	virtual int				GetIndicesCount() = 0;

	// updates buffer without map/unmap operations which are slower
	virtual void			Update(void* data, int size, int offset, bool discard = true) = 0;

	// locks index buffer and gives to programmer buffer data
	virtual bool			Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly) = 0;

	// unlocks buffer
	virtual void			Unlock() = 0;
};

#endif // IINDEXBUFFER_H
