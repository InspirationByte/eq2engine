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

// undefine windows macro for Visual Studio
#if defined(_WIN32)
#undef DrawText
#undef DrawTextEx
#endif

enum ETextOrientation
{
	TEXT_ORIENT_RIGHT = 0,
	TEXT_ORIENT_UP,
	TEXT_ORIENT_DOWN,
};

enum ETextStyleFlag
{
	TEXT_STYLE_SHADOW		= (1 << 0), // render a shadow
	TEXT_STYLE_MONOSPACE	= (1 << 1), // act as monospace, don't use advX
	TEXT_STYLE_FROM_CAP		= (1 << 2),	// offset from cap height instead of baseline
	TEXT_STYLE_USE_TAGS		= (1 << 3),	// use additional styling tags defined in the translated text

	//
	// font cache only flags
	//
	TEXT_STYLE_REGULAR		= 0,
	TEXT_STYLE_BOLD			= (1 << 8),
	TEXT_STYLE_ITALIC		= (1 << 9),
};

enum ETextAlignment
{
	TEXT_ALIGN_LEFT = 0,
	TEXT_ALIGN_RIGHT,
	TEXT_ALIGN_CENTER,
};

// text
struct eqFontStyleParam_t
{
	eqFontStyleParam_t()
	{
		align = TEXT_ALIGN_LEFT;
		orient = TEXT_ORIENT_RIGHT;
		styleFlag = 0;

		textColor = ColorRGBA(1.0f);

		shadowOffset = 1.0f;
		shadowColor = ColorRGB(0.0f);
		shadowAlpha = 0.7f;
	}

	int			orient;			// ETextOrientation
	int			align;			// ETextAlignment
	int			styleFlag;		// ETextStyleFlag
	float		shadowOffset;

	ColorRGB	shadowColor;
	float		shadowAlpha;

	ColorRGBA	textColor;
};

class IEqFont
{
public:
	virtual ~IEqFont() {}

	// returns string width in pixels
	virtual float				GetStringWidth( const wchar_t* str, int styleFlags, int charCount = -1, int breakOnChar = -1) const = 0;

	// returns string width in pixels
	virtual float				GetStringWidth( const char* str, int styleFlags, int charCount = -1, int breakOnChar = -1) const = 0;

	// returns font line height in pixels
	virtual float				GetLineHeight() const = 0;

	// returns font baseline offset in pixels
	virtual float				GetBaselineOffs() const = 0;

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
