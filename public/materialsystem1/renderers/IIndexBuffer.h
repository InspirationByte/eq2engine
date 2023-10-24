//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Middle-Level rendering API (ShaderAPI)
//				Index Buffer interface declaration
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IIndexBuffer
{
public:
	virtual ~IIndexBuffer() = default;

	virtual int		GetSizeInBytes() const = 0;

	virtual int		GetIndexSize() const = 0;
	virtual int		GetIndicesCount() const = 0;

	// updates buffer without map/unmap operations which are slower
	virtual void	Update(void* data, int size, int offset = 0) = 0;

	// locks index buffer and gives to programmer buffer data
	virtual bool	Lock(int lockOfs, int sizeToLock, void** outdata, int flags) = 0;
	virtual void	Unlock() = 0;
};
