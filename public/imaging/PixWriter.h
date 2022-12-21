//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Color writing class
 //////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "textureformats.h"

class PixelWriter
{
public:
					PixelWriter( ETextureFormat format, void* pMemory, int stride );

	void			Seek( int x, int y );

	void*			SkipBytes( int n );
	void			SkipPixels( int n );


	void			WritePixel(int r, int g, int b, int a = 255);
	void			WritePixel(const MColor& color);
	void			WritePixelNoAdvance(int r, int g, int b, int a = 255); 
	void			WritePixelNoAdvance(const MColor& color);

	ubyte			GetPixelSize() { return m_Size; }
private:
	int				m_nWritePos;
	ubyte*			m_pBits;
	ushort			m_BytesPerRow;
	ubyte			m_Size;
	char			m_RShift;
	char			m_GShift;
	char			m_BShift;
	char			m_AShift;
	ubyte			m_RMask;
	ubyte			m_GMask;
	ubyte			m_BMask;
	ubyte			m_AMask;
};

inline PixelWriter::PixelWriter( ETextureFormat format, void* pMemory, int stride )
{
	m_pBits = (ubyte*)pMemory;
	m_BytesPerRow = (ubyte)stride;

	m_nWritePos = 0;

	switch(format)
	{
		case FORMAT_RGBA8:
			m_Size = 4;
			m_RShift = 0;
			m_GShift = 8;
			m_BShift = 16;
			m_AShift = 24;
			m_RMask = 0xFF;
			m_GMask = 0xFF;
			m_BMask = 0xFF;
			m_AMask = 0xFF;
			break;

		case FORMAT_RGBA4:
			m_Size = 2;
			m_RShift = -4;
			m_GShift = 0;
			m_BShift = 4;
			m_AShift = 8;
			m_RMask = 0xF0;
			m_GMask = 0xF0;
			m_BMask = 0xF0;
			m_AMask = 0xF0;
			break;

		case FORMAT_RGB8:
			m_Size = 3;
			m_RShift = 0;
			m_GShift = 8;
			m_BShift = 16;
			m_AShift = 0;
			m_RMask = 0xFF;
			m_GMask = 0xFF;
			m_BMask = 0xFF;
			m_AMask = 0x00;
			break;

		case FORMAT_I8:
			m_Size = 1;
			m_RShift = 0;
			m_GShift = 0;
			m_BShift = 0;
			m_AShift = 0;
			m_RMask = 0x00;
			m_GMask = 0x00;
			m_BMask = 0x00;
			m_AMask = 0xFF;
			break;

		default:
		{
			ASSERT_FAIL( "PixelWriter::SetPixelMemory:  Unsupported image format %i\n", format );
			m_Size = 0; // set to zero so that we don't stomp memory for formats that we don't understand.
		}
		break;
	}
}

//-------------------------------------------------------
// Sets where we're writing to
//-------------------------------------------------------

inline void PixelWriter::Seek( int x, int y )
{
	m_nWritePos = m_nWritePos + y * m_BytesPerRow + x * m_Size;
}

//-------------------------------------------------------
// Skips some bytes:
//-------------------------------------------------------

inline void* PixelWriter::SkipBytes( int n )
{
	m_pBits += n;
	return m_pBits;
}

//-------------------------------------------------------
// Skips some pixels:
//-------------------------------------------------------

inline void PixelWriter::SkipPixels( int n )
{
	SkipBytes( n * m_Size );
}

//-------------------------------------------------------
// Writes a pixel, advances the write index
//-------------------------------------------------------

inline void PixelWriter::WritePixel( int r, int g, int b, int a )
{
	WritePixelNoAdvance(r,g,b,a);
	//m_pBits += m_Size;
	m_nWritePos += m_Size;
}

inline void PixelWriter::WritePixel(const MColor& color)
{
	const uint packed = color.pack();
	WritePixel(packed & 255, packed >> 8 & 255, packed >> 16 & 255, packed >> 24 & 255);
}

//-------------------------------------------------------
// Writes a pixel without advancing the index
//-------------------------------------------------------

inline void PixelWriter::WritePixelNoAdvance( int r, int g, int b, int a )
{
	int val = (r & m_RMask) << m_RShift;
	val |=  (g & m_GMask) << m_GShift;
	val |= (m_BShift > 0) ? ((b & m_BMask) << m_BShift) : ((b & m_BMask) >> -m_BShift);
	val |=	(a & m_AMask) << m_AShift;

	m_pBits[m_nWritePos] = (ubyte)((val & 0xff));
	m_pBits[m_nWritePos + 1] = (ubyte)((val >> 8) & 0xff);
	if (m_Size > 1)
	{
		m_pBits[m_nWritePos + 1] = (ubyte)((val >> 8) & 0xff);
		if (m_Size > 2)
		{
			m_pBits[m_nWritePos + 2] = (ubyte)((val >> 16) & 0xff);
			if (m_Size > 3)
				m_pBits[m_nWritePos + 3] = (ubyte)((val >> 24) & 0xff);
		}
	}
}


inline void PixelWriter::WritePixelNoAdvance(const MColor& color)
{
	const uint packed = color.pack();
	WritePixelNoAdvance(packed & 255, packed >> 8 & 255, packed >> 16 & 255, packed >> 24 & 255);
}