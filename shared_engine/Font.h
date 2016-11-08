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

class CMeshBuilder;

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
	float					GetStringWidth( const wchar_t* str, const eqFontStyleParam_t& params, int charCount = -1, int breakOnChar = -1) const;
	float					GetStringWidth( const char* str, const eqFontStyleParam_t& params, int charCount = -1, int breakOnChar = -1) const;

	float					GetLineHeight( const eqFontStyleParam_t& params ) const;
	float					GetBaselineOffs( const eqFontStyleParam_t& params ) const;

	// renders text (wide char)
	void					RenderText(	const wchar_t* pszText,
								const Vector2D& start,
								const eqFontStyleParam_t& params);
	// renders text (ASCII)
	void					RenderText(	const char* pszText,
								const Vector2D& start,
								const eqFontStyleParam_t& params);

protected:

	// returns the character data
	const eqFontChar_t&		GetFontCharById( const int chrId ) const;

	// returns the scaled character
	void					GetScaledCharacter( eqFontChar_t& chr, const int chrId, const Vector2D& scale = 1.0f ) const;

	// builds vertex buffer for characters
	template <typename CHAR_T>
	void					BuildCharVertexBuffer(	CMeshBuilder& builder, 
													const CHAR_T* str, 
													const Vector2D& startPos, 
													const eqFontStyleParam_t& params);

	template <typename CHAR_T>
	float					_GetStringWidth( const CHAR_T* str, const eqFontStyleParam_t& params, int charCount = 0, int breakOnChar = -1) const;

	template <typename CHAR_T>
	int						GetTextQuadsCount(const CHAR_T *str, const eqFontStyleParam_t& params) const;

	// map of chars
	std::map<ushort, eqFontChar_t>	m_charMap;

	float							m_spacing;
	float							m_baseline;
	float							m_lineHeight;

	Vector2D						m_scale;

	EqString						m_name;

	ITexture*						m_fontTexture;
	Vector2D						m_invTexSize;

	ColorRGBA						m_textColor;

	bool							m_isSDF;
};

#endif //IFONT_H
