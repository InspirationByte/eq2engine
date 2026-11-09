//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "font/IFont.h"

class CMeshBuilder;
struct KVSection;
struct RenderDrawCmd;
struct RenderPassContext;
struct SamplerStateParams;
class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

enum EFontType : int
{
	FONT_TYPE_BITMAP = 0,	// bitmap glyphs, not scaled by FontStyleParam::scale
	FONT_TYPE_SDF,			// legacy BMFont SDF in alpha, edge set by r_font_sdfStart / r_font_sdfRange
	FONT_TYPE_MSDF,			// eqfont2 msdf / sdf / psdf atlas, distance is the median of RGB
	FONT_TYPE_MTSDF,		// eqfont2 mtsdf atlas, MSDF in RGB and true SDF in alpha
};

class CFont : public IEqFont
{
	friend class CEqConsoleInput;
public:
	CFont();

	const char*		GetName() const { return m_name; }

	bool			LoadFont( const char* filenamePrefix );

	// returns kerning between two characters in pixels
	float			GetKerning( int firstChrId, int secondChrId, const FontStyleParam& params ) const;

	// returns string width in pixels
	float			GetStringWidth( const wchar_t* str, const FontStyleParam& params, int charCount = -1, int breakOnChar = -1) const;
	float			GetStringWidth( const char* str, const FontStyleParam& params, int charCount = -1, int breakOnChar = -1) const;

	float			GetLineHeight( const FontStyleParam& params ) const;
	float			GetBaselineOffs( const FontStyleParam& params ) const;

	// renders text (wide char)
	void			SetupRenderText(const wchar_t* pszText,
								const Vector2D& start,
								const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder);
	// renders text (ASCII)
	void			SetupRenderText(const char* pszText,
								const Vector2D& start,
								const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder);

protected:

	using FontCharMap = Map<ushort, FontChar>;
	using FontKerningMap = Map<uint, float>;

	bool			LoadFontGrid( const KVSection* fontSec, const char* finalFileName );
	bool			LoadFontV1( const KVSection* fontSec, const char* finalFileName );
	bool			LoadFontV2( const KVSection* fontSec, const char* finalFileName );
	bool			LoadFontTexture( const KVSection* fontSec, const SamplerStateParams& samplerParams, const char* finalFileName );

	// glyphs are scaled by FontStyleParam::scale and drawn with distance field shading
	bool			IsDistanceField() const { return m_type != FONT_TYPE_BITMAP; }

	// eqfont2 distance field atlas
	bool			IsMSDF() const { return m_type == FONT_TYPE_MSDF || m_type == FONT_TYPE_MTSDF; }

	void			SetupDrawTextMeshBuffer(RenderDrawCmd& drawCmd, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder);

	// returns the character data
	const FontChar&	GetFontCharById( const int chrId ) const;

	// returns the scaled character
	void			GetScaledCharacter( FontChar& chr, const int chrId, const FontStyleParam& params) const;

	// builds vertex buffer for characters
	template <typename CHAR_T>
	void			BuildCharVertexBuffer(	CMeshBuilder& builder, 
											const CHAR_T* str, 
											const Vector2D& startPos, 
											const FontStyleParam& params, 
											IGPURenderPassRecorder* rendPassRecorder);

	template <typename CHAR_T>
	float			_GetStringWidth(const CHAR_T* str, const FontStyleParam& params, int charCount = 0, int breakOnChar = -1) const;

	template <typename CHAR_T>
	int				GetTextQuadsCount(const CHAR_T *str, const FontStyleParam& params) const;

	// map of chars
	FontCharMap		m_charMap{ PP_SL };
	FontKerningMap	m_kerning{ PP_SL };

	EqString		m_name;

	float			m_spacing{ 0.0f };
	float			m_baseline{ 0.0f };
	float			m_lineHeight{ 0.0f };
	float			m_distanceRange{ 0.0f };	// eqfont2 distance range in atlas pixels

	Vector2D		m_scale{ 1.0f };
	Vector2D		m_invTexSize{ 1.0f };
	ITexturePtr		m_fontTexture;

	EFontType		m_type{ FONT_TYPE_BITMAP };
};