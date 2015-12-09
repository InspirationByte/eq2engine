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
	TEXT_STYLE_SHADOW		= (1 << 0),
	TEXT_STYLE_WIDTHRATIO	= (1 << 1),

	TEXT_STYLE_CENTER		= (1 << 2),	// legacy flag

	// font cache only flags
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
		styleFlag = TEXT_STYLE_WIDTHRATIO;

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

	// sets color of next rendered text
	virtual void		DrawSetColor(const ColorRGBA &color) = 0;
	virtual void		DrawSetColor(float r, float g, float b, float a) = 0;

	// Draw text
	virtual void		DrawText(	const char* pszText,
									int x, int y,
									int lw, int lh,
									bool enableWidthRatio = true) = 0;

	// Draws text with orientatoin
	virtual void		DrawTextEx(	const char* pszText,
									int x, int y,
									int lw, int lh,
									ETextOrientation textOrientation = TEXT_ORIENT_RIGHT,
									int styleFlags = 0,
									bool enableWidthRatio = true) = 0;

	// Renders text
	virtual void		RenderText(	const wchar_t* pszText,
									int x, int y,
									int lw, int lh,
									const eqFontStyleParam_t& params) = 0;

	// Draw text in rectange:
	//Rectangle_t rect(512,512,512+100,512+25);
	//DrawTextInRect("This is a long text that cannot be in rectangle",rect,17,17);
	virtual void		DrawTextInRect(	const char* pszText,
										Rectangle_t &clipRect,
										int lw, int lh,
										bool enableWidthRatio = true,
										int *nLinesCount = NULL) = 0;

	// Returns the string length by it's width ratio
	virtual float		GetStringLength(const char *string, int length, bool enableWidthRatio = true) const = 0;

	// Returns the string length by it's width ratio
	virtual float		GetStringLength(const wchar_t *string, int length, bool enableWidthRatio = true) const = 0;
};

#endif //IFONT_H
