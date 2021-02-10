//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Vertex Buffer interface declaration
//////////////////////////////////////////////////////////////////////////////////

#ifndef IVERTEXBUFFER_H
#define IVERTEXBUFFER_H

#include "ShaderAPI_defs.h"
#include "core/ppmem.h"

enum EVertexBufferFlags
{
	VERTBUFFER_FLAG_INSTANCEDATA = (1 << 0),	// GetVertexCount becomes Instance Count
};

// index buffer class
class IVertexBuffer
{
public:
	virtual ~IVertexBuffer() {}

	PPMEM_MANAGED_OBJECT();

	// returns size in bytes
	virtual long		GetSizeInBytes() = 0;

	// returns vertex count
	virtual int			GetVertexCount() = 0;

	// retuns stride size
	virtual int			GetStrideSize() = 0;

	// updates buffer without map/unmap operations which are slower
	virtual void		Update(void* data, int size, int offset, bool discard = true) = 0;

	// locks vertex buffer and gives to programmer buffer data
	virtual bool		Lock(int lockOfs, int sizeToLock, void** outdata, bool readOnly) = 0;

	// unlocks buffer
	virtual void		Unlock() = 0;

	// sets vertex buffer flags
	virtual void		SetFlags( int flags ) = 0;
	virtual int			GetFlags() = 0;
};

#endif // IVERTEXBUFFER_H