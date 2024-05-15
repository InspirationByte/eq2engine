//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
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

#pragma once

// text styles
enum ETextStyleFlag : int
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
enum ETextAlignment : int
{
	TEXT_ALIGN_LEFT		= 0, // default flag
	TEXT_ALIGN_RIGHT	= (1 << 0), // has no right?
	TEXT_ALIGN_HCENTER	= (1 << 1),

	TEXT_ALIGN_TOP		= 0, // by default
	TEXT_ALIGN_BOTTOM	= (1 << 3),
	TEXT_ALIGN_VCENTER	= (1 << 4),
};

struct FontChar
{
	float x0{ 0.0f };
	float y0{ 0.0f };
	float x1{ 0.0f };
	float y1{ 0.0f };

	float ofsX{ 0.0f }; 
	float ofsY{ 0.0f };
	float advX{ 0.0f };
};

class IGPURenderPassRecorder;
class IEqFont;
struct FontStyleParam;

//-------------------------------------------------------------------------
// Font layout interface
//-------------------------------------------------------------------------
class ITextLayoutBuilder
{
public:
	virtual ~ITextLayoutBuilder() {}

	virtual void	Reset( IEqFont* font ) { m_font = font; }

	// controls the newline. For different text orientations
	virtual void	OnNewLine(	const FontStyleParam& params,
								void* strCurPos, bool isWideChar,
								int lineNumber,
								const Vector2D& textStart,
								Vector2D& curTextPos ) = 0;

	// for special layouts like rectangles
	// if false then stops output, and don't render this char
	virtual bool	LayoutChar( const FontStyleParam& params,
								void* strCurPos, bool isWideChar,
								const FontChar& chr,
								Vector2D& curTextPos,
								Vector2D& cPos, Vector2D& cSize ) = 0;

protected:
	IEqFont*	m_font{ nullptr };
};

// text
struct FontStyleParam
{
	ITextLayoutBuilder* layoutBuilder{ nullptr };

	int					align{ 0 };			// ETextAlignment
	int					styleFlag{ 0 };		// ETextStyleFlag

	MColor				textColor{ color_white };

	Vector2D			shadowOffset{ 1.0f };
	float				shadowWeight{ 0.01f };

	MColor				shadowColor{ 0 };
	float				shadowAlpha{ 0.7f };

	// SDF font size scaling
	Vector2D			scale{ 1.0f };
	float				textWeight{ 0.0f };
};

//-------------------------------------------------------------------------
// The font interface
//-------------------------------------------------------------------------
class IEqFont
{
public:
	virtual ~IEqFont() {}

	// returns string width in pixels
	virtual float				GetStringWidth( const wchar_t* str, const FontStyleParam& params, int charCount = -1, int breakOnChar = -1) const = 0;

	// returns string width in pixels
	virtual float				GetStringWidth( const char* str, const FontStyleParam& params, int charCount = -1, int breakOnChar = -1) const = 0;

	// returns font line height in pixels
	virtual float				GetLineHeight(const FontStyleParam& params) const = 0;

	// returns font baseline offset in pixels
	virtual float				GetBaselineOffs(const FontStyleParam& params) const = 0;

	// returns the character data
	virtual const FontChar&		GetFontCharById( const int chrId ) const = 0;

	// returns the scaled character
	virtual void				GetScaledCharacter( FontChar& chr, const int chrId, const Vector2D& scale = 1.0f ) const = 0;


	// renders text
	virtual void				SetupRenderText(const wchar_t* pszText,
											const Vector2D& start,
											const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder) = 0;

	// renders text
	virtual void				SetupRenderText(const char* pszText,
											const Vector2D& start,
											const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder) = 0;
};
