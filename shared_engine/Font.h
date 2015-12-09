//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//////////////////////////////////////////////////////////////////////////////////

#ifndef FONT_H
#define FONT_H

#include "IFont.h"

struct eqfontchar_t
{
	// rectangle
    float x0, y0;
    float x1, y1;

	// width ratio
    float ratio;
};

// loads font
IEqFont* InternalLoadFont(const char* pszFontName);

class CFont : public IEqFont
{
	friend class			CEngineHost;
	friend class			CEqSysConsole;

public:
							CFont();
							~CFont();

	const char*				GetName() const;

	bool					LoadFont( const char* filenamePrefix );

	float					GetStringLength(const char *string, int length, bool enableWidthRatio = true) const;
	float					GetStringLength(const wchar_t *string, int length, bool enableWidthRatio = true) const;

	// sets color of next rendered text
	void					DrawSetColor(const ColorRGBA &color);
	void					DrawSetColor(float r, float g, float b, float a);

	void					DrawText(	const char* pszText,
								int x, int y,
								int cw, int ch,
								bool enableWidthRatio = true);

	void					DrawTextEx(	const char* pszText,
								int x, int y,
								int cw, int ch,
								ETextOrientation textOrientation = TEXT_ORIENT_RIGHT,
								int styleFlags = 0,
								bool enableWidthRatio = true);

	void					RenderText(	const wchar_t* pszText,
								int x, int y,
								int cw, int ch,
								const eqFontStyleParam_t& params);

	void					DrawTextInRect(	const char* pszText,
									Rectangle_t &clipRect, int cw, int ch, bool enableWidthRatio = true, int *nLinesCount = NULL);

protected:

	// ASCII chars
	eqfontchar_t			m_FontChars[256];

	EqString				m_name;

	// extended character tables (wchar_t only)
	eqfontchar_t*			m_extChars;
	int						m_extCharsStart;
	int						m_extCharsLength;

	float					m_spacing;

	ITexture*				m_fontTexture;
	ColorRGBA				m_textColor;

	int						GetTextQuadsCount(const char *str) const;
	int						GetTextQuadsCount(const wchar_t *str) const;

	const eqfontchar_t&		GetFontCharById( const int chrId ) const;

	int						FillTextBuffer(	Vertex2D_t *dest, 
											const char *str, 
											float x, float y, 
											float charWidth, float charHeight, 
											ETextOrientation textOrientation = TEXT_ORIENT_RIGHT, 
											bool enableWidthRatio = true, 
											int styleFlags = 0, 
											float fOffset = 0.0f);

	int						FillTextBufferW(Vertex2D_t *dest, 
											const wchar_t *str, 
											float x, float y, 
											float charWidth, float charHeight, 
											const eqFontStyleParam_t& params);

	int						RectangularFillTextBuffer(	Vertex2D_t *dest, 
														const char *str, 
														float x, float y, 
														float charWidth, float charHeight, 
														Rectangle_t &rect, 
														int *nLinesCount = NULL,
														bool enableWidthRatio = true,
														float fOffset = 0.0f);

	Vertex2D_t*				m_vertexBuffer;
	int						m_numVerts;
};

#endif //IFONT_H
