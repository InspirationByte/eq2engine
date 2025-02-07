//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Image loader
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "math/Vector.h"

#define STBI_NO_STDIO
#define STBI_ASSERT ASSERT
#define STBI_MALLOC PPAlloc
#define STBI_REALLOC PPReAlloc
#define STBI_FREE PPFree
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_TGA		// we have our okay-ish TGA reader and writer
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PNM
#define STBI_NO_PIC
#define STBI_NO_HDR
#include <stb_image.h>

#define STBI_WRITE_NO_STDIO
#define STBIW_ASSERT ASSERT
#define STBIW_MALLOC PPAlloc
#define STBIW_REALLOC PPReAlloc
#define STBIW_FREE PPFree
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "ImageLoader.h"

static stbi_io_callbacks STBImageCallbacks = {
	[](void* user,char* data,int size)->int {
		return reinterpret_cast<IFile*>(user)->Read(data, size, 1);
	},
	[](void* user,int n) {
		reinterpret_cast<IFile*>(user)->Seek(n, VS_SEEK_CUR); 
	},
	[](void* user)->int {
		return reinterpret_cast<IFile*>(user)->Tell() >= reinterpret_cast<IFile*>(user)->GetSize(); 
	}
};

static void STBWriteFunc(void* context, void* data, int size)
{
	reinterpret_cast<IFile*>(context)->Write(data, 1, size);
};

#pragma pack (push, 1)

#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040

#define DDSD_CAPS        0x00000001
#define DDSD_HEIGHT      0x00000002
#define DDSD_WIDTH       0x00000004
#define DDSD_PITCH       0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE  0x00080000
#define DDSD_DEPTH       0x00800000

#define DDSCAPS_COMPLEX  0x00000008
#define DDSCAPS_TEXTURE  0x00001000
#define DDSCAPS_MIPMAP   0x00400000

#define DDSCAPS2_CUBEMAP 0x00000200
#define DDSCAPS2_VOLUME  0x00200000

#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDSCAPS2_CUBEMAP_ALL_FACES (DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ)

#define D3D10_RESOURCE_MISC_TEXTURECUBE 0x4
#define D3D10_RESOURCE_DIMENSION_BUFFER    1
#define D3D10_RESOURCE_DIMENSION_TEXTURE1D 2
#define D3D10_RESOURCE_DIMENSION_TEXTURE2D 3
#define D3D10_RESOURCE_DIMENSION_TEXTURE3D 4

enum D3D10_DXGI_FORMAT 
{
	DXGI_FORMAT_UNKNOWN = 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
	DXGI_FORMAT_R32G32B32A32_UINT = 3,
	DXGI_FORMAT_R32G32B32A32_SINT = 4,
	DXGI_FORMAT_R32G32B32_TYPELESS = 5,
	DXGI_FORMAT_R32G32B32_FLOAT = 6,
	DXGI_FORMAT_R32G32B32_UINT = 7,
	DXGI_FORMAT_R32G32B32_SINT = 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
	DXGI_FORMAT_R16G16B16A16_UNORM = 11,
	DXGI_FORMAT_R16G16B16A16_UINT = 12,
	DXGI_FORMAT_R16G16B16A16_SNORM = 13,
	DXGI_FORMAT_R16G16B16A16_SINT = 14,
	DXGI_FORMAT_R32G32_TYPELESS = 15,
	DXGI_FORMAT_R32G32_FLOAT = 16,
	DXGI_FORMAT_R32G32_UINT = 17,
	DXGI_FORMAT_R32G32_SINT = 18,
	DXGI_FORMAT_R32G8X24_TYPELESS = 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
	DXGI_FORMAT_R10G10B10A2_UNORM = 24,
	DXGI_FORMAT_R10G10B10A2_UINT = 25,
	DXGI_FORMAT_R11G11B10_FLOAT = 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
	DXGI_FORMAT_R8G8B8A8_UNORM = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	DXGI_FORMAT_R8G8B8A8_UINT = 30,
	DXGI_FORMAT_R8G8B8A8_SNORM = 31,
	DXGI_FORMAT_R8G8B8A8_SINT = 32,
	DXGI_FORMAT_R16G16_TYPELESS = 33,
	DXGI_FORMAT_R16G16_FLOAT = 34,
	DXGI_FORMAT_R16G16_UNORM = 35,
	DXGI_FORMAT_R16G16_UINT = 36,
	DXGI_FORMAT_R16G16_SNORM = 37,
	DXGI_FORMAT_R16G16_SINT = 38,
	DXGI_FORMAT_R32_TYPELESS = 39,
	DXGI_FORMAT_D32_FLOAT = 40,
	DXGI_FORMAT_R32_FLOAT = 41,
	DXGI_FORMAT_R32_UINT = 42,
	DXGI_FORMAT_R32_SINT = 43,
	DXGI_FORMAT_R24G8_TYPELESS = 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
	DXGI_FORMAT_R8G8_TYPELESS = 48,
	DXGI_FORMAT_R8G8_UNORM = 49,
	DXGI_FORMAT_R8G8_UINT = 50,
	DXGI_FORMAT_R8G8_SNORM = 51,
	DXGI_FORMAT_R8G8_SINT = 52,
	DXGI_FORMAT_R16_TYPELESS = 53,
	DXGI_FORMAT_R16_FLOAT = 54,
	DXGI_FORMAT_D16_UNORM = 55,
	DXGI_FORMAT_R16_UNORM = 56,
	DXGI_FORMAT_R16_UINT = 57,
	DXGI_FORMAT_R16_SNORM = 58,
	DXGI_FORMAT_R16_SINT = 59,
	DXGI_FORMAT_R8_TYPELESS = 60,
	DXGI_FORMAT_R8_UNORM = 61,
	DXGI_FORMAT_R8_UINT = 62,
	DXGI_FORMAT_R8_SNORM = 63,
	DXGI_FORMAT_R8_SINT = 64,
	DXGI_FORMAT_A8_UNORM = 65,
	DXGI_FORMAT_R1_UNORM = 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
	DXGI_FORMAT_BC1_TYPELESS = 70,
	DXGI_FORMAT_BC1_UNORM = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	DXGI_FORMAT_BC2_TYPELESS = 73,
	DXGI_FORMAT_BC2_UNORM = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	DXGI_FORMAT_BC3_TYPELESS = 76,
	DXGI_FORMAT_BC3_UNORM = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	DXGI_FORMAT_BC4_TYPELESS = 79,
	DXGI_FORMAT_BC4_UNORM = 80,
	DXGI_FORMAT_BC4_SNORM = 81,
	DXGI_FORMAT_BC5_TYPELESS = 82,
	DXGI_FORMAT_BC5_UNORM = 83,
	DXGI_FORMAT_BC5_SNORM = 84,
	DXGI_FORMAT_B5G6R5_UNORM = 85,
	DXGI_FORMAT_B5G5R5A1_UNORM = 86,
	DXGI_FORMAT_B8G8R8A8_UNORM = 87,
	DXGI_FORMAT_B8G8R8X8_UNORM = 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
	DXGI_FORMAT_BC6H_TYPELESS = 94,
	DXGI_FORMAT_BC6H_UF16 = 95,
	DXGI_FORMAT_BC6H_SF16 = 96,
	DXGI_FORMAT_BC7_TYPELESS = 97,
	DXGI_FORMAT_BC7_UNORM = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB = 99,
	DXGI_FORMAT_AYUV = 100,
	DXGI_FORMAT_Y410 = 101,
	DXGI_FORMAT_Y416 = 102,
	DXGI_FORMAT_NV12 = 103,
	DXGI_FORMAT_P010 = 104,
	DXGI_FORMAT_P016 = 105,
	DXGI_FORMAT_420_OPAQUE = 106,
	DXGI_FORMAT_YUY2 = 107,
	DXGI_FORMAT_Y210 = 108,
	DXGI_FORMAT_Y216 = 109,
	DXGI_FORMAT_NV11 = 110,
	DXGI_FORMAT_AI44 = 111,
	DXGI_FORMAT_IA44 = 112,
	DXGI_FORMAT_P8 = 113,
	DXGI_FORMAT_A8P8 = 114,
	DXGI_FORMAT_B4G4R4A4_UNORM = 115,
	DXGI_FORMAT_P208 = 130,
	DXGI_FORMAT_V208 = 131,
	DXGI_FORMAT_V408 = 132,
	DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
	DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
	DXGI_FORMAT_FORCE_UINT = 0xffffffff
};

struct DDSHeader
{
	uint32 dwMagic;
	uint32 dwSize;
	uint32 dwFlags;
	uint32 dwHeight;
	uint32 dwWidth;
	uint32 dwPitchOrLinearSize;
	uint32 dwDepth;
	uint32 dwMipMapCount;
	uint32 dwReserved[11];

	struct
	{
		uint32 dwSize;
		uint32 dwFlags;
		uint32 dwFourCC;
		uint32 dwRGBBitCount;
		uint32 dwRBitMask;
		uint32 dwGBitMask;
		uint32 dwBBitMask;
		uint32 dwRGBAlphaBitMask;
	} ddpfPixelFormat;

	struct
	{
		uint32 dwCaps1;
		uint32 dwCaps2;
		uint32 Reserved[2];
	} ddsCaps;

	uint32 dwReserved2;
};

struct DDSHeaderDXT10
{
	uint32 dxgiFormat;
	uint32 resourceDimension;
	uint32 miscFlag;
	uint32 arraySize;
	uint32 reserved;
};

struct TGAHeader
{
	uint8  descriptionlen;
	uint8  cmaptype;
	uint8  imagetype;
	uint16 cmapstart;
	uint16 cmapentries;
	uint8  cmapbits;
	uint16 xoffset;
	uint16 yoffset;
	uint16 width;
	uint16 height;
	uint8  bpp;
	uint8  attrib;
};

#pragma pack (pop)


/* ---------------------------------------------- */

struct FormatString
{
	ETextureFormat	format;
	EqStringRef		name;
};

static const FormatString formatStrings[] = {
	{ FORMAT_NONE,   "NONE"  },

	{ FORMAT_R8,     "R8"    },
	{ FORMAT_RG8,    "RG8"   },
	{ FORMAT_RGB8,   "RGB8"  },
	{ FORMAT_RGBA8,  "RGBA8" },

	{ FORMAT_R16,    "R16"   },
	{ FORMAT_RG16,   "RG16"  },
	{ FORMAT_RGB16,  "RGB16" },
	{ FORMAT_RGBA16, "RGBA16"},

	{ FORMAT_R16F,   "R16F"    },
	{ FORMAT_RG16F,  "RG16F"   },
	{ FORMAT_RGB16F, "RGB16F"  },
	{ FORMAT_RGBA16F,"RGBA16F" },

	{ FORMAT_R32F,   "R32F"    },
	{ FORMAT_RG32F,  "RG32F"   },
	{ FORMAT_RGB32F, "RGB32F"  },
	{ FORMAT_RGBA32F,"RGBA32F" },

	{ FORMAT_RGBE8,  "RGBE8"   },
	{ FORMAT_RGB565, "RGB565"  },
	{ FORMAT_RGBA4,  "RGBA4"   },
	{ FORMAT_RGB10A2,"RGB10A2" },

	{ FORMAT_DXT1,   "DXT1"  },
	{ FORMAT_DXT3,   "DXT3"  },
	{ FORMAT_DXT5,   "DXT5"  },
	{ FORMAT_ATI1N,  "ATI1N" },
	{ FORMAT_ATI2N,  "ATI2N" },
};

const char* GetFormatString(const ETextureFormat format)
{
	for (unsigned int i = 0; i < elementsOf(formatStrings); i++) {
		if (format == formatStrings[i].format) 
			return formatStrings[i].name;
	}
	return nullptr;
}

ETextureFormat GetFormatFromString(const char* string)
{
	for (unsigned int i = 0; i < elementsOf(formatStrings); i++) {
		if (!formatStrings[i].name.CompareCaseIns(string))
			return formatStrings[i].format;
	}
	return FORMAT_NONE;
}


template <typename DATA_TYPE>
inline void _SwapChannels(DATA_TYPE* pixels, int nPixels, const int channels, const int ch0, const int ch1)
{
	do
	{
		QuickSwap<DATA_TYPE>(pixels[ch0], pixels[ch1]);
		pixels += channels;
	} while (--nPixels);
}


/* ---------------------------------------------- */

CImage::CImage()
{
	m_pPixels = nullptr;
	m_nWidth = 0;
	m_nHeight = 0;
	m_nDepth = 0;
	m_nMipMaps = 0;
	m_nArraySize = 0;
	m_nFormat = FORMAT_NONE;

	m_nExtraDataSize = 0;
	m_pExtraData = nullptr;
}

CImage::CImage(const CImage& img)
{
	m_nWidth = img.m_nWidth;
	m_nHeight = img.m_nHeight;
	m_nDepth = img.m_nDepth;
	m_nMipMaps = img.m_nMipMaps;
	m_nArraySize = img.m_nArraySize;
	m_nFormat = img.m_nFormat;

	int size = GetMipMappedSize(0, m_nMipMaps) * m_nArraySize;
	m_pPixels = PPNew ubyte[size];
	memcpy(m_pPixels, img.m_pPixels, size);

	m_nExtraDataSize = img.m_nExtraDataSize;
	if (m_nExtraDataSize)
	{
		m_pExtraData = PPNew ubyte[m_nExtraDataSize];
		memcpy(m_pExtraData, img.m_pExtraData, m_nExtraDataSize);
	}
	else
		m_pExtraData = nullptr;
}

CImage::~CImage()
{
	Free();
}

ubyte* CImage::Create(const ETextureFormat fmt, const int w, const int h, const int d, const int mipMapCount, const int arraysize)
{
	Free();

	m_nFormat = fmt;
	m_nWidth = w;
	m_nHeight = h;
	m_nDepth = d;
	m_nMipMaps = mipMapCount;
	m_nArraySize = arraysize;

	return (m_pPixels = PPNew ubyte[GetMipMappedSize(0, m_nMipMaps) * m_nArraySize]);
}

void CImage::Free()
{
	SAFE_DELETE_ARRAY(m_pPixels);
	SAFE_DELETE_ARRAY(m_pExtraData);

	m_pExtraData = nullptr;
}

void CImage::Clear()
{
	Free();

	m_nWidth = 0;
	m_nHeight = 0;
	m_nDepth = 0;
	m_nMipMaps = 0;
	m_nArraySize = 0;
	m_nFormat = FORMAT_NONE;
	m_nExtraDataSize = 0;
}

ubyte* CImage::GetPixels(const int mipMapLevel) const
{
	return (mipMapLevel < m_nMipMaps) ? m_pPixels + GetMipMappedSize(0, mipMapLevel) : nullptr;
}

ubyte* CImage::GetPixels(const int mipMapLevel, const int arraySlice) const
{
	if (mipMapLevel >= m_nMipMaps || arraySlice >= m_nArraySize) return nullptr;

	return m_pPixels + GetMipMappedSize(0, m_nMipMaps) * arraySlice + GetMipMappedSize(0, mipMapLevel);
}


int CImage::GetMipMapCountFromDimesions() const
{
	int m = max(m_nWidth, m_nHeight);
	m = max(m, m_nDepth);

	int i = 0;
	while (m > 0)
	{
		m >>= 1;
		i++;
	}

	return i;
}

int CImage::GetMipMappedSize(const int firstMipMapLevel, int nMipMapLevels, ETextureFormat srcFormat) const
{
	int w = GetWidth(firstMipMapLevel);
	int h = GetHeight(firstMipMapLevel);
	int d = GetDepth(firstMipMapLevel);

	if (srcFormat == FORMAT_NONE)
		srcFormat = m_nFormat;

	int size = 0;
	while (nMipMapLevels)
	{
		if (IsCompressedFormat(srcFormat))
			size += ((w + 3) >> 2) * ((h + 3) >> 2) * d;
		else
			size += w * h * d;

		w >>= 1;
		h >>= 1;
		d >>= 1;
		if (w + h + d == 0) break;
		if (w == 0) w = 1;
		if (h == 0) h = 1;
		if (d == 0) d = 1;

		nMipMapLevels--;
	}

	if (IsCompressedFormat(srcFormat))
		size *= GetBytesPerBlock(srcFormat);
	else
		size *= GetBytesPerPixel(srcFormat);

	return (m_nDepth == IMAGE_DEPTH_CUBEMAP) ? (6 * size) : size;
}

int CImage::GetSliceSize(const int mipMapLevel, ETextureFormat srcFormat) const
{
	int w = GetWidth(mipMapLevel);
	int h = GetHeight(mipMapLevel);

	if (srcFormat == FORMAT_NONE)
		srcFormat = m_nFormat;

	int size;
	if (IsCompressedFormat(srcFormat))
		size = ((w + 3) >> 2) * ((h + 3) >> 2) * GetBytesPerBlock(srcFormat);
	else
		size = w * h * GetBytesPerPixel(srcFormat);

	return size;
}

int CImage::GetPixelCount(const int firstMipMapLevel, int nMipMapLevels) const
{
	int w = GetWidth(firstMipMapLevel);
	int h = GetHeight(firstMipMapLevel);
	int d = GetDepth(firstMipMapLevel);

	int size = 0;

	while (nMipMapLevels)
	{
		size += w * h * d;
		w >>= 1;
		h >>= 1;
		d >>= 1;
		if (w + h + d == 0) break;
		if (w == 0) w = 1;
		if (h == 0) h = 1;
		if (d == 0) d = 1;

		nMipMapLevels--;
	}

	return (m_nDepth == IMAGE_DEPTH_CUBEMAP) ? (6 * size) : size;
}

int CImage::GetWidth(const int mipMapLevel) const
{
	int a = m_nWidth >> mipMapLevel;
	return (a == 0) ? 1 : a;
}

int CImage::GetHeight(const int mipMapLevel) const
{
	int a = m_nHeight >> mipMapLevel;
	return (a == 0) ? 1 : a;
}

int CImage::GetDepth(const int mipMapLevel) const
{
	int a = m_nDepth >> mipMapLevel;
	return (a == 0) ? 1 : a;
}

EImageType CImage::GetImageType() const
{
	if (Is1D())
		return IMAGE_TYPE_1D;

	if (Is2D())
		return IMAGE_TYPE_2D;

	if (Is3D())
		return IMAGE_TYPE_3D;

	if (IsCube())
		return IMAGE_TYPE_CUBE;

	return IMAGE_TYPE_INVALID;
}

bool CImage::LoadDDS(IFilePtr fileHandle, uint flags)
{
	if (!fileHandle)
		return false;

	DDSHeader header;
	fileHandle->Read(&header, sizeof(header), 1);

	if (header.dwMagic != MAKECHAR4('D', 'D', 'S', ' '))
	{
		MsgError("This image is not Direct Draw Surface!\n");
		return false;
	}

	m_nWidth = header.dwWidth;
	m_nHeight = header.dwHeight;
	m_nDepth = (header.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP) ? IMAGE_DEPTH_CUBEMAP : (header.dwDepth == 0) ? 1 : header.dwDepth;
	m_nMipMaps = ((flags & DONT_LOAD_MIPMAPS) || (header.dwMipMapCount == 0)) ? 1 : header.dwMipMapCount;
	m_nArraySize = 1;

	if (header.ddpfPixelFormat.dwFourCC == MAKECHAR4('D', 'X', '1', '0'))
	{
		DDSHeaderDXT10 dxt10Header;
		fileHandle->Read(&dxt10Header, sizeof(dxt10Header), 1);

		switch (dxt10Header.dxgiFormat)
		{
		case DXGI_FORMAT_R8_UNORM: m_nFormat = FORMAT_R8; break;
		case DXGI_FORMAT_R8G8_UNORM: m_nFormat = FORMAT_RG8; break;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM: m_nFormat = FORMAT_RGBA8; break;
		case DXGI_FORMAT_R16_UNORM: m_nFormat = FORMAT_R16; break;
		case DXGI_FORMAT_R16G16_UNORM: m_nFormat = FORMAT_RG16; break;
		case DXGI_FORMAT_R16G16B16A16_UNORM: m_nFormat = FORMAT_RGBA16; break;
		case DXGI_FORMAT_R16_FLOAT: m_nFormat = FORMAT_R16F; break;
		case DXGI_FORMAT_R16G16_FLOAT: m_nFormat = FORMAT_RG16F; break;
		case DXGI_FORMAT_R16G16B16A16_FLOAT: m_nFormat = FORMAT_RGBA16F; break;
		case DXGI_FORMAT_R32_FLOAT: m_nFormat = FORMAT_R32F; break;
		case DXGI_FORMAT_R32G32_FLOAT: m_nFormat = FORMAT_RG32F; break;
		case DXGI_FORMAT_R32G32B32_FLOAT:  m_nFormat = FORMAT_RGB32F; break;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:  m_nFormat = FORMAT_RGBA32F; break;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP: m_nFormat = FORMAT_RGB9E5; break;
		case DXGI_FORMAT_R11G11B10_FLOAT: m_nFormat = FORMAT_RG11B10F; break;
		case DXGI_FORMAT_R10G10B10A2_UNORM: m_nFormat = FORMAT_RGB10A2; break;
		case DXGI_FORMAT_BC1_UNORM: m_nFormat = FORMAT_DXT1; break;
		case DXGI_FORMAT_BC2_UNORM: m_nFormat = FORMAT_DXT3; break;
		case DXGI_FORMAT_BC3_UNORM: m_nFormat = FORMAT_DXT5; break;
		case DXGI_FORMAT_BC4_UNORM: m_nFormat = FORMAT_ATI1N; break;
		case DXGI_FORMAT_BC5_UNORM: m_nFormat = FORMAT_ATI2N; break;
		case 0:
			m_nFormat = FORMAT_NONE;
			MsgError("Invalid DDS file %s\n", GetName());
			break;
		default:
			MsgError("Image %s has unknown or invalid DXGI format %d\n", GetName(), dxt10Header.dxgiFormat);
			return false;
		}

		m_nArraySize = dxt10Header.arraySize;
		if (dxt10Header.miscFlag & D3D10_RESOURCE_MISC_TEXTURECUBE)
		{
			ASSERT(IsCube());
		}
		if (dxt10Header.resourceDimension == D3D10_RESOURCE_DIMENSION_TEXTURE1D)
		{
			ASSERT(m_nDepth == 1 && m_nHeight == 1);
		}
	}
	else
	{
		switch (header.ddpfPixelFormat.dwFourCC) {
			case 34:  m_nFormat = FORMAT_RG16; break;
			case 36:  m_nFormat = FORMAT_RGBA16; break;
			case 111: m_nFormat = FORMAT_R16F; break;
			case 112: m_nFormat = FORMAT_RG16F; break;
			case 113: m_nFormat = FORMAT_RGBA16F; break;
			case 114: m_nFormat = FORMAT_R32F; break;
			case 115: m_nFormat = FORMAT_RG32F; break;
			case 116: m_nFormat = FORMAT_RGBA32F; break;
			case MAKECHAR4('D', 'X', 'T', '1'): m_nFormat = FORMAT_DXT1; break;
			case MAKECHAR4('D', 'X', 'T', '3'): m_nFormat = FORMAT_DXT3; break;
			case MAKECHAR4('D', 'X', 'T', '5'): m_nFormat = FORMAT_DXT5; break;
			case MAKECHAR4('A', 'T', 'I', '1'): m_nFormat = FORMAT_ATI1N; break;
			case MAKECHAR4('A', 'T', 'I', '2'): m_nFormat = FORMAT_ATI2N; break;
			case MAKECHAR4('E', 'T', 'C', '1'): m_nFormat = FORMAT_ETC1; break;
			case MAKECHAR4('E', 'T', 'C', '2'): m_nFormat = FORMAT_ETC2; break;
			case MAKECHAR4('E', 'T', 'C', 'P'): m_nFormat = FORMAT_ETC2A1; break;
			case MAKECHAR4('E', 'T', 'C', 'A'): m_nFormat = FORMAT_ETC2A8; break;
			default:
				switch (header.ddpfPixelFormat.dwRGBBitCount)
				{
				case 8: m_nFormat = FORMAT_I8; break;
				case 16:
					m_nFormat = (header.ddpfPixelFormat.dwRGBAlphaBitMask == 0xF000) ? FORMAT_RGBA4 :
						(header.ddpfPixelFormat.dwRGBAlphaBitMask == 0xFF00) ? FORMAT_IA8 :
						(header.ddpfPixelFormat.dwBBitMask == 0x1F) ? FORMAT_RGB565 : FORMAT_I16;
					break;
				case 24: m_nFormat = FORMAT_RGB8; break;
				case 32:
					m_nFormat = (header.ddpfPixelFormat.dwRBitMask == 0x3FF00000) ? FORMAT_RGB10A2 : FORMAT_RGBA8;
					break;
				default:
					MsgError("Image %s has unknown format.\n", GetName());
					return false;
				}
		}
	}

	const int size = GetMipMappedSize(0, m_nMipMaps) * m_nArraySize;
	m_pPixels = PPNew ubyte[size];

	for (int arrIdx = 0; arrIdx < m_nArraySize; ++arrIdx)
	{
		if (IsCube())
		{
			for (int face = 0; face < 6; face++)
			{
				for (int mipMapLevel = 0; mipMapLevel < m_nMipMaps; mipMapLevel++)
				{
					const int faceSize = GetMipMappedSize(mipMapLevel, 1) / 6;
					ubyte* src = GetPixels(mipMapLevel, arrIdx) + face * faceSize;

					fileHandle->Read(src, 1, faceSize);
				}

				if ((flags & DONT_LOAD_MIPMAPS) && header.dwMipMapCount > 1)
				{
					fileHandle->Seek(GetMipMappedSize(1, header.dwMipMapCount - 1) / 6, VS_SEEK_CUR);
				}
			}
		}
		else
			fileHandle->Read(m_pPixels, 1, size);
	}

	if ((m_nFormat == FORMAT_RGB8 || m_nFormat == FORMAT_RGBA8) && header.ddpfPixelFormat.dwBBitMask == 0xFF)
	{
		int nChannels = GetChannelCount(m_nFormat);
		_SwapChannels(m_pPixels, size / nChannels, nChannels, 0, 2);
	}
	return true;
}

bool CImage::Load(IFilePtr fileHandle)
{
	int numComponents;
	stbi_uc* imgData = stbi_load_from_callbacks(&STBImageCallbacks, fileHandle.Ptr(), &m_nWidth, &m_nHeight, &numComponents, 0);
	if (!imgData)
	{
		MsgError("%s\n", stbi_failure_reason());
		return false;
	}

	switch (numComponents)
	{
	case 1:
		m_nFormat = FORMAT_I8;
		break;
	case 3:
		m_nFormat = FORMAT_RGB8;
		break;
	case 4:
		m_nFormat = FORMAT_RGBA8;
		break;
	}

	m_nDepth = 1;
	m_nMipMaps = 1;
	m_nArraySize = 1;

	m_pPixels = PPNew ubyte[m_nWidth * m_nHeight * numComponents];
	memcpy(m_pPixels, imgData, m_nWidth * m_nHeight * numComponents);

	stbi_image_free(imgData);

	return true;
}

bool CImage::LoadTGA(IFilePtr fileHandle)
{
#ifdef NO_TGA
	return false;
#else
	TGAHeader header;

	int size, x, y, pixelSize, palLength;
	ubyte* tempBuffer, * fBuffer, * dest, * src;
	uint tempPixel;
	ubyte palette[768];

	// Find file size
	size = fileHandle->GetSize();

	// Read the header
	fileHandle->Read(&header, sizeof(header), 1);

	m_nWidth = header.width;
	m_nHeight = header.height;
	m_nDepth = 1;
	m_nMipMaps = 1;
	m_nArraySize = 1;

	pixelSize = header.bpp / 8;

	if ((palLength = header.descriptionlen + header.cmapentries * header.cmapbits / 8) > 0)
		fileHandle->Read(palette, sizeof(palette), 1);

	// Read the file data
	fBuffer = PPNew ubyte[size - sizeof(header) - palLength];
	fileHandle->Read(fBuffer, size - sizeof(header) - palLength, 1);
	fileHandle = nullptr;

	size = m_nWidth * m_nHeight * pixelSize;

	tempBuffer = PPNew ubyte[size];

	// Decode if rle compressed. Bit 3 of .imagetype tells if the file is compressed
	if (header.imagetype & 0x08)
	{
		uint c, count;

		dest = tempBuffer;
		src = fBuffer;

		while (size > 0)
		{
			// Get packet header
			c = *src++;

			count = (c & 0x7f) + 1;
			size -= count * pixelSize;

			if (c & 0x80)
			{
				// Rle packet
				do
				{
					memcpy(dest, src, pixelSize);
					dest += pixelSize;
				} while (--count);

				src += pixelSize;
			}
			else
			{
				// Raw packet
				count *= pixelSize;
				memcpy(dest, src, count);
				src += count;
				dest += count;
			}
		}

		src = tempBuffer;
	}
	else
		src = fBuffer;

	src += (header.bpp / 8) * m_nWidth * (m_nHeight - 1);

	switch (header.bpp)
	{
	case 8:
		if (palLength > 0)
		{
			m_nFormat = FORMAT_RGB8;
			dest = m_pPixels = PPNew ubyte[m_nWidth * m_nHeight * 3];
			for (y = 0; y < m_nHeight; y++)
			{
				for (x = 0; x < m_nWidth; x++)
				{
					tempPixel = 3 * (*src++);
					*dest++ = palette[tempPixel + 2];
					*dest++ = palette[tempPixel + 1];
					*dest++ = palette[tempPixel];
				}
				src -= 2 * m_nWidth;
			}
		}
		else
		{
			m_nFormat = FORMAT_I8;
			dest = m_pPixels = PPNew ubyte[m_nWidth * m_nHeight];
			for (y = 0; y < m_nHeight; y++)
			{
				memcpy(dest, src, m_nWidth);
				dest += m_nWidth;
				src -= m_nWidth;
			}
		}
		break;
	case 16:
		m_nFormat = FORMAT_RGBA8;
		dest = m_pPixels = PPNew ubyte[m_nWidth * m_nHeight * 4];
		for (y = 0; y < m_nHeight; y++)
		{
			for (x = 0; x < m_nWidth; x++)
			{
				tempPixel = *((unsigned short*)src);

				dest[0] = ((tempPixel >> 10) & 0x1F) << 3;
				dest[1] = ((tempPixel >> 5) & 0x1F) << 3;
				dest[2] = ((tempPixel) & 0x1F) << 3;
				dest[3] = ((tempPixel >> 15) ? 0xFF : 0);
				dest += 4;
				src += 2;
			}
			src -= 4 * m_nWidth;
		}
		break;
	case 24:
		m_nFormat = FORMAT_RGB8;
		dest = m_pPixels = PPNew ubyte[m_nWidth * m_nHeight * 3];

		for (y = 0; y < m_nHeight; y++)
		{
			for (x = 0; x < m_nWidth; x++)
			{
				*dest++ = src[2];
				*dest++ = src[1];
				*dest++ = src[0];
				src += 3;
			}
			src -= 6 * m_nWidth;
		}
		break;
	case 32:
		m_nFormat = FORMAT_RGBA8;
		dest = m_pPixels = PPNew ubyte[m_nWidth * m_nHeight * 4];
		for (y = 0; y < m_nHeight; y++)
		{
			for (x = 0; x < m_nWidth; x++)
			{
				*dest++ = src[2];
				*dest++ = src[1];
				*dest++ = src[0];
				*dest++ = src[3];
				src += 4;
			}
			src -= 8 * m_nWidth;
		}
		break;
	}

	delete[] tempBuffer;
	delete[] fBuffer;
#endif
 	return true;
 }

bool CImage::Load(const char* fileName, uint flags, int searchFlags)
{
	Clear();

	const EqString extension = fnmPathExtractExt(fileName);
	if (!extension.Length())
		return false;

	SetName(fileName);

	IFilePtr file;
	if (!(file = g_fileSystem->Open(fileName, FS_OPEN_READ, searchFlags)))
		return false;

	if (extension == "dds")
		return LoadDDS(file, flags);
	if (extension == "tga")
		return LoadTGA(file);
	else
		return Load(file);

	return true;
}

bool CImage::SaveDDS(IVirtualStreamPtr fileHandle) const
{
	if (!fileHandle)
		return false;

	// Set up the header
	DDSHeader header;
	memset(&header, 0, sizeof(header));
	DDSHeaderDXT10 headerDXT10;
	memset(&headerDXT10, 0, sizeof(headerDXT10));

	header.dwMagic = MAKECHAR4('D', 'D', 'S', ' ');
	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | (m_nMipMaps > 1 ? DDSD_MIPMAPCOUNT : 0) | (m_nDepth > 1 ? DDSD_DEPTH : 0);
	header.dwHeight = m_nHeight;
	header.dwWidth = m_nWidth;
	header.dwPitchOrLinearSize = 0;
	header.dwDepth = (m_nDepth > 1) ? m_nDepth : 0;
	header.dwMipMapCount = (m_nMipMaps > 1) ? m_nMipMaps : 0;

	const int nChannels = GetChannelCount(m_nFormat);

	header.ddpfPixelFormat.dwSize = 32;
	if (m_nArraySize == 1 && (m_nFormat <= FORMAT_I16 || m_nFormat == FORMAT_RGB10A2))
	{
		header.ddpfPixelFormat.dwFlags = ((nChannels < 3) ? 0x00020000 : DDPF_RGB) | ((nChannels & 1) ? 0 : DDPF_ALPHAPIXELS);
		if (m_nFormat <= FORMAT_RGBA8)
		{
			header.ddpfPixelFormat.dwRGBBitCount = 8 * nChannels;
			header.ddpfPixelFormat.dwRBitMask = (nChannels > 2) ? 0x00FF0000 : 0xFF;
			header.ddpfPixelFormat.dwGBitMask = (nChannels > 1) ? 0x0000FF00 : 0;
			header.ddpfPixelFormat.dwBBitMask = (nChannels > 1) ? 0x000000FF : 0;
			header.ddpfPixelFormat.dwRGBAlphaBitMask = (nChannels == 4) ? 0xFF000000 : (nChannels == 2) ? 0xFF00 : 0;
		}
		else if (m_nFormat == FORMAT_I16)
		{
			header.ddpfPixelFormat.dwRGBBitCount = 16;
			header.ddpfPixelFormat.dwRBitMask = 0xFFFF;
		}
		else
		{
			header.ddpfPixelFormat.dwRGBBitCount = 32;
			header.ddpfPixelFormat.dwRBitMask = 0x3FF00000;
			header.ddpfPixelFormat.dwGBitMask = 0x000FFC00;
			header.ddpfPixelFormat.dwBBitMask = 0x000003FF;
			header.ddpfPixelFormat.dwRGBAlphaBitMask = 0xC0000000;
		}
	}
	else
	{
		header.ddpfPixelFormat.dwFlags = DDPF_FOURCC;

		if (m_nFormat <= FORMAT_RGBA8)
		{
			header.ddpfPixelFormat.dwRGBBitCount = 8 * nChannels;
			header.ddpfPixelFormat.dwRBitMask = (nChannels > 2) ? 0x00FF0000 : 0xFF;
			header.ddpfPixelFormat.dwGBitMask = (nChannels > 1) ? 0x0000FF00 : 0;
			header.ddpfPixelFormat.dwBBitMask = (nChannels > 1) ? 0x000000FF : 0;
			header.ddpfPixelFormat.dwRGBAlphaBitMask = (nChannels == 4) ? 0xFF000000 : (nChannels == 2) ? 0xFF00 : 0;
		}
		else if (m_nFormat == FORMAT_I16)
		{
			header.ddpfPixelFormat.dwRGBBitCount = 16;
			header.ddpfPixelFormat.dwRBitMask = 0xFFFF;
		}

		bool dx10Format = m_nArraySize > 1;
		if (!dx10Format)
		{
			switch (m_nFormat)
			{
			case FORMAT_RG16:    header.ddpfPixelFormat.dwFourCC = 34; break;
			case FORMAT_RGBA16:  header.ddpfPixelFormat.dwFourCC = 36; break;
			case FORMAT_R16F:    header.ddpfPixelFormat.dwFourCC = 111; break;
			case FORMAT_RG16F:   header.ddpfPixelFormat.dwFourCC = 112; break;
			case FORMAT_RGBA16F: header.ddpfPixelFormat.dwFourCC = 113; break;
			case FORMAT_R32F:    header.ddpfPixelFormat.dwFourCC = 114; break;
			case FORMAT_RG32F:   header.ddpfPixelFormat.dwFourCC = 115; break;
			case FORMAT_RGBA32F: header.ddpfPixelFormat.dwFourCC = 116; break;
			case FORMAT_DXT1:    header.ddpfPixelFormat.dwFourCC = MAKECHAR4('D', 'X', 'T', '1'); break;
			case FORMAT_DXT3:    header.ddpfPixelFormat.dwFourCC = MAKECHAR4('D', 'X', 'T', '3'); break;
			case FORMAT_DXT5:    header.ddpfPixelFormat.dwFourCC = MAKECHAR4('D', 'X', 'T', '5'); break;
			case FORMAT_ATI1N:   header.ddpfPixelFormat.dwFourCC = MAKECHAR4('A', 'T', 'I', '1'); break;
			case FORMAT_ATI2N:   header.ddpfPixelFormat.dwFourCC = MAKECHAR4('A', 'T', 'I', '2'); break;
			default:
				dx10Format = true;
			}
		}

		if (dx10Format)
		{
			header.ddpfPixelFormat.dwFourCC = MAKECHAR4('D', 'X', '1', '0');
			headerDXT10.arraySize = m_nArraySize;
			headerDXT10.miscFlag = (m_nDepth == IMAGE_DEPTH_CUBEMAP) ? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;
			headerDXT10.resourceDimension = Is1D() ? D3D10_RESOURCE_DIMENSION_TEXTURE1D : Is3D() ? D3D10_RESOURCE_DIMENSION_TEXTURE3D : D3D10_RESOURCE_DIMENSION_TEXTURE2D;
			switch (m_nFormat)
			{
			case FORMAT_R8:			headerDXT10.dxgiFormat = DXGI_FORMAT_R8_UNORM; break;
			case FORMAT_RG8:		headerDXT10.dxgiFormat = DXGI_FORMAT_R8G8_UNORM; break;
			case FORMAT_RGBA8:		headerDXT10.dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case FORMAT_R16:		headerDXT10.dxgiFormat = DXGI_FORMAT_R16_UNORM; break;
			case FORMAT_RG16:		headerDXT10.dxgiFormat = DXGI_FORMAT_R16G16_UNORM; break;
			case FORMAT_RGBA16:		headerDXT10.dxgiFormat = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			case FORMAT_R16F:		headerDXT10.dxgiFormat = DXGI_FORMAT_R16_FLOAT; break;
			case FORMAT_RG16F:		headerDXT10.dxgiFormat = DXGI_FORMAT_R16G16_FLOAT; break;
			case FORMAT_RGBA16F:	headerDXT10.dxgiFormat = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case FORMAT_R32F:		headerDXT10.dxgiFormat = DXGI_FORMAT_R32_FLOAT; break;
			case FORMAT_RG32F:		headerDXT10.dxgiFormat = DXGI_FORMAT_R32G32_FLOAT; break;
			case FORMAT_RGB32F:		headerDXT10.dxgiFormat = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case FORMAT_RGBA32F:	headerDXT10.dxgiFormat = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case FORMAT_RGB9E5:		headerDXT10.dxgiFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP; break;
			case FORMAT_RG11B10F:	headerDXT10.dxgiFormat = DXGI_FORMAT_R11G11B10_FLOAT; break;
			case FORMAT_RGB10A2:	headerDXT10.dxgiFormat = DXGI_FORMAT_R10G10B10A2_UNORM; break;
			case FORMAT_DXT1:		headerDXT10.dxgiFormat = DXGI_FORMAT_BC1_UNORM; break;
			case FORMAT_DXT3:		headerDXT10.dxgiFormat = DXGI_FORMAT_BC2_UNORM; break;
			case FORMAT_DXT5:		headerDXT10.dxgiFormat = DXGI_FORMAT_BC3_UNORM; break;
			case FORMAT_ATI1N:		headerDXT10.dxgiFormat = DXGI_FORMAT_BC4_UNORM; break;
			case FORMAT_ATI2N:		headerDXT10.dxgiFormat = DXGI_FORMAT_BC5_UNORM; break;
			default:
				return false;
			}
		}
	}

	header.ddsCaps.dwCaps1 = DDSCAPS_TEXTURE | (m_nMipMaps > 1 ? DDSCAPS_MIPMAP | DDSCAPS_COMPLEX : 0) | (m_nDepth != 1 ? DDSCAPS_COMPLEX : 0);
	header.ddsCaps.dwCaps2 = (m_nDepth > 1) ? DDSCAPS2_VOLUME : (m_nDepth == IMAGE_DEPTH_CUBEMAP) ? DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES : 0;
	header.ddsCaps.Reserved[0] = 0;
	header.ddsCaps.Reserved[1] = 0;
	header.dwReserved2 = 0;

	fileHandle->Write(&header, sizeof(header), 1);
	if (headerDXT10.dxgiFormat)
		fileHandle->Write(&headerDXT10, sizeof(headerDXT10), 1);

	const int size = GetMipMappedSize(0, m_nMipMaps);

	const bool flipChannels = m_nArraySize == 1 && (m_nFormat == FORMAT_RGB8 || m_nFormat == FORMAT_RGBA8);

	// RGB to BGR
	if (flipChannels)
		_SwapChannels(m_pPixels, size / nChannels, nChannels, 0, 2);

	for (int arrIdx = 0; arrIdx < m_nArraySize; ++arrIdx)
	{
		if (IsCube())
		{
			for (int face = 0; face < 6; face++)
			{
				for (int mipMapLevel = 0; mipMapLevel < m_nMipMaps; mipMapLevel++)
				{
					const int faceSize = GetMipMappedSize(mipMapLevel, 1) / 6;
					const ubyte* src = GetPixels(mipMapLevel, arrIdx) + face * faceSize;

					fileHandle->Write(src, 1, faceSize);
				}
			}
		}
		else
		{
			fileHandle->Write(m_pPixels + size * arrIdx, size, 1);
		}
	}

	// Restore to RGB
	if (flipChannels)
		_SwapChannels(m_pPixels, size / nChannels, nChannels, 0, 2);

	return true;
}

bool CImage::SaveJPEG(IVirtualStreamPtr fileHandle, const int quality) const
{
	if (m_nFormat != FORMAT_I8 && m_nFormat != FORMAT_RGB8)
		return false;

	const int nChannels = GetChannelCount(m_nFormat);
	stbi_write_jpg_to_func(STBWriteFunc, fileHandle.Ptr(), m_nWidth, m_nHeight, nChannels, m_pPixels, quality);

	return true;
}

bool CImage::SaveTGA(IVirtualStreamPtr fileHandle) const
{
#ifdef NO_TGA
	return false;
#else
	if (!fileHandle)
		return false;

	if (m_nFormat != FORMAT_I8 && m_nFormat != FORMAT_RGB8 && m_nFormat != FORMAT_RGBA8)
		return false;

	int nChannels = GetChannelCount(m_nFormat);

	TGAHeader header = {
		0x00,
		(uint8)((m_nFormat == FORMAT_I8) ? 1 : 0),
		(uint8)((m_nFormat == FORMAT_I8) ? 1 : 2),
		0x0000,
		(uint16)((m_nFormat == FORMAT_I8) ? 256 : 0),
		(uint8)((m_nFormat == FORMAT_I8) ? 24 : 0),
		0x0000,
		0x0000,
		(uint16)m_nWidth,
		(uint16)m_nHeight,
		(uint8)(nChannels * 8),
		0x00
	};

	fileHandle->Write(&header, sizeof(header), 1);

	ubyte* dest, * src, * buffer;
	if (m_nFormat == FORMAT_I8)
	{
		ubyte pal[768];
		int p = 0;
		for (int i = 0; i < 256; i++)
		{
			pal[p++] = i;
			pal[p++] = i;
			pal[p++] = i;
		}
		fileHandle->Write(pal, sizeof(pal), 1);

		src = m_pPixels + m_nWidth * m_nHeight;
		for (int y = 0; y < m_nHeight; y++)
		{
			src -= m_nWidth;
			fileHandle->Write(src, m_nWidth, 1);
		}

	}
	else
	{
		bool useAlpha = (nChannels == 4);
		int lineLength = m_nWidth * (useAlpha ? 4 : 3);

		buffer = PPNew ubyte[m_nHeight * lineLength];
		int len;

		for (int y = 0; y < m_nHeight; y++)
		{
			dest = buffer + (m_nHeight - y - 1) * lineLength;
			src = m_pPixels + y * m_nWidth * nChannels;
			len = m_nWidth;

			do
			{
				*dest++ = src[2];
				*dest++ = src[1];
				*dest++ = src[0];
				if (useAlpha) *dest++ = src[3];
				src += nChannels;
			} while (--len);
		}

		fileHandle->Write(buffer, m_nHeight * lineLength, 1);
		delete[] buffer;
	}
	return true;
#endif // NO_TGA
}

bool CImage::SaveImage(const char* fileName, int searchFlags) const
{
	const EqString extension = fnmPathExtractExt(fileName);
	if (!extension.Length())
		return false;

	IFilePtr file;
	if (!(file = g_fileSystem->Open(fileName, FS_OPEN_WRITE, searchFlags)))
		return false;

	if (extension == "dds") 
	{
		return SaveDDS(file);
	}
	else if (extension == "jpg" || extension == "jpeg")
	{
		return SaveJPEG(file, 75);
	}
	else if (extension == "tga")
	{
		return SaveTGA(file);
	}

	return false;
}

void CImage::LoadFromMemory(void* mem, const ETextureFormat frmt, const int w, const int h, const int d, const int mipMapCount, bool ownsMemory)
{
	Free();

	m_nWidth = w;
	m_nHeight = h;
	m_nDepth = d;
	m_nFormat = frmt;
	m_nMipMaps = mipMapCount;
	m_nArraySize = 1;

	if (ownsMemory)
		m_pPixels = (unsigned char*)mem;
	else
	{
		int size = GetMipMappedSize(0, m_nMipMaps);
		m_pPixels = PPNew unsigned char[size];
		memcpy(m_pPixels, mem, size);
	}
}


template <typename DATA_TYPE>
void BuildMipMap(DATA_TYPE* dst, const DATA_TYPE* src, const uint w, const uint h, const uint d, const uint c)
{
	uint xOff = (w < 2) ? 0 : c;
	uint yOff = (h < 2) ? 0 : c * w;
	uint zOff = (d < 2) ? 0 : c * w * h;

	for (uint z = 0; z < d; z += 2) {
		for (uint y = 0; y < h; y += 2) {
			for (uint x = 0; x < w; x += 2) {
				for (uint i = 0; i < c; i++) {
					*dst++ = (src[0] + src[xOff] + src[yOff] + src[yOff + xOff] + src[zOff] + src[zOff + xOff] + src[zOff + yOff] + src[zOff + yOff + xOff]) / 8;
					src++;
				}
				src += xOff;
			}
			src += yOff;
		}
		src += zOff;
	}
}

bool CImage::CreateMipMaps(const int mipMaps)
{
	if (IsCompressedFormat(m_nFormat))
		return false;

	if (!isPowerOf2(m_nWidth) || !isPowerOf2(m_nHeight) || !isPowerOf2(m_nDepth))
		return false;

	int actualMipMaps = min(mipMaps, GetMipMapCountFromDimesions());

	if (m_nMipMaps != actualMipMaps)
	{
		int size = GetMipMappedSize(0, actualMipMaps);
		if (m_nArraySize > 1)
		{
			ubyte* newPixels = PPNew ubyte[size * m_nArraySize];

			// Copy top mipmap of all array slices to new location
			int firstMipSize = GetMipMappedSize(0, 1);
			int oldSize = GetMipMappedSize(0, m_nMipMaps);

			for (int i = 0; i < m_nArraySize; i++)
			{
				memcpy(newPixels + i * size, m_pPixels + i * oldSize, firstMipSize);
			}

			delete[] m_pPixels;
			m_pPixels = newPixels;
		}
		else
		{
			/*
			unsigned char* newpels  = PPNew unsigned char[size/sizeof(ubyte)];
			memcpy(newpels,pixels,sizeof(newpels));
			delete pixels;
			*/

			m_pPixels = (ubyte*)PPReAlloc(m_pPixels, size);
		}

		m_nMipMaps = actualMipMaps;
	}

	int nChannels = GetChannelCount(m_nFormat);


	int n = IsCube() ? 6 : 1;

	for (int arraySlice = 0; arraySlice < m_nArraySize; arraySlice++)
	{
		ubyte* src = GetPixels(0, arraySlice);
		ubyte* dst = GetPixels(1, arraySlice);

		for (int level = 1; level < m_nMipMaps; level++)
		{
			int w = GetWidth(level - 1);
			int h = GetHeight(level - 1);
			int d = GetDepth(level - 1);

			int srcSize = GetMipMappedSize(level - 1, 1) / n;
			int dstSize = GetMipMappedSize(level, 1) / n;

			for (int i = 0; i < n; i++)
			{
				if (IsPlainFormat(m_nFormat))
				{
					if (IsFloatFormat(m_nFormat))
					{
						BuildMipMap((float*)dst, (float*)src, w, h, d, nChannels);
					}
					else if (m_nFormat >= FORMAT_I16)
					{
						BuildMipMap((ushort*)dst, (ushort*)src, w, h, d, nChannels);
					}
					else
					{
						BuildMipMap(dst, src, w, h, d, nChannels);
					}
				}
				src += srcSize;
				dst += dstSize;
			}
		}
	}

	return true;
}

bool CImage::RemoveMipMaps(const int firstMipMap, int mipMapsToSave)
{
	// UNDONE: mipmap removal on array textures
	if (m_nArraySize > 1) return false;
	if (firstMipMap > m_nMipMaps) return false;

	int newMipCount = min(firstMipMap + mipMapsToSave, m_nMipMaps) - firstMipMap;

	int size = GetMipMappedSize(firstMipMap, newMipCount);
	ubyte* newPixels = PPNew ubyte[size];

	memcpy(newPixels, GetPixels(firstMipMap), size);
	int newWidth = GetWidth(firstMipMap);
	int newHeight = GetHeight(firstMipMap);
	int newDepth = m_nDepth ? GetDepth(firstMipMap) : IMAGE_DEPTH_CUBEMAP;

	delete[] m_pPixels;
	m_pPixels = newPixels;
	m_nWidth = newWidth;
	m_nHeight = newHeight;
	m_nDepth = newDepth;
	m_nMipMaps = newMipCount;

	return true;
}

bool CImage::Convert(const ETextureFormat newFormat)
{
	ubyte* newPixels;
	uint nPixels = GetPixelCount(0, m_nMipMaps) * m_nArraySize;

	if (m_nFormat == FORMAT_RGBE8 && (newFormat == FORMAT_RGB32F || newFormat == FORMAT_RGBA32F))
	{
		newPixels = PPNew ubyte[GetMipMappedSize(0, m_nMipMaps, newFormat) * m_nArraySize];
		float* dest = (float*)newPixels;

		bool writeAlpha = (newFormat == FORMAT_RGBA32F);
		ubyte* src = m_pPixels;
		do
		{
			*((Vector3D*)dest) = rgbeToRGB(src).rgb();
			if (writeAlpha)
			{
				dest[3] = 1.0f;
				dest += 4;
			}
			else
			{
				dest += 3;
			}
			src += 4;
		} while (--nPixels);

	}
	else
	{
		if (!IsPlainFormat(m_nFormat) || !(IsPlainFormat(newFormat) || newFormat == FORMAT_RGB10A2 || newFormat == FORMAT_RGBE8 || newFormat == FORMAT_RGB9E5))
			return false;

		if (m_nFormat == newFormat)
			return true;

		ubyte* src = m_pPixels;
		ubyte* dest = newPixels = PPNew ubyte[GetMipMappedSize(0, m_nMipMaps, newFormat) * m_nArraySize];

		if (m_nFormat == FORMAT_RGB8 && newFormat == FORMAT_RGBA8)
		{
			// Fast path for RGB->RGBA8
			do
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest[3] = 255;
				dest += 4;
				src += 3;
			} while (--nPixels);

		}
		else
		{
			int srcSize = GetBytesPerPixel(m_nFormat);
			int nSrcChannels = GetChannelCount(m_nFormat);

			int destSize = GetBytesPerPixel(newFormat);
			int nDestChannels = GetChannelCount(newFormat);

			do
			{
				float rgba[4];

				if (IsFloatFormat(m_nFormat))
				{
					if (m_nFormat <= FORMAT_RGBA16F)
					{
						for (int i = 0; i < nSrcChannels; i++)
							rgba[i] = ((half*)src)[i];
					}
					else
					{
						for (int i = 0; i < nSrcChannels; i++)
							rgba[i] = ((float*)src)[i];
					}
				}
				else if (m_nFormat >= FORMAT_I16 && m_nFormat <= FORMAT_RGBA16)
				{
					for (int i = 0; i < nSrcChannels; i++)
						rgba[i] = ((ushort*)src)[i] * (1.0f / 65535.0f);
				}
				else
				{
					for (int i = 0; i < nSrcChannels; i++)
						rgba[i] = src[i] * (1.0f / 255.0f);
				}

				if (nSrcChannels < 4)
					rgba[3] = 1.0f;

				if (nSrcChannels == 1)
					rgba[2] = rgba[1] = rgba[0];

				if (nDestChannels == 1)	rgba[0] = 0.30f * rgba[0] + 0.59f * rgba[1] + 0.11f * rgba[2];

				if (IsFloatFormat(newFormat))
				{
					if (newFormat <= FORMAT_RGBA32F)
					{
						if (newFormat <= FORMAT_RGBA16F)
						{
							for (int i = 0; i < nDestChannels; i++)
								((half*)dest)[i] = rgba[i];
						}
						else
						{
							for (int i = 0; i < nDestChannels; i++)
								((float*)dest)[i] = rgba[i];
						}
					}
					else
					{
						if (newFormat == FORMAT_RGBE8)
						{
							*(uint32*)dest = rgbToRGBE8(MColor(rgba[0], rgba[1], rgba[2]));
						}
						else
						{
							*(uint32*)dest = rgbToRGB9E5(MColor(rgba[0], rgba[1], rgba[2]));
						}
					}
				}
				else if (newFormat >= FORMAT_I16 && newFormat <= FORMAT_RGBA16)
				{
					for (int i = 0; i < nDestChannels; i++)	((ushort*)dest)[i] = (ushort)(65535 * saturate(rgba[i]) + 0.5f);
				}
				else if (/*isPackedFormat(newFormat)*/newFormat == FORMAT_RGB10A2)
				{
					*(uint*)dest =
						(uint(1023.0f * saturate(rgba[0]) + 0.5f) << 22) |
						(uint(1023.0f * saturate(rgba[1]) + 0.5f) << 12) |
						(uint(1023.0f * saturate(rgba[2]) + 0.5f) << 2) |
						(uint(3.0f * saturate(rgba[3]) + 0.5f));
				}
				else
				{
					for (int i = 0; i < nDestChannels; i++)
						dest[i] = (unsigned char)(255 * saturate(rgba[i]) + 0.5f);
				}

				src += srcSize;
				dest += destSize;
			} while (--nPixels);
		}
	}

	delete[] m_pPixels;
	m_pPixels = newPixels;
	m_nFormat = newFormat;

	return true;
}

bool CImage::SwapChannels(const int ch0, const int ch1)
{
	if (!IsPlainFormat(m_nFormat))
		return false;

	uint nPixels = GetPixelCount(0, m_nMipMaps) * m_nArraySize;
	uint nChannels = GetChannelCount(m_nFormat);

	if (m_nFormat <= FORMAT_RGBA8)
		_SwapChannels((unsigned char*)m_pPixels, nPixels, nChannels, ch0, ch1);
	else if (m_nFormat <= FORMAT_RGBA16F)
		_SwapChannels((unsigned short*)m_pPixels, nPixels, nChannels, ch0, ch1);
	else
		_SwapChannels((float*)m_pPixels, nPixels, nChannels, ch0, ch1);

	return true;
}
