//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

class CImage;

class ITexture : public RefCountedObject<ITexture, RefCountedKeepPolicy>
{
public:
	struct LockData
	{
		ubyte*	pData;
		int		nPitch;
	};

	// initializes procedural (lockable) texture
	virtual bool				InitProcedural(const SamplerStateParam_t& sampler,
											ETextureFormat format,
											int width, int height, int depth = 1, int arraySize = 1,
											int flags = 0
											) = 0;

	// initializes texture from image array of images
	virtual	bool				Init(const SamplerStateParam_t& sampler, const ArrayCRef<CImage*> images, int flags = 0) = 0;

	// generates a new error texture
	virtual bool				GenerateErrorTexture(int flags = 0) = 0;

	// Name of texture
	virtual const char*			GetName() const = 0;
	virtual int					GetFlags() const = 0;

	// The dimensions of texture
	virtual ETextureFormat		GetFormat() const = 0;
	virtual int					GetWidth() const = 0;
	virtual int					GetHeight() const = 0;
	virtual int					GetMipCount() const = 0;

	// Animated texture props (matsystem usage)
	virtual int					GetAnimationFrameCount() const = 0;
	virtual int					GetAnimationFrame() const = 0;
	virtual void				SetAnimationFrame(int frame) = 0;

	// texture data management
	virtual void				Lock(LockData* pLockData, Rectangle_t* pRect = nullptr, bool discard = false, bool readOnly = false, int level = 0, int cubeFaceId = 0) = 0;
	virtual void				Unlock() = 0;
};