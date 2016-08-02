//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Image loader
//////////////////////////////////////////////////////////////////////////////////

#include "ImageLoader.h"
#include "DebugInterface.h"
#include "ConVar.h"

#include <string.h>
#include <stdlib.h>

#include "../math/Vector.h"

#ifndef NO_JPEG

extern "C"
{
#include <jpeglib.h>
}

#endif // NO_JPEG

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
	ETextureFormat format;
	const char *string;
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
	for (unsigned int i = 0; i < elementsOf(formatStrings); i++){
		if (format == formatStrings[i].format) return formatStrings[i].string;
	}
	return NULL;
}

ETextureFormat GetFormatFromString(const char *string)
{
	for (unsigned int i = 0; i < elementsOf(formatStrings); i++){
		if (stricmp(string, formatStrings[i].string) == 0) return formatStrings[i].format;
	}
	return FORMAT_NONE;
}


template <typename DATA_TYPE>
inline void _SwapChannels(DATA_TYPE *pixels, int nPixels, const int channels, const int ch0, const int ch1)
{
	do
	{
		DATA_TYPE tmp = pixels[ch1];
		pixels[ch1] = pixels[ch0];
		pixels[ch0] = tmp;
		pixels += channels;
	}
	while (--nPixels);
}


/* ---------------------------------------------- */

CImage::CImage()
{
	m_pPixels = NULL;
	m_nWidth  = 0;
	m_nHeight = 0;
	m_nDepth  = 0;
	m_nMipMaps = 0;
	m_nArraySize = 0;
	m_nFormat = FORMAT_NONE;

	m_nExtraDataSize = 0;
	m_pExtraData = NULL;
}

CImage::CImage(const CImage &img)
{
	m_nWidth  = img.m_nWidth;
	m_nHeight = img.m_nHeight;
	m_nDepth  = img.m_nDepth;
	m_nMipMaps = img.m_nMipMaps;
	m_nArraySize = img.m_nArraySize;
	m_nFormat = img.m_nFormat;

	int size = GetMipMappedSize(0, m_nMipMaps) * m_nArraySize;
	m_pPixels = new ubyte[size];
	memcpy(m_pPixels, img.m_pPixels, size);

	m_nExtraDataSize = img.m_nExtraDataSize;
	if(m_nExtraDataSize)
	{
		m_pExtraData = new ubyte[m_nExtraDataSize];
		memcpy(m_pExtraData, img.m_pExtraData, m_nExtraDataSize);
	}
	else
		m_pExtraData = NULL;
}

CImage::~CImage()
{
	Free();
}

ubyte* CImage::Create(const ETextureFormat fmt, const int w, const int h, const int d, const int mipMapCount, const int arraysize)
{
    m_nFormat = fmt;
    m_nWidth  = w;
	m_nHeight = h;
	m_nDepth  = d;
	m_nMipMaps = mipMapCount;
	m_nArraySize = arraysize;

	return (m_pPixels = new ubyte[GetMipMappedSize(0, m_nMipMaps) * m_nArraySize]);
}

void CImage::Free()
{
	delete [] m_pPixels;
	m_pPixels = NULL;

	if(m_pExtraData)
		delete [] m_pExtraData;

	m_pExtraData = NULL;
}

void CImage::Clear()
{
	Free();

	m_nWidth  = 0;
	m_nHeight = 0;
	m_nDepth  = 0;
	m_nMipMaps = 0;
	m_nArraySize = 0;
	m_nFormat = FORMAT_NONE;
	m_nExtraDataSize = 0;
}

ubyte* CImage::GetPixels(const int mipMapLevel) const
{
	return (mipMapLevel < m_nMipMaps)? m_pPixels + GetMipMappedSize(0, mipMapLevel) : NULL;
}

ubyte* CImage::GetPixels(const int mipMapLevel, const int arraySlice) const
{
	if (mipMapLevel >= m_nMipMaps || arraySlice >= m_nArraySize) return NULL;

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
	int w = GetWidth (firstMipMapLevel);
	int h = GetHeight(firstMipMapLevel);
	int d = GetDepth (firstMipMapLevel);

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

	return (m_nDepth == 0) ? (6 * size) : size;
}

int CImage::GetSliceSize(const int mipMapLevel, ETextureFormat srcFormat) const
{
	int w = GetWidth (mipMapLevel);
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
	int w = GetWidth (firstMipMapLevel);
	int h = GetHeight(firstMipMapLevel);
	int d = GetDepth (firstMipMapLevel);

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

	return (m_nDepth == 0) ? (6 * size) : size;
}

int CImage::GetWidth(const int mipMapLevel) const
{
	int a = m_nWidth >> mipMapLevel;
	return (a == 0)? 1 : a;
}

int CImage::GetHeight(const int mipMapLevel) const
{
	int a = m_nHeight >> mipMapLevel;
	return (a == 0)? 1 : a;
}

int CImage::GetDepth(const int mipMapLevel) const
{
	int a = m_nDepth >> mipMapLevel;
	return (a == 0)? 1 : a;
}

bool CImage::LoadDDS(const char *fileName, uint flags)
{
	DKFILE *file;
	if ((file = g_fileSystem->Open(fileName, "rb")) == NULL) return false;

	SetName(fileName);

	return LoadDDSfromHandle(file,flags);
}

#ifndef NO_JPEG
bool CImage::LoadJPEG(const char *fileName)
{
	DKFILE *file;
	if ((file = g_fileSystem->Open(fileName, "rb")) == NULL) return false;

	SetName(fileName);

	return LoadJPEGfromHandle(file);
}
#endif // NO_JPEG

#ifndef NO_TGA
bool CImage::LoadTGA(const char *fileName)
{
	DKFILE *file;
	if ((file = g_fileSystem->Open(fileName, "rb")) == NULL) return false;

	SetName(fileName);

	return LoadTGAfromHandle(file);
}
#endif

bool CImage::LoadDDSfromHandle(DKFILE *fileHandle, uint flags)
{
	DDSHeader header;

	DKFILE *file = fileHandle;

	if(!file)
		return false;

	file->Read(&header, sizeof(header), 1);

	if (header.dwMagic != MCHAR4('D','D','S',' '))
	{
		MsgError("This image is not Direct Draw Surface!\n");
		g_fileSystem->Close(file);
		return false;
	}

	m_nWidth  = header.dwWidth;
	m_nHeight = header.dwHeight;
	m_nDepth  = (header.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)? 0 : (header.dwDepth == 0)? 1 : header.dwDepth;
	m_nMipMaps = ((flags & DONT_LOAD_MIPMAPS) || (header.dwMipMapCount == 0))? 1 : header.dwMipMapCount;
	m_nArraySize = 1;

	if (header.ddpfPixelFormat.dwFourCC == MCHAR4('D','X','1','0'))
	{
		DDSHeaderDXT10 dxt10Header;
		file->Read(&dxt10Header, sizeof(dxt10Header), 1);

		switch (dxt10Header.dxgiFormat)
		{
			case 61: m_nFormat = FORMAT_R8; break;
			case 49: m_nFormat = FORMAT_RG8; break;
			case 28: m_nFormat = FORMAT_RGBA8; break;

			case 56: m_nFormat = FORMAT_R16; break;
			case 35: m_nFormat = FORMAT_RG16; break;
			case 11: m_nFormat = FORMAT_RGBA16; break;

			case 54: m_nFormat = FORMAT_R16F; break;
			case 34: m_nFormat = FORMAT_RG16F; break;
			case 10: m_nFormat = FORMAT_RGBA16F; break;

			case 41: m_nFormat = FORMAT_R32F; break;
			case 16: m_nFormat = FORMAT_RG32F; break;
			case 6:  m_nFormat = FORMAT_RGB32F; break;
			case 2:  m_nFormat = FORMAT_RGBA32F; break;

			case 67: m_nFormat = FORMAT_RGB9E5; break;
			case 26: m_nFormat = FORMAT_RG11B10F; break;
			case 24: m_nFormat = FORMAT_RGB10A2; break;

			case 71: m_nFormat = FORMAT_DXT1; break;
			case 74: m_nFormat = FORMAT_DXT3; break;
			case 77: m_nFormat = FORMAT_DXT5; break;
			case 80: m_nFormat = FORMAT_ATI1N; break;
			case 83: m_nFormat = FORMAT_ATI2N; break;
			case 0:
                m_nFormat = FORMAT_ETC2;
				MsgError("Invalid DDS file %s\n", GetName());
				//g_fileSystem->Close(file);
				//return false;
				break;
			default:
				MsgError("Image %s has unknown or invalid DXGI format %d\n", GetName(), dxt10Header.dxgiFormat);
				g_fileSystem->Close(file);
				return false;
		}
	}
	else
	{

		switch (header.ddpfPixelFormat.dwFourCC){
			case 34:  m_nFormat = FORMAT_RG16; break;
			case 36:  m_nFormat = FORMAT_RGBA16; break;
			case 111: m_nFormat = FORMAT_R16F; break;
			case 112: m_nFormat = FORMAT_RG16F; break;
			case 113: m_nFormat = FORMAT_RGBA16F; break;
			case 114: m_nFormat = FORMAT_R32F; break;
			case 115: m_nFormat = FORMAT_RG32F; break;
			case 116: m_nFormat = FORMAT_RGBA32F; break;
			case MCHAR4('D','X','T','1'): m_nFormat = FORMAT_DXT1; break;
			case MCHAR4('D','X','T','3'): m_nFormat = FORMAT_DXT3; break;
			case MCHAR4('D','X','T','5'): m_nFormat = FORMAT_DXT5; break;
			case MCHAR4('A','T','I','1'): m_nFormat = FORMAT_ATI1N; break;
			case MCHAR4('A','T','I','2'): m_nFormat = FORMAT_ATI2N; break;
			case MCHAR4('E','T','C','1'): m_nFormat = FORMAT_ETC1; break;
			case MCHAR4('E','T','C','2'): m_nFormat = FORMAT_ETC2; break;
			case MCHAR4('E','T','A','2'): m_nFormat = FORMAT_ETC2_EAC; break;
			default:
				switch (header.ddpfPixelFormat.dwRGBBitCount)
				{
					case 8: m_nFormat = FORMAT_I8; break;
					case 16:
						m_nFormat = (header.ddpfPixelFormat.dwRGBAlphaBitMask == 0xF000)? FORMAT_RGBA4 :
								 (header.ddpfPixelFormat.dwRGBAlphaBitMask == 0xFF00)? FORMAT_IA8 :
								 (header.ddpfPixelFormat.dwBBitMask == 0x1F)? FORMAT_RGB565 : FORMAT_I16;
						break;
					case 24: m_nFormat = FORMAT_RGB8; break;
					case 32:
						m_nFormat = (header.ddpfPixelFormat.dwRBitMask == 0x3FF00000)? FORMAT_RGB10A2 : FORMAT_RGBA8;
						break;
					default:
						MsgError("Image %s has unknown format.\n", GetName());
						g_fileSystem->Close(file);
						return false;
				}
		}
	}

	int size = GetMipMappedSize(0, m_nMipMaps);
	m_pPixels = new ubyte[size];
	if (IsCube())
	{
		for (int face = 0; face < 6; face++)
		{
			for (int mipMapLevel = 0; mipMapLevel < m_nMipMaps; mipMapLevel++)
			{
				int faceSize = GetMipMappedSize(mipMapLevel, 1) / 6;
				ubyte *src = GetPixels(mipMapLevel) + face * faceSize;

				file->Read(src, 1, faceSize);
			}
			if ((flags & DONT_LOAD_MIPMAPS) && header.dwMipMapCount > 1)
			{
				file->Seek(GetMipMappedSize(1, header.dwMipMapCount - 1) / 6, VS_SEEK_CUR);
			}
		}
	}
	else
		file->Read(m_pPixels, 1, size);

	if ((m_nFormat == FORMAT_RGB8 || m_nFormat == FORMAT_RGBA8) && header.ddpfPixelFormat.dwBBitMask == 0xFF)
	{
		int nChannels = GetChannelCount(m_nFormat);
		_SwapChannels(m_pPixels, size / nChannels, nChannels, 0, 2);
	}

	g_fileSystem->Close(file);
	return true;
}

#ifndef NO_JPEG
bool CImage::LoadJPEGfromHandle(DKFILE *fileHandle)
{
	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	DKFILE *file = fileHandle;

	// HACK: the DKFILE has FILE pointer at beginning. JPEGs can't be open from packages.
	FILE* pFile = *(FILE**)(void*)fileHandle;

	jpeg_stdio_src(&cinfo, pFile);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	switch (cinfo.num_components)
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

	m_nWidth  = cinfo.output_width;
	m_nHeight = cinfo.output_height;
	m_nDepth  = 1;
	m_nMipMaps = 1;
	m_nArraySize = 1;

	m_pPixels = new ubyte[m_nWidth * m_nHeight * cinfo.num_components];

	for( ubyte *curr_scanline = m_pPixels ; cinfo.output_scanline < cinfo.output_height; curr_scanline += m_nWidth * cinfo.num_components)
		jpeg_read_scanlines(&cinfo, &curr_scanline, 1);

	/*
	ubyte *curr_scanline = m_pPixels;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, &curr_scanline, 1);
		curr_scanline += width * cinfo.num_components;
	}
	*/

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	g_fileSystem->Close(file);

	return true;
}
#endif // NO_JPEG

#ifndef NO_TGA
bool CImage::LoadTGAfromHandle(DKFILE *fileHandle)
{
	TGAHeader header;

	int size, x, y, pixelSize, palLength;
	ubyte *tempBuffer, *fBuffer, *dest, *src;
	uint tempPixel;
	ubyte palette[768];
	DKFILE *file = fileHandle;


	// Find file size
	size = file->GetSize();

	// Read the header
	file->Read(&header, sizeof(header), 1);

	m_nWidth  = header.width;
	m_nHeight = header.height;
	m_nDepth  = 1;
	m_nMipMaps = 1;
	m_nArraySize = 1;

	pixelSize = header.bpp / 8;

	if ((palLength = header.descriptionlen + header.cmapentries * header.cmapbits / 8) > 0)
		file->Read(palette, sizeof(palette), 1);

	// Read the file data
	fBuffer = new ubyte[size - sizeof(header) - palLength];
	file->Read(fBuffer, size - sizeof(header) - palLength, 1);
	g_fileSystem->Close(file);

	size = m_nWidth * m_nHeight * pixelSize;

	tempBuffer = new ubyte[size];


	// Decode if rle compressed. Bit 3 of .imagetype tells if the file is compressed
	if (header.imagetype & 0x08)
	{
		uint c,count;

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
					memcpy(dest,src,pixelSize);
					dest += pixelSize;
				}
				while (--count);

				src += pixelSize;
			}
			else
			{
				// Raw packet
				count *= pixelSize;
				memcpy(dest,src,count);
				src += count;
				dest += count;
			}
		}

		src = tempBuffer;
	}
	else
		src = fBuffer;

	src += (header.bpp / 8) * m_nWidth * (m_nHeight - 1);

	switch(header.bpp)
	{
	case 8:
		if (palLength > 0)
		{
			m_nFormat = FORMAT_RGB8;
			dest = m_pPixels = new ubyte[m_nWidth * m_nHeight * 3];
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
			dest = m_pPixels = new ubyte[m_nWidth * m_nHeight];
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
		dest = m_pPixels = new ubyte[m_nWidth * m_nHeight * 4];
		for (y = 0; y < m_nHeight; y++)
		{
			for (x = 0; x < m_nWidth; x++)
			{
				tempPixel = *((unsigned short *) src);

				dest[0] = ((tempPixel >> 10) & 0x1F) << 3;
				dest[1] = ((tempPixel >>  5) & 0x1F) << 3;
				dest[2] = ((tempPixel      ) & 0x1F) << 3;
				dest[3] = ((tempPixel >> 15) ? 0xFF : 0);
				dest += 4;
				src += 2;
			}
			src -= 4 * m_nWidth;
		}
		break;
	case 24:
		m_nFormat = FORMAT_RGB8;
		dest = m_pPixels = new ubyte[m_nWidth * m_nHeight * 3];

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
		dest = m_pPixels = new ubyte[m_nWidth * m_nHeight * 4];
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

	delete [] tempBuffer;
	delete [] fBuffer;

	return true;
}
#endif

bool CImage::LoadImage(const char *fileName, uint flags)
{
	const char *extension = strrchr(fileName, '.');

	Clear();

	if (extension == NULL) return false;

	if (stricmp(extension, ".dds") == 0)
	{
		if (!LoadDDS(fileName, flags)) return false;
	}
#ifndef NO_JPEG
	else if (stricmp(extension, ".jpg") == 0 || stricmp(extension, ".jpeg") == 0)
	{
		if (!LoadJPEG(fileName)) return false;
	}
#endif // NO_JPEG
#ifndef NO_TGA
	else if (stricmp(extension, ".tga") == 0)
	{
		if (!LoadTGA(fileName)) return false;
	}
#endif // NO_TGA
	else
	{
		return false;
	}
	return true;
}

bool CImage::LoadFromHandle(DKFILE *fileHandle,const char *fileName, uint flags)
{
	const char *extension = strrchr(fileName, '.');

	Clear();

	if (extension == NULL) return false;

	if (stricmp(extension, ".dds") == 0)
	{
		if (!LoadDDSfromHandle(fileHandle, flags)) return false;
	}
#ifndef NO_JPEG
	else if (stricmp(extension, ".jpg") == 0 || stricmp(extension, ".jpeg") == 0)
	{
		if (!LoadJPEGfromHandle(fileHandle)) return false;
	}
#endif // NO_JPEG
#ifndef NO_TGA
	else if (stricmp(extension, ".tga") == 0)
	{
		if (!LoadTGAfromHandle(fileHandle)) return false;
	}
#endif // NO_TGA
	else
		return false;

	return true;
}

bool CImage::SaveDDS(const char *fileName)
{
	// Set up the header
	DDSHeader header;
	memset(&header, 0, sizeof(header));
	DDSHeaderDXT10 headerDXT10;
	memset(&headerDXT10, 0, sizeof(headerDXT10));

	header.dwMagic = MCHAR4('D', 'D', 'S', ' ');
	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | (m_nMipMaps > 1? DDSD_MIPMAPCOUNT : 0) | (m_nDepth > 1? DDSD_DEPTH : 0);
	header.dwHeight = m_nHeight;
	header.dwWidth  = m_nWidth;
	header.dwPitchOrLinearSize = 0;
	header.dwDepth = (m_nDepth > 1)? m_nDepth : 0;
	header.dwMipMapCount = (m_nMipMaps > 1)? m_nMipMaps : 0;

	int nChannels = GetChannelCount(m_nFormat);

	header.ddpfPixelFormat.dwSize = 32;
	if (m_nFormat <= FORMAT_I16 || m_nFormat == FORMAT_RGB10A2)
	{
		header.ddpfPixelFormat.dwFlags = ((nChannels < 3)? 0x00020000 : DDPF_RGB) | ((nChannels & 1)? 0 : DDPF_ALPHAPIXELS);
		if (m_nFormat <= FORMAT_RGBA8)
		{
			header.ddpfPixelFormat.dwRGBBitCount = 8 * nChannels;
			header.ddpfPixelFormat.dwRBitMask = (nChannels > 2)? 0x00FF0000 : 0xFF;
			header.ddpfPixelFormat.dwGBitMask = (nChannels > 1)? 0x0000FF00 : 0;
			header.ddpfPixelFormat.dwBBitMask = (nChannels > 1)? 0x000000FF : 0;
			header.ddpfPixelFormat.dwRGBAlphaBitMask = (nChannels == 4)? 0xFF000000 : (nChannels == 2)? 0xFF00 : 0;
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
			case FORMAT_DXT1:    header.ddpfPixelFormat.dwFourCC = MCHAR4('D','X','T','1'); break;
			case FORMAT_DXT3:    header.ddpfPixelFormat.dwFourCC = MCHAR4('D','X','T','3'); break;
			case FORMAT_DXT5:    header.ddpfPixelFormat.dwFourCC = MCHAR4('D','X','T','5'); break;
			case FORMAT_ATI1N:   header.ddpfPixelFormat.dwFourCC = MCHAR4('A','T','I','1'); break;
			case FORMAT_ATI2N:   header.ddpfPixelFormat.dwFourCC = MCHAR4('A','T','I','2'); break;
			default:
				header.ddpfPixelFormat.dwFourCC = MCHAR4('D','X','1','0');
				headerDXT10.arraySize = 1;
				headerDXT10.miscFlag = (m_nDepth == 0)? D3D10_RESOURCE_MISC_TEXTURECUBE : 0;
				headerDXT10.resourceDimension = Is1D()? D3D10_RESOURCE_DIMENSION_TEXTURE1D : Is3D()? D3D10_RESOURCE_DIMENSION_TEXTURE3D : D3D10_RESOURCE_DIMENSION_TEXTURE2D;
				switch (m_nFormat)
				{
					//case FORMAT_RGBA8:    headerDXT10.dxgiFormat = 28; break;
					case FORMAT_RGB32F:   headerDXT10.dxgiFormat = 6; break;
					case FORMAT_RGB9E5:   headerDXT10.dxgiFormat = 67; break;
					case FORMAT_RG11B10F: headerDXT10.dxgiFormat = 26; break;
					default:
						return false;
				}
		}
	}

	header.ddsCaps.dwCaps1 = DDSCAPS_TEXTURE | (m_nMipMaps > 1? DDSCAPS_MIPMAP | DDSCAPS_COMPLEX : 0) | (m_nDepth != 1? DDSCAPS_COMPLEX : 0);
	header.ddsCaps.dwCaps2 = (m_nDepth > 1)? DDSCAPS2_VOLUME : (m_nDepth == 0)? DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES : 0;
	header.ddsCaps.Reserved[0] = 0;
	header.ddsCaps.Reserved[1] = 0;
	header.dwReserved2 = 0;

	FILE *file;
	if ((file = fopen(fileName, "wb")) == NULL) return false;

	fwrite(&header, sizeof(header), 1, file);
	if (headerDXT10.dxgiFormat) fwrite(&headerDXT10, sizeof(headerDXT10), 1, file);


	int size = GetMipMappedSize(0, m_nMipMaps);

	// RGB to BGR
	if (m_nFormat == FORMAT_RGB8 || m_nFormat == FORMAT_RGBA8)
		_SwapChannels(m_pPixels, size / nChannels, nChannels, 0, 2);

	if (IsCube())
	{
		for (int face = 0; face < 6; face++){
			for (int mipMapLevel = 0; mipMapLevel < m_nMipMaps; mipMapLevel++)
			{
				int faceSize = GetMipMappedSize(mipMapLevel, 1) / 6;
                ubyte *src = GetPixels(mipMapLevel) + face * faceSize;
				fwrite(src, 1, faceSize, file);
			}
		}
	}
	else
		fwrite(m_pPixels, size, 1, file);

	fclose(file);

	// Restore to RGB
	if (m_nFormat == FORMAT_RGB8 || m_nFormat == FORMAT_RGBA8)
		_SwapChannels(m_pPixels, size / nChannels, nChannels, 0, 2);

	return true;
}

#ifndef NO_JPEG
bool CImage::SaveJPEG(const char *fileName, const int quality)
{
	if (m_nFormat != FORMAT_I8 && m_nFormat != FORMAT_RGB8)
		return false;

	jpeg_compress_struct cinfo;
	jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	FILE *file;
	if ((file = fopen(fileName, "wb")) == NULL) return false;

	int nChannels = GetChannelCount(m_nFormat);

	cinfo.in_color_space = (nChannels == 1)? JCS_GRAYSCALE : JCS_RGB;
	jpeg_set_defaults(&cinfo);

	cinfo.input_components = nChannels;
	cinfo.num_components   = nChannels;
	cinfo.image_width  = m_nWidth;
	cinfo.image_height = m_nHeight;
	cinfo.data_precision = 8;
	cinfo.input_gamma = 1.0;

	jpeg_set_quality(&cinfo, quality, FALSE);

	jpeg_stdio_dest(&cinfo, file);
	jpeg_start_compress(&cinfo, TRUE);

	ubyte *curr_scanline = m_pPixels;

	for (int y = 0; y < m_nHeight; y++)
	{
		jpeg_write_scanlines(&cinfo, &curr_scanline, 1);
		curr_scanline += nChannels * m_nWidth;
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	fclose(file);

	return true;
}
#endif // NO_JPEG

#ifndef NO_TGA
bool CImage::SaveTGA(const char *fileName)
{
	if (m_nFormat != FORMAT_I8 && m_nFormat != FORMAT_RGB8 && m_nFormat != FORMAT_RGBA8)
		return false;

	FILE *file;
	if ((file = fopen(fileName, "wb")) == NULL) return false;

	int nChannels = GetChannelCount(m_nFormat);

	TGAHeader header = {
		0x00,
		(uint8)((m_nFormat == FORMAT_I8)? 1 : 0),
		(uint8)((m_nFormat == FORMAT_I8)? 1 : 2),
		0x0000,
		(uint16)((m_nFormat == FORMAT_I8)? 256 : 0),
		(uint8)((m_nFormat == FORMAT_I8)? 24  : 0),
		0x0000,
		0x0000,
		(uint16)m_nWidth,
		(uint16)m_nHeight,
		(uint8)(nChannels * 8),
		0x00
	};

	fwrite(&header, sizeof(header), 1, file);

	ubyte *dest, *src, *buffer;

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
		fwrite(pal, sizeof(pal), 1, file);

		src = m_pPixels + m_nWidth * m_nHeight;
		for (int y = 0; y < m_nHeight; y++)
		{
			src -= m_nWidth;
			fwrite(src, m_nWidth, 1, file);
		}

	}
	else
	{
		bool useAlpha = (nChannels == 4);
		int lineLength = m_nWidth * (useAlpha? 4 : 3);

		buffer = new ubyte[m_nHeight * lineLength];
		int len;

		for (int y = 0; y < m_nHeight; y++){
			dest = buffer + (m_nHeight - y - 1) * lineLength;
			src  = m_pPixels + y * m_nWidth * nChannels;
			len = m_nWidth;

			do
			{
				*dest++ = src[2];
				*dest++ = src[1];
				*dest++ = src[0];
				if (useAlpha) *dest++ = src[3];
				src += nChannels;
			}
			while (--len);
		}

		fwrite(buffer, m_nHeight * lineLength, 1, file);
		delete [] buffer;
	}

	fclose(file);

	return true;
}
#endif // NO_TGA

bool CImage::SaveImage(const char *fileName)
{
	const char *extension = strrchr(fileName, '.');

	if (extension != NULL){
		if (stricmp(extension, ".dds") == 0){
			return SaveDDS(fileName);
		}
#ifndef NO_JPEG
		else if (stricmp(extension, ".jpg") == 0 ||
                   stricmp(extension, ".jpeg") == 0){
			return SaveJPEG(fileName, 75);
		}
#endif // NO_JPEG
#ifndef NO_TGA
		else if (stricmp(extension, ".tga") == 0){
			return SaveTGA(fileName);
		}
#endif // NO_TGA
	}
	return false;
}

void CImage::LoadFromMemory(void *mem, const ETextureFormat frmt, const int w, const int h, const int d, const int mipMapCount, bool ownsMemory)
{
	Free();

	m_nWidth  = w;
	m_nHeight = h;
	m_nDepth  = d;
    m_nFormat = frmt;
	m_nMipMaps = mipMapCount;
	m_nArraySize = 1;

	if (ownsMemory)
		m_pPixels = (unsigned char *) mem;
	else
	{
		int size = GetMipMappedSize(0, m_nMipMaps);
		m_pPixels = new unsigned char[size];
		memcpy(m_pPixels, mem, size);
	}
}


template <typename DATA_TYPE>
void BuildMipMap(DATA_TYPE *dst, const DATA_TYPE *src, const uint w, const uint h, const uint d, const uint c)
{
	uint xOff = (w < 2)? 0 : c;
	uint yOff = (h < 2)? 0 : c * w;
	uint zOff = (d < 2)? 0 : c * w * h;

	for (uint z = 0; z < d; z += 2){
		for (uint y = 0; y < h; y += 2){
			for (uint x = 0; x < w; x += 2){
				for (uint i = 0; i < c; i++){
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

	if (!IsPowerOf2(m_nWidth) || !IsPowerOf2(m_nHeight) || !IsPowerOf2(m_nDepth))
		return false;

	int actualMipMaps = min(mipMaps, GetMipMapCountFromDimesions());

	if (m_nMipMaps != actualMipMaps)
	{
		int size = GetMipMappedSize(0, actualMipMaps);
		if (m_nArraySize > 1)
		{
			ubyte *newPixels = new ubyte[size * m_nArraySize];

			// Copy top mipmap of all array slices to new location
			int firstMipSize = GetMipMappedSize(0, 1);
			int oldSize = GetMipMappedSize(0, m_nMipMaps);

			for (int i = 0; i < m_nArraySize; i++)
			{
				memcpy(newPixels + i * size, m_pPixels + i * oldSize, firstMipSize);
			}

			delete [] m_pPixels;
			m_pPixels = newPixels;
		}
		else
		{
			/*
			unsigned char* newpels = new unsigned char[size/sizeof(ubyte)];
			memcpy(newpels,pixels,sizeof(newpels));
			delete pixels;
			*/

			m_pPixels = (ubyte *) realloc(m_pPixels, size);
		}

		m_nMipMaps = actualMipMaps;
	}

	int nChannels = GetChannelCount(m_nFormat);


	int n = IsCube()? 6 : 1;

	for (int arraySlice = 0; arraySlice < m_nArraySize; arraySlice++)
	{
		ubyte *src = GetPixels(0, arraySlice);
		ubyte *dst = GetPixels(1, arraySlice);

		for (int level = 1; level < m_nMipMaps; level++)
		{
			int w = GetWidth (level - 1);
			int h = GetHeight(level - 1);
			int d = GetDepth (level - 1);

			int srcSize = GetMipMappedSize(level - 1, 1) / n;
			int dstSize = GetMipMappedSize(level,     1) / n;

			for (int i = 0; i < n; i++)
			{
				if (IsPlainFormat(m_nFormat))
				{
					if (IsFloatFormat(m_nFormat))
					{
						BuildMipMap((float *) dst, (float *) src, w, h, d, nChannels);
					}
					else if (m_nFormat >= FORMAT_I16)
					{
						BuildMipMap((ushort *) dst, (ushort *) src, w, h, d, nChannels);
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

	int mipMapCount = min(firstMipMap + mipMapsToSave, m_nMipMaps) - firstMipMap;

	int size = GetMipMappedSize(firstMipMap, mipMapCount);
	ubyte *newPixels = new ubyte[size];

	memcpy(newPixels, GetPixels(firstMipMap), size);

	delete [] m_pPixels;
	m_pPixels = newPixels;
	m_nWidth = GetWidth(firstMipMap);
	m_nHeight = GetHeight(firstMipMap);
	m_nDepth = m_nDepth? GetDepth(firstMipMap) : 0;
	m_nMipMaps = mipMapCount;

	return true;
}

bool CImage::Convert(const ETextureFormat newFormat)
{
	ubyte *newPixels;
	uint nPixels = GetPixelCount(0, m_nMipMaps) * m_nArraySize;

	if (m_nFormat == FORMAT_RGBE8 && (newFormat == FORMAT_RGB32F || newFormat == FORMAT_RGBA32F))
	{
		newPixels = new ubyte[GetMipMappedSize(0, m_nMipMaps, newFormat) * m_nArraySize];
		float *dest = (float *) newPixels;

		bool writeAlpha = (newFormat == FORMAT_RGBA32F);
		ubyte *src = m_pPixels;
		do
		{
			*((Vector3D *) dest) = rgbeToRGB(src);
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
		}
		while (--nPixels);

	}
	else
	{
		if (!IsPlainFormat(m_nFormat) || !(IsPlainFormat(newFormat) || newFormat == FORMAT_RGB10A2 || newFormat == FORMAT_RGBE8 || newFormat == FORMAT_RGB9E5))
			return false;

		if (m_nFormat == newFormat)
			return true;

		ubyte *src = m_pPixels;
		ubyte *dest = newPixels = new ubyte[GetMipMappedSize(0, m_nMipMaps, newFormat) * m_nArraySize];

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
			}
			while (--nPixels);

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
							rgba[i] = ((half *) src)[i];
					}
					else
					{
						for (int i = 0; i < nSrcChannels; i++)
							rgba[i] = ((float *) src)[i];
					}
				}
				else if (m_nFormat >= FORMAT_I16 && m_nFormat <= FORMAT_RGBA16)
				{
					for (int i = 0; i < nSrcChannels; i++)
						rgba[i] = ((ushort *) src)[i] * (1.0f / 65535.0f);
				}
				else
				{
					for (int i = 0; i < nSrcChannels; i++)
						rgba[i] = src[i] * (1.0f / 255.0f);
				}

				if (nSrcChannels  < 4)
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
								((half *) dest)[i] = rgba[i];
						}
						else
						{
							for (int i = 0; i < nDestChannels; i++)
								((float *) dest)[i] = rgba[i];
						}
					}
					else
					{
						if (newFormat == FORMAT_RGBE8)
						{
							*(uint32 *) dest = rgbToRGBE8(Vector3D(rgba[0], rgba[1], rgba[2]));
						}
						else
						{
							*(uint32 *) dest = rgbToRGB9E5(Vector3D(rgba[0], rgba[1], rgba[2]));
						}
					}
				}
				else if (newFormat >= FORMAT_I16 && newFormat <= FORMAT_RGBA16)
				{
					for (int i = 0; i < nDestChannels; i++)	((ushort *) dest)[i] = (ushort) (65535 * saturate(rgba[i]) + 0.5f);
				}
				else if (/*isPackedFormat(newFormat)*/newFormat == FORMAT_RGB10A2)
				{
					*(uint *) dest =
						(uint(1023.0f * saturate(rgba[0]) + 0.5f) << 22) |
						(uint(1023.0f * saturate(rgba[1]) + 0.5f) << 12) |
						(uint(1023.0f * saturate(rgba[2]) + 0.5f) <<  2) |
						(uint(   3.0f * saturate(rgba[3]) + 0.5f));
				}
				else
				{
					for (int i = 0; i < nDestChannels; i++)
						dest[i] = (unsigned char) (255 * saturate(rgba[i]) + 0.5f);
				}

				src  += srcSize;
				dest += destSize;
			}
			while (--nPixels);
		}
	}

	delete [] m_pPixels;
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
		_SwapChannels((unsigned char *) m_pPixels, nPixels, nChannels, ch0, ch1);
	else if (m_nFormat <= FORMAT_RGBA16F)
		_SwapChannels((unsigned short *) m_pPixels, nPixels, nChannels, ch0, ch1);
	else
		_SwapChannels((float *) m_pPixels, nPixels, nChannels, ch0, ch1);

	return true;
}
