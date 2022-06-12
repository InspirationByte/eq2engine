//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

class ITexture : public RefCountedObject<ITexture>
{
public:
	struct LockData
	{
		ubyte*	pData;
		int		nPitch;
	};

	// Name of texture
	virtual const char*		GetName() const = 0;
	virtual int				GetFlags() const = 0;

	// The dimensions of texture
	virtual ETextureFormat	GetFormat() const = 0;
	virtual int				GetWidth() const = 0;
	virtual int				GetHeight() const = 0;
	virtual int				GetMipCount() const = 0;

	// Animated texture props (matsystem usage)
	virtual int				GetAnimationFrameCount() const = 0;
	virtual int				GetAnimationFrame() const = 0;
	virtual void			SetAnimationFrame(int frame) = 0;

	// texture data management
	virtual void			Lock(LockData* pLockData, Rectangle_t* pRect = nullptr, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0) = 0;
	virtual void			Unlock() = 0;
};