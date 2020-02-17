//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Image loader
//////////////////////////////////////////////////////////////////////////////////

#ifndef IMAGE_H
#define IMAGE_H

#include <stdio.h>
#include "platform/Platform.h"
#include "utils/eqstring.h"

#include "textureformats.h"

#include "IFileSystem.h"

#define ALL_MIPMAPS 127

#ifdef _WIN32
#undef LoadImage
#endif

// Image loading flags
#define DONT_LOAD_MIPMAPS 0x1

const char*		GetFormatString(const ETextureFormat format);
ETextureFormat	GetFormatFromString(const char *string);

class CImage
{
public:
	CImage();
	CImage(const CImage &img);
	~CImage();

	ubyte*			Create(const ETextureFormat fmt, const int w, const int h, const int d, const int mipMapCount, const int arraysize = 1);
	void			Free();
	void			Clear();

	void			SetName(const char* name) {m_szName = name;}
	char*			GetName() const {return (char*)m_szName.GetData();}

	ubyte*			GetPixels() const { return m_pPixels; }
	ubyte*			GetPixels(const int mipMapLevel) const;
	ubyte*			GetPixels(const int mipMapLevel, const int arraySlice) const;
	int				GetMipMapCount() const { return m_nMipMaps; }
	int				GetMipMapCountFromDimesions() const;
	int				GetMipMappedSize(const int firstMipMapLevel = 0, int nMipMapLevels = ALL_MIPMAPS, ETextureFormat srcFormat = FORMAT_NONE) const;
	int				GetSliceSize(const int mipMapLevel = 0, ETextureFormat srcFormat = FORMAT_NONE) const;
	int				GetPixelCount(const int firstMipMapLevel = 0, int nMipMapLevels = ALL_MIPMAPS) const;

	int				GetWidth () const { return m_nWidth;  }
	int				GetHeight() const { return m_nHeight; }
	int				GetDepth () const { return m_nDepth;  }
	int				GetWidth (const int mipMapLevel) const;
	int				GetHeight(const int mipMapLevel) const;
	int				GetDepth (const int mipMapLevel) const;
	int				GetArraySize() const { return m_nArraySize; }

	bool			Is1D()    const { return (m_nDepth == 1 && m_nHeight == 1); }
	bool			Is2D()    const { return (m_nDepth == 1 && m_nHeight >  1); }
	bool			Is3D()    const { return (m_nDepth >  1); }
	bool			IsCube()  const { return (m_nDepth == 0); }
	bool			IsArray() const { return (m_nArraySize > 1); }

	ETextureFormat	GetFormat() const { return m_nFormat; }
	void			GetFormat(const ETextureFormat form) { m_nFormat = form; }

	int				GetExtraDataBytes() const { return m_nExtraDataSize; }
	ubyte*			GetExtraData() const { return m_pExtraData; }

	void			SetExtraData(void *data, const int nBytes)
	{
		m_nExtraDataSize = nBytes;
		m_pExtraData = (unsigned char *) data;
	}

	bool			LoadDDS(const char *fileName, uint flags = 0);
#ifndef NO_JPEG
	bool			LoadJPEG(const char *fileName);
#endif // NO_JPEG
#ifndef NO_TGA
	bool			LoadTGA(const char *fileName);
#endif // NO_TGA

	bool			LoadDDSfromHandle(IFile *fileHandle, uint flags = 0);
#ifndef NO_JPEG
	bool			LoadJPEGfromHandle(IFile *fileHandle);
#endif // NO_JPEG
#ifndef NO_TGA
	bool			LoadTGAfromHandle(IFile *fileHandle);
#endif // NO_TGA

	bool			SaveDDS(const char *fileName);
#ifndef NO_JPEG
	bool			SaveJPEG(const char *fileName, const int quality);
#endif // NO_JPEG
#ifndef NO_TGA
	bool			SaveTGA(const char *fileName);
#endif // NO_TGA

	bool			LoadImage(const char *fileName, uint flags = 0);
	bool			LoadFromHandle(IFile *fileHandle,const char *fileName, uint flags = 0);

	bool			SaveImage(const char *fileName);

	void			SetDepth(int nDepth)	{m_nDepth = nDepth;}

	void			LoadFromMemory(void *mem, const ETextureFormat frmt, const int wide, const int tall, const int nDepth, const int mipMapCount, bool ownsMemory);

	bool			CreateMipMaps(const int mipMaps = ALL_MIPMAPS);
	bool			RemoveMipMaps(const int firstMipMap, const int mipMapsToSave = ALL_MIPMAPS);

	// not supported
	//bool			UncompressImage();
	//bool			UnpackImage();

	bool			SwapChannels(const int ch0, const int ch1);

	bool			Convert(const ETextureFormat newFormat);

protected:
	ubyte*			m_pPixels;
	int				m_nWidth;
	int				m_nHeight;
	int				m_nDepth;
	int				m_nMipMaps;
	int				m_nArraySize;
	ETextureFormat	m_nFormat;

	EqString		m_szName;

	int				m_nExtraDataSize;
	ubyte*			m_pExtraData;
};

#endif // IMAGE_H
