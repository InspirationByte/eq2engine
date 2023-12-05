//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

class CImage;
using CImagePtr = CRefPtr<CImage>;

struct SamplerStateParams;
enum ETextureFormat : int;

enum ETextureFlags : int
{
	// texture creation flags
	TEXFLAG_PROGRESSIVE_LODS	= (1 << 0),		// progressive LOD uploading, might improve performance
	TEXFLAG_NULL_ON_ERROR		= (1 << 1),
	TEXFLAG_CUBEMAP				= (1 << 2),		// should create cubemap
	TEXFLAG_IGNORE_QUALITY		= (1 << 3),		// not affected by "r_loadmiplevel", always all mip levels are loaded
	TEXFLAG_SRGB				= (1 << 4),		// texture should be sampled as in sRGB color space
	TEXFLAG_STORAGE				= (1 << 5),		// allows storage access (compute)
	TEXFLAG_COPY_SRC			= (1 << 6),		// texture can be used as copy source
	TEXFLAG_COPY_DST			= (1 << 7),		// texture can be used as copy destination

	// texture identification flags
	TEXFLAG_RENDERTARGET		= (1 << 16),	// this is a rendertarget texture
};

enum ETextureLockFlags
{
	TEXLOCK_READONLY	= (1 << 0),
	TEXLOCK_DISCARD		= (1 << 1)
};

class ITexture : public RefCountedObject<ITexture>
{
public:
	static constexpr const int DEFAULT_VIEW = 0;

	inline static int ViewArraySlice(int index)
	{
		return 1 + static_cast<int>(index);
	}

	struct LockInOutData;

	// initializes procedural (lockable) texture
	virtual bool			InitProcedural(const SamplerStateParams& sampler,
											ETextureFormat format,
											int width, int height, int depth = 1, int arraySize = 1,
											int flags = 0
											) = 0;

	// initializes texture from image array of images
	virtual	bool			Init(const SamplerStateParams& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0) = 0;

	// generates a checkerboard texture
	virtual bool			GenerateErrorTexture(int flags = 0) = 0;

	// Name of texture
	virtual const char*		GetName() const = 0;
	virtual int				GetFlags() const = 0;

	// The dimensions of texture
	virtual ETextureFormat	GetFormat() const = 0;

	virtual int				GetWidth() const = 0;
	virtual int				GetHeight() const = 0;
	virtual int				GetArraySize() const = 0;

	virtual int				GetMipCount() const = 0;

	// texture data management
	virtual bool			Lock(LockInOutData& data) = 0;
	virtual void			Unlock() = 0;

	// FIXME: remove?
	virtual const SamplerStateParams& GetSamplerState() const = 0;

	// DEPRECATED
	virtual int				GetAnimationFrameCount() const = 0;
	virtual int				GetAnimationFrame() const = 0;
	virtual void			SetAnimationFrame(int frame) = 0;


};
using ITexturePtr = CRefPtr<ITexture>;

struct TextureView
{
	TextureView() = default;
	TextureView(ITexture* texture, int arraySlice = 0)
		: texture(texture), arraySlice(arraySlice)
	{
	}
	TextureView(ITexturePtr texture, int arraySlice = 0)
		: texture(texture), arraySlice(arraySlice)
	{
	}

	operator bool() const { return texture; }

	ITexturePtr	texture;
	int			arraySlice{ 0 };
};

struct TextureOrigin
{
	int		x{ 0 };
	int		y{ 0 };
	int		arraySlice{ 0 };
	int		mipLevel{ 0 };
};

struct TextureExtent
{
	int		width{ -1 };
	int		height{ -1 };
	int		arraySize{ 1 };
};

struct ITexture::LockInOutData
{
	LockInOutData(int lockFlags, const IAARectangle& rectangle, int mipLevel = 0)
		: flags(lockFlags)
	{
		lockOrigin.x = rectangle.leftTop.x;
		lockOrigin.y = rectangle.leftTop.y;
		lockOrigin.arraySlice = 0;
		lockOrigin.mipLevel = mipLevel;

		lockSize.width = rectangle.rightBottom.x - lockOrigin.x;
		lockSize.height = rectangle.rightBottom.y - lockOrigin.y;
		lockSize.arraySize = 1;
	}

	LockInOutData(int lockFlags, const IBoundingBox& box, int mipLevel = 0)
		: flags(lockFlags)
	{
		lockOrigin.x = box.minPoint.x;
		lockOrigin.y = box.minPoint.y;
		lockOrigin.arraySlice = box.minPoint.z;
		lockOrigin.mipLevel = mipLevel;

		lockSize.width = box.maxPoint.x - lockOrigin.x;
		lockSize.height = box.maxPoint.y - lockOrigin.y;
		lockSize.arraySize = box.maxPoint.z - lockOrigin.arraySlice;
	}

	~LockInOutData()
	{
		ASSERT_MSG(lockData == nullptr, "CRITICAL - Texture was still locked! Unlock() implementation must set lockData to NULL");
	}

	TextureOrigin	lockOrigin;
	TextureExtent	lockSize;

	operator const bool() const { return lockData != nullptr; }

	void*			lockData{ nullptr };	// the place where you can write the data
	int				lockPitch{ 0 };
	int				lockByteCount{ 0 };

	int				flags{ 0 };
};