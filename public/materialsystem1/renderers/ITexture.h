//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "imaging/textureformats.h"

class IGPUCommandRecorder;
class CImage;
using CImagePtr = CRefPtr<CImage>;

struct SamplerStateParams;
enum ETextureFormat : int;

struct TextureDesc;

enum ETextureFlags : int
{
	// texture creation flags
	TEXFLAG_PROGRESSIVE_LODS	= (1 << 0),		// progressive LOD uploading, might improve performance
	TEXFLAG_NULL_ON_ERROR		= (1 << 1),
	TEXFLAG_CUBEMAP				= (1 << 2),		// should create cubemap
	TEXFLAG_IGNORE_QUALITY		= (1 << 3),		// not affected by "r_loadmiplevel", always all mip levels are loaded
	TEXFLAG_STORAGE				= (1 << 4),		// allows storage access (compute)
	TEXFLAG_COPY_SRC			= (1 << 5),		// texture can be used as copy source
	TEXFLAG_COPY_DST			= (1 << 6),		// texture can be used as copy destination
	TEXFLAG_TRANSIENT			= (1 << 7),		// (RenderTarget Only) texture is temporary and not registered in RenderAPI list

	// texture identification flags
	TEXFLAG_RENDERTARGET		= (1 << 16),	// this is a rendertarget texture
};

enum ETextureLockFlags
{
	TEXLOCK_READONLY	= (1 << 0),
	TEXLOCK_DISCARD		= (1 << 1)
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

	TextureExtent() = default;
	TextureExtent(const IVector2D& size, int arraySize = 1)
		: width(size.x)
		, height(size.y)
		, arraySize(arraySize)
	{
	}
	TextureExtent(int width, int height, int arraySize = 1) 
		: width(width)
		, height(height)
		, arraySize(arraySize)
	{
	}

	operator IVector2D() const { return { width, height }; }
	IVector2D xy() const { return { width, height }; }
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
	virtual bool			InitProcedural(const TextureDesc& textureDesc) = 0;

	// initializes texture from image array of images
	virtual	bool			Init(const ArrayCRef<CRefPtr<CImage>> images, const SamplerStateParams& sampler, int flags = 0) = 0;

	// generates a checkerboard texture
	virtual bool			GenerateErrorTexture(int flags = 0) = 0;

	// Name of texture
	virtual const char*		GetName() const = 0;
	virtual int				GetFlags() const = 0;
	virtual ETextureFormat	GetFormat() const = 0;
	virtual TextureExtent	GetSize() const = 0;

	virtual int				GetWidth() const = 0;
	virtual int				GetHeight() const = 0;
	virtual int				GetArraySize() const = 0;

	virtual int				GetMipCount() const = 0;
	virtual int				GetSampleCount() const = 0;

	// texture data management
	virtual bool			Lock(LockInOutData& data) = 0;
	virtual void			Unlock(IGPUCommandRecorder* writeCmdRecorder = nullptr) = 0;

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
	TextureView(const ITexturePtr& texture, int arraySlice = 0)
		: texture(texture), arraySlice(arraySlice)
	{
	}

	operator bool() const { return texture; }

	ITexturePtr	texture;
	int			arraySlice{ 0 };
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

	operator const bool() const { return lockData != nullptr; }

	TextureOrigin	lockOrigin;
	TextureExtent	lockSize;

	void*			lockData{ nullptr };	// the place where you can write the data
	int				lockPitch{ 0 };
	int				lockByteCount{ 0 };

	int				flags{ 0 };
};