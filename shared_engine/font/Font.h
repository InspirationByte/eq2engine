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
struct RenderDrawCmd;
struct RenderPassContext;
class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class CFont : public IEqFont
{
	friend class CEqConsoleInput;
public:
	CFont();

	const char*		GetName() const { return m_name; }

	bool			LoadFont( const char* filenamePrefix );

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

	void			SetupDrawTextMeshBuffer(RenderDrawCmd& drawCmd, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder);

	// returns the character data
	const FontChar&	GetFontCharById( const int chrId ) const;

	// returns the scaled character
	void			GetScaledCharacter( FontChar& chr, const int chrId, const Vector2D& scale = 1.0f ) const;

	// builds vertex buffer for characters
	template <typename CHAR_T>
	void			BuildCharVertexBuffer(	CMeshBuilder& builder, 
											const CHAR_T* str, 
											const Vector2D& startPos, 
											const FontStyleParam& params);

	template <typename CHAR_T>
	float			_GetStringWidth(const CHAR_T* str, const FontStyleParam& params, int charCount = 0, int breakOnChar = -1) const;

	template <typename CHAR_T>
	int				GetTextQuadsCount(const CHAR_T *str, const FontStyleParam& params) const;

	// map of chars
	FontCharMap		m_charMap{ PP_SL };

	EqString		m_name;

	float			m_spacing{ 0.0f };
	float			m_baseline{ 0.0f };
	float			m_lineHeight{ 0.0f };

	Vector2D		m_scale{ 1.0f };
	Vector2D		m_invTexSize{ 1.0f };
	ITexturePtr		m_fontTexture;

	struct
	{
		bool	sdf		: 1;
		bool	bold	: 2;
	} m_flags;
};