//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Image loader
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "textureformats.h"

class CImage
	: public RefCountedObject<CImage, RefCountDefaultPolicy, RefCountedUnsafe>
{
public:
	// Image loading flags
	enum ELoadFlags
	{
		SKIP_MIPMAPS = 0x1
	};

	static constexpr int ALL_MIPMAPS = 127;
	static constexpr int IMAGE_DEPTH_CUBEMAP = 0;

	static const char*		GetFormatString(const ETextureFormat format);
	static ETextureFormat	GetFormatFromString(const char* string);

	CImage();
	CImage(const CImage& img);
	~CImage();

	ubyte*			Create(const ETextureFormat fmt, const int w, const int h, const int d, const int mipMapCount, const int arraysize = 1);
	void			Free();
	void			Clear();

	void			SetName(const char* name) { m_szName = name; }
	char*			GetName() const { return (char*)m_szName.GetData(); }

	ubyte*			GetPixels() const { return m_pPixels; }
	ubyte*			GetPixels(const int mipMapLevel) const;
	ubyte*			GetPixels(const int mipMapLevel, const int arraySlice) const;
	int				GetMipMapCount() const { return m_nMipMaps; }
	int				GetMipMapCountFromDimesions() const;
	int				GetMipMappedSize(const int firstMipMapLevel = 0, int nMipMapLevels = ALL_MIPMAPS, ETextureFormat srcFormat = FORMAT_NONE) const;
	int				GetSliceSize(const int mipMapLevel = 0, ETextureFormat srcFormat = FORMAT_NONE) const;
	int				GetPixelCount(const int firstMipMapLevel = 0, int nMipMapLevels = ALL_MIPMAPS) const;

	int				GetWidth() const { return m_nWidth; }
	int				GetHeight() const { return m_nHeight; }
	int				GetDepth() const { return m_nDepth; }
	int				GetWidth(const int mipMapLevel) const;
	int				GetHeight(const int mipMapLevel) const;
	int				GetDepth(const int mipMapLevel) const;
	int				GetArraySize() const { return m_nArraySize; }

	EImageType		GetImageType() const;

	bool			Is1D()    const { return (m_nDepth == 1 && m_nHeight == 1); }
	bool			Is2D()    const { return (m_nDepth == 1 && m_nHeight > 1); }
	bool			Is3D()    const { return (m_nDepth > 1); }
	bool			IsCube()  const { return (m_nDepth == IMAGE_DEPTH_CUBEMAP); }
	bool			IsArray() const { return (m_nArraySize > 1); }

	ETextureFormat	GetFormat() const { return m_nFormat; }
	void			GetFormat(const ETextureFormat form) { m_nFormat = form; }

	int				GetExtraDataBytes() const { return m_nExtraDataSize; }
	ubyte*			GetExtraData() const { return m_pExtraData; }

	void			SetExtraData(void* data, const int nBytes)
	{
		m_nExtraDataSize = nBytes;
		m_pExtraData = (unsigned char*)data;
	}

	bool			LoadDDS(IFileStreamPtr fileHandle, uint flags = 0);
	bool			SaveDDS(IFileStreamPtr fileHandle) const;

	bool			Load(IFileStreamPtr fileHandle);
	bool			SaveJPEG(IFileStreamPtr fileHandle, const int quality) const;

	bool			LoadTGA(IFileStreamPtr fileHandle);
	bool			SaveTGA(IFileStreamPtr fileHandle) const;

	bool			Load(const char* fileName, uint flags = 0, int searchFlags = -1);
	bool			SaveImage(const char* fileName, int searchFlags = -1) const;

	void			LoadFromMemory(void* mem, const ETextureFormat frmt, const int wide, const int tall, const int nDepth, const int mipMapCount, bool ownsMemory);

	bool			CreateMipMaps(const int mipMaps = ALL_MIPMAPS);
	bool			RemoveMipMaps(const int firstMipMap, const int mipMapsToSave = ALL_MIPMAPS);

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
	ubyte* m_pExtraData;
};

using CImagePtr = CRefPtr<CImage>;