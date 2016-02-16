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
#include <map>


class CFont : public IEqFont
{
	friend class			CEngineHost;
	friend class			CEqSysConsole;
	friend class			CPlainTextLayoutBuilder;

public:
							CFont();
							~CFont();

	const char*				GetName() const;

	bool					LoadFont( const char* filenamePrefix );

	// returns string width in pixels
	float					GetStringWidth( const wchar_t* str, int fontStyleFlags, int charCount = -1, int breakOnChar = -1) const;
	float					GetStringWidth( const char* str, int fontStyleFlags, int charCount = -1, int breakOnChar = -1) const;

	float					GetLineHeight() const {return m_lineHeight;}
	float					GetBaselineOffs() const {return m_baseline;}

	// renders text (wide char)
	void					RenderText(	const wchar_t* pszText,
								const Vector2D& start,
								const eqFontStyleParam_t& params);
	// renders text (ASCII)
	void					RenderText(	const char* pszText,
								const Vector2D& start,
								const eqFontStyleParam_t& params);

protected:

	const eqFontChar_t&		GetFontCharById( const int chrId ) const;

	// builds vertex buffer for characters
	template <typename CHAR_T>
	int						BuildCharVertexBuffer(	Vertex2D_t* dest, 
													const CHAR_T* str, 
													const Vector2D& startPos, 
													const eqFontStyleParam_t& params);

	template <typename CHAR_T>
	float					_GetStringWidth( const CHAR_T* str, int fontStyleFlags, int charCount = 0, int breakOnChar = -1) const;

	template <typename CHAR_T>
	int						GetTextQuadsCount(const CHAR_T *str, int fontStyleFlags) const;

	///////////////////////////////////////////////////////////////
	// OLD DEPRECATED FUNCTIONS BELOW
	/*
	int						FillTextBuffer(	Vertex2D_t *dest, 
											const char *str, 
											float x, float y, 
											float charWidth, float charHeight, 
											ETextOrientation textOrientation = TEXT_ORIENT_RIGHT, 
											bool enableWidthRatio = true, 
											int styleFlags = 0, 
											float fOffset = 0.0f);

	int						RectangularFillTextBuffer(	Vertex2D_t *dest, 
														const char *str, 
														float x, float y, 
														float charWidth, float charHeight, 
														Rectangle_t &rect, 
														int *nLinesCount = NULL,
														bool enableWidthRatio = true,
														float fOffset = 0.0f);
	*/
	//---------------------------------------------------------

	// map of chars
	std::map<int, eqFontChar_t>	m_charMap;

	float						m_spacing;
	float						m_baseline;
	float						m_lineHeight;
	Vector2D					m_scale;

	EqString					m_name;

	ITexture*					m_fontTexture;
	Vector2D					m_invTexSize;

	ColorRGBA					m_textColor;

	Vertex2D_t*					m_vertexBuffer;
	int							m_numVerts;
};

#endif //IFONT_H
