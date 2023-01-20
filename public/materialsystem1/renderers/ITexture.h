//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

class CImage;
enum ETextureFormat;

enum ETextureLockFlags
{
	TEXLOCK_READONLY	= (1 << 0),
	TEXLOCK_DISCARD		= (1 << 1),

	// don't use manually, use constructor
	TEXLOCK_REGION_RECT		= (1 << 2),
	TEXLOCK_REGION_BOX		= (1 << 3),
};

typedef struct SamplerStateParam_s SamplerStateParam_t;

class ITexture : public RefCountedObject<ITexture, RefCountedKeepPolicy>
{
public:
	struct LockInOutData;

	// initializes procedural (lockable) texture
	virtual bool				InitProcedural(const SamplerStateParam_t& sampler,
											ETextureFormat format,
											int width, int height, int depth = 1, int arraySize = 1,
											int flags = 0
											) = 0;

	// initializes texture from image array of images
	virtual	bool				Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0) = 0;

	// generates a new error texture
	virtual bool				GenerateErrorTexture(int flags = 0) = 0;

	// Name of texture
	virtual const char*			GetName() const = 0;
	virtual int					GetFlags() const = 0;

	// The dimensions of texture
	virtual ETextureFormat		GetFormat() const = 0;

	virtual int					GetWidth() const = 0;
	virtual int					GetHeight() const = 0;
	virtual int					GetDepth() const = 0;

	virtual int					GetMipCount() const = 0;

	// Animated texture props (matsystem usage)
	virtual int					GetAnimationFrameCount() const = 0;
	virtual int					GetAnimationFrame() const = 0;
	virtual void				SetAnimationFrame(int frame) = 0;

	// texture data management
	virtual bool				Lock(LockInOutData& data) = 0;
	virtual void				Unlock() = 0;
};

using ITexturePtr = CRefPtr<ITexture>;

struct ITexture::LockInOutData
{
	LockInOutData(int lockFlags, int level = 0, int cubeFaceIdx = 0)
		: flags(lockFlags)
	{
	}

	LockInOutData(int lockFlags, const IRectangle & rectangle, int level = 0, int cubeFaceIdx = 0)
		: flags(lockFlags | TEXLOCK_REGION_RECT)
	{
		region.rectangle = rectangle;
	}

	LockInOutData(int lockFlags, const IBoundingBox & box, int level = 0, int cubeFaceIdx = 0)
		: flags(lockFlags | TEXLOCK_REGION_BOX)
	{
		region.box = box;
	}

	~LockInOutData()
	{
		ASSERT_MSG(lockData == nullptr, "CRITICAL - Texture was still locked! Unlock() implementation must set lockData to NULL");
	}

	union LockRegion {
		IRectangle		rectangle;
		IBoundingBox	box;
	} region{};

	operator const bool() const { return lockData != nullptr; }

	void*			lockData{ nullptr };	// the place where you can write the data
	int				lockPitch{ 0 };

	int				flags{ 0 };
	int				level{ 0 };
	int				cubeFaceIdx{ 0 };
};