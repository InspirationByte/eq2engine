//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//
//		Text tagging supported by RenderText command:
//			&<tag[properties]>; <tagget text> &;
//
//		Using &; closes tag
//
//		Example:
//			"&#FF4040;Colored text&;non-colored text"
//
//		for now only supported text color.
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef IFONT_H
#define IFONT_H

#include "math/Rectangle.h"
#include "materialsystem/IMaterialSystem.h"

// text styles
enum ETextStyleFlag
{
	TEXT_STYLE_SHADOW		= (1 << 0), // render a shadow
	TEXT_STYLE_MONOSPACE	= (1 << 1), // act as monospace, don't use advX
	TEXT_STYLE_FROM_CAP		= (1 << 2),	// offset from cap height instead of baseline
	TEXT_STYLE_USE_TAGS		= (1 << 3),	// use additional styling tags defined in the translated text
	TEXT_STYLE_SCISSOR		= (1 << 4), // render using scissor

	//
	// font cache only flags
	//
	TEXT_STYLE_REGULAR		= 0,
	TEXT_STYLE_BOLD			= (1 << 8),
	TEXT_STYLE_ITALIC		= (1 << 9),
};

// alignment types
enum ETextAlignment
{
	TEXT_ALIGN_LEFT		= 0, // default flag
	TEXT_ALIGN_RIGHT	= (1 << 0), // has no right?
	TEXT_ALIGN_HCENTER	= (1 << 1),

	TEXT_ALIGN_TOP		= 0, // by default
	TEXT_ALIGN_BOTTOM	= (1 << 3),
	TEXT_ALIGN_VCENTER	= (1 << 4),
};

struct eqFontChar_t
{
	eqFontChar_t()
	{
		x0 = y0 = x1 = y1 = ofsX = ofsY = advX = 0.0f;
	}

	// rectangle
    half x0, y0;
    half x1, y1;

	half ofsX,ofsY;
	half advX;
};

class IEqFont;
struct eqFontStyleParam_t;

//-------------------------------------------------------------------------
// Font layout interface
//-------------------------------------------------------------------------
class ITextLayoutBuilder
{
public:
	virtual ~ITextLayoutBuilder() {}

	virtual void	Reset( IEqFont* font ) { m_font = font; }

	// controls the newline. For different text orientations
	virtual void	OnNewLine(	const eqFontStyleParam_t& params,
								void* strCurPos, bool isWideChar,
								int lineNumber,
								const Vector2D& textStart,
								Vector2D& curTextPos ) = 0;

	// for special layouts like rectangles
	// if false then stops output, and don't render this char
	virtual bool	LayoutChar( const eqFontStyleParam_t& params,
								void* strCurPos, bool isWideChar,
								const eqFontChar_t& chr,
								Vector2D& curTextPos,
								Vector2D& cPos, Vector2D& cSize ) = 0;

protected:
	IEqFont*		m_font;
};

// text
struct eqFontStyleParam_t
{
	eqFontStyleParam_t()
	{
		align = 0; // which is (TEXT_ALIGN_LEFT | TEXT_ALIGN_TOP)
		styleFlag = 0;

		shadowOffset = 1.0f;
		shadowWidth = 0.01f;

		layoutBuilder = NULL;

		shadowColor = ColorRGB(0.0f);
		shadowAlpha = 0.7f;

		textColor = ColorRGBA(1.0f);

		scale		= 1.0f;
	}

	eqFontStyleParam_t(const eqFontStyleParam_t& derived)
	{
		align = derived.align;
		styleFlag = derived.styleFlag;

		shadowOffset = derived.shadowOffset;
		shadowWidth = derived.shadowWidth;

		layoutBuilder = derived.layoutBuilder;

		shadowColor = derived.shadowColor;
		shadowAlpha = derived.shadowAlpha;

		textColor = derived.textColor;

		scale = derived.scale;
	}

	int					align;			// ETextAlignment
	int					styleFlag;		// ETextStyleFlag

	float				shadowOffset;
	float				shadowWidth;

	ITextLayoutBuilder*	layoutBuilder;

	ColorRGB			shadowColor;
	float				shadowAlpha;

	ColorRGBA			textColor;

	// SDF font size scaling
	Vector2D			scale;
};

//-------------------------------------------------------------------------
// The font interface
//-------------------------------------------------------------------------
class IEqFont
{
public:
	virtual ~IEqFont() {}

	// returns string width in pixels
	virtual float				GetStringWidth( const wchar_t* str, const eqFontStyleParam_t& params, int charCount = -1, int breakOnChar = -1) const = 0;

	// returns string width in pixels
	virtual float				GetStringWidth( const char* str, const eqFontStyleParam_t& params, int charCount = -1, int breakOnChar = -1) const = 0;

	// returns font line height in pixels
	virtual float				GetLineHeight(const eqFontStyleParam_t& params) const = 0;

	// returns font baseline offset in pixels
	virtual float				GetBaselineOffs(const eqFontStyleParam_t& params) const = 0;

	// returns the character data
	virtual const eqFontChar_t&	GetFontCharById( const int chrId ) const = 0;

	// returns the scaled character
	virtual void				GetScaledCharacter( eqFontChar_t& chr, const int chrId, const Vector2D& scale = 1.0f ) const = 0;

	// renders text
	virtual void				RenderText(	const wchar_t* pszText,
											const Vector2D& start,
											const eqFontStyleParam_t& params) = 0;

	// renders text
	virtual void				RenderText(	const char* pszText,
											const Vector2D& start,
											const eqFontStyleParam_t& params) = 0;
};

#endif //IFONT_H
