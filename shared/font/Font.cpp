//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//////////////////////////////////////////////////////////////////////////////////

/*
TODO:
		- Text rendering shaders and effects
*/

#include "core/core_common.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"
#include "Font.h"
#include "FontCache.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "materialsystem1/ITextureLoader.h"

#define FONT_DEFAULT_PATH "resources/fonts/"

#if !defined(_RETAIL) || !defined(_PROFILE)
#define ENABLE_FONT_DEBUG_DRAWING
DECLARE_CVAR(r_font_debug, "0", nullptr, CV_CHEAT);
#endif

// TODO: really a font parameters!!!
DECLARE_CVAR(r_font_sdfStart, "0.94", nullptr, CV_CHEAT);
DECLARE_CVAR(r_font_sdfRange, "0.06", nullptr, CV_CHEAT);

static bool IsVisibleChar( int ch )
{
	return	(ch != '\n') && 
			(ch != '\r');
}

//-----------------------------------------------------------------------------------------

class CPlainTextLayoutBuilder : public ITextLayoutBuilder
{
public:
	// controls the newline. For different text orientations
	void	OnNewLine(	const FontStyleParam& params, 
						void* strCurPos, bool isWideChar,
						int lineNumber,
						const Vector2D& textStart,
						Vector2D& curTextPos );

	// for special layouts like rectangles
	// if false then stops output, and don't render this char
	bool	LayoutChar( const FontStyleParam& params,
						void* strCurPos, bool isWideChar,
						const FontChar& chr,
						Vector2D& curTextPos,
						Vector2D& cPos, Vector2D& cSize );
};

void CPlainTextLayoutBuilder::OnNewLine(const FontStyleParam& params, 
										void* strCurPos, bool isWideChar,
										int lineNumber,
										const Vector2D& textStart,
										Vector2D& curTextPos )
{
	if(lineNumber > 0)
		curTextPos.y += m_font->GetLineHeight(params);

	curTextPos.x = textStart.x;

	float newlineStringWidth;
	
	if(isWideChar)
		newlineStringWidth = m_font->GetStringWidth( (wchar_t*)strCurPos, params, -1, '\n' );
	else
		newlineStringWidth = m_font->GetStringWidth( (char*)strCurPos, params, -1, '\n' );

	// calc start position for first time
	if( params.align != TEXT_ALIGN_LEFT )
	{
		if(params.align & TEXT_ALIGN_HCENTER)
			curTextPos.x -= newlineStringWidth * 0.5f;
		else if(params.align & TEXT_ALIGN_RIGHT)
			curTextPos.x -= newlineStringWidth;

		curTextPos.x = floor(curTextPos.x);
	}
}

bool CPlainTextLayoutBuilder::LayoutChar(	const FontStyleParam& params,
											void* strCurPos, bool isWideChar,
											const FontChar& chr,
											Vector2D& curTextPos,
											Vector2D& cPos, Vector2D& cSize )
{
	IEqFont* font = m_font;

	if (isWideChar)
		curTextPos.x += font->GetStringWidth((wchar_t*)strCurPos, params, 1);
	else
		curTextPos.x += font->GetStringWidth((char*)strCurPos, params, 1);

	return true;
}

static CPlainTextLayoutBuilder s_defaultTextLayout;

//-----------------------------------------------------------------------------------------
CFont::CFont()
{
	memset(&m_flags, 1, sizeof(m_flags));
}

float CFont::GetStringWidth( const char* str, const FontStyleParam& params, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = CString::Length(str);

	return _GetStringWidth(str, params, charCount, breakOnChar);
}

float CFont::GetStringWidth( const wchar_t* str, const FontStyleParam& params, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = CString::Length(str);

	return _GetStringWidth(str, params, charCount, breakOnChar);
}

float CFont::GetLineHeight( const FontStyleParam& params ) const
{
	if(m_flags.sdf) // only scale SDF characters
		return m_lineHeight * params.scale.y;

	return m_lineHeight;
}

float CFont::GetBaselineOffs( const FontStyleParam& params ) const
{
	if(m_flags.sdf) // only scale SDF characters
		return m_baseline * params.scale.y;

	return m_baseline;
}

enum class ECharMode
{
	NORMAL = 0,
	TAG,
};

enum class ETextTag
{
	NONE = 0,
	COLOR,
	STYLE_FLAG,
};

struct TagProcessorState
{
	FixedList<FontStyleParam, 16>	styleStack;
	FontStyleParam					curStyle;

	ECharMode	mode = ECharMode::NORMAL;
	ETextTag	tagType = ETextTag::NONE;
	int			prevChar = 0;
	int			charIdx = 0;
};

// using CHAR_FUNC = bool(const CHAR_T* str, const FontStyleParam& fontStyle)

template <typename CHAR_T, typename CHAR_FUNC>
void ProcessTaggedText(const CHAR_T* str, const FontStyleParam& initialStyle, const CHAR_FUNC& processCharFunc)
{
	TagProcessorState tagState;
	for (; *str; ++str)
	{
		tagState.prevChar = tagState.charIdx;
		tagState.charIdx = *str;

		if (tagState.styleStack.getCount() == 0)
			tagState.styleStack.append(initialStyle);

		if (tagState.mode == ECharMode::NORMAL)
		{
			if (tagState.charIdx == '&')
			{
				tagState.curStyle = tagState.styleStack.back();
				tagState.mode = ECharMode::TAG;
			}
		}
		else if (tagState.mode == ECharMode::TAG)
		{
			if (tagState.prevChar == '&' && tagState.charIdx == '&')
			{
				// escape
				tagState.mode = ECharMode::NORMAL;
			}
			else if (tagState.charIdx == '#')
			{
				tagState.tagType = ETextTag::COLOR;
				tagState.curStyle.textColor = ColorRGBA(hexToColor3(str + 1), tagState.curStyle.textColor.a);
				str += 6;
			}
			else if (tagState.charIdx == '^')
			{
				// begin upper-case text
				tagState.tagType = ETextTag::STYLE_FLAG;
				tagState.curStyle.styleFlag |= TEXT_STYLE_UPPERCASE;
			}
			else if (tagState.charIdx == '_')
			{
				// begin lower-case text
				tagState.tagType = ETextTag::STYLE_FLAG;
				tagState.curStyle.styleFlag |= TEXT_STYLE_LOWERCASE;
			}
			else if (tagState.charIdx == ';')
			{
				if (tagState.tagType == ETextTag::NONE)
				{
					if (tagState.styleStack.getCount())
						tagState.styleStack.popBack();
				}
				else
				{
					tagState.styleStack.append(tagState.curStyle);
				}

				tagState.tagType = ETextTag::NONE;
				tagState.mode = ECharMode::NORMAL;
				tagState.prevChar = tagState.charIdx;
				continue;
			}
		}

		// skip characters in tag mode
		if (tagState.mode == ECharMode::TAG)
			continue;

		if (!processCharFunc(str, tagState.styleStack.back()))
			break;
	}
}

template <typename CHAR_T>
float CFont::_GetStringWidth( const CHAR_T* str, const FontStyleParam& params, int charCount, int breakOnChar) const
{
	float totalWidth = 0.0f;

	int count = 0;
	auto AddCharWidth = [&](const CHAR_T* str, const FontStyleParam& fontStyle) -> bool {
		if (count >= charCount)
			return false;

		if (breakOnChar != -1 && *str == breakOnChar)
			return false;

		if (!IsVisibleChar(*str))
			return true;

		FontChar chr;
		GetScaledCharacter(chr, *str, fontStyle);

		if (params.styleFlag & TEXT_STYLE_MONOSPACE)
			totalWidth += (chr.x1 - chr.x0) + m_spacing;
		else
			totalWidth += chr.advX + m_spacing;

		++count;

		return true;
	};

	if (params.styleFlag & TEXT_STYLE_USE_TAGS)
	{
		ProcessTaggedText(str, params, AddCharWidth);
	}
	else
	{
		for (; *str; ++str)
		{
			if (!AddCharWidth(str, params))
				break;
		}
	}

	return totalWidth;
	/*
	// parse
	FixedList<FontStyleParam, 16> styleStack;
	FontStyleParam parsedParams;

	ECharMode charMode = ECharMode::NORMAL;
	ETextTag tagType = ETextTag::NONE;
	int prevChar = 0;
	int charIdx = 0;

    for(int i = 0; i < charCount; i++)
	{
		prevChar = charIdx;
		charIdx = str[i];

		if (styleStack.getCount() == 0)
			styleStack.append(params);

		const FontStyleParam& stateParams = styleStack.back();

		//
		// Preprocessing part - text color and mode
		//
		if ((params.styleFlag & TEXT_STYLE_USE_TAGS) && charMode == ECharMode::NORMAL && charIdx == '&')
		{
			charMode = ECharMode::TAG;
			i++;
			continue;
		}

		if (charMode == ECharMode::TAG)
		{
			if (prevChar == '&' && charIdx == '&')
			{
				// escape
				charMode = ECharMode::NORMAL;
			}
			else if (charIdx == '#')
			{
				parsedParams = stateParams;
				tagType = ETextTag::COLOR;

				i++;
				i += 6;

				continue;
			}
			else if (charIdx == '^')
			{
				// begin upper-case text
				i++;
				tagType = ETextTag::STYLE_FLAG;
				parsedParams = stateParams;
				parsedParams.styleFlag |= TEXT_STYLE_UPPERCASE;
				continue;
			}
			else if (charIdx == '_')
			{
				// begin lower-case text
				i++;
				tagType = ETextTag::STYLE_FLAG;
				parsedParams = stateParams;
				parsedParams.styleFlag |= TEXT_STYLE_LOWERCASE;
				continue;
			}
			else if (charIdx == ';')
			{
				if (tagType == ETextTag::NONE)
				{
					if (styleStack.getCount())
						styleStack.popBack();
				}
				else
				{
					styleStack.append(parsedParams);
				}

				tagType = ETextTag::NONE;
				charMode = ECharMode::NORMAL;
				prevChar = charIdx;
				i++;
				continue;
			}
		}

		if(breakOnChar != -1 && charIdx == breakOnChar)
			break;

		if(!IsVisibleChar(charIdx))
			continue;

		FontChar chr;
		GetScaledCharacter(chr, charIdx, params);

		if( params.styleFlag & TEXT_STYLE_MONOSPACE)
			totalWidth += (chr.x1 - chr.x0) + m_spacing;
		else
			totalWidth += chr.advX + m_spacing;
	}

    return totalWidth;*/
}

//
// Fills text buffer and processes tags
//
template <typename CHAR_T>
void CFont::BuildCharVertexBuffer(CMeshBuilder& builder, const CHAR_T* str, const Vector2D& textPos, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	const bool isWideChar = std::is_same<CHAR_T,wchar_t>::value;

	ITextLayoutBuilder* layoutBuilder = params.layoutBuilder ? params.layoutBuilder : &s_defaultTextLayout;

#ifdef ENABLE_FONT_DEBUG_DRAWING
	CMeshBuilder dbgMeshBuilder;
	if (r_font_debug.GetBool())
	{
		rendPassRecorder->SetScissorRectangle(IAARectangle(0, 0, COM_INT_MAX, COM_INT_MAX));

		dbgMeshBuilder.Init(g_matSystem->GetDynamicMesh());
		dbgMeshBuilder.Begin(PRIM_LINES);

		layoutBuilder->DebugDraw(dbgMeshBuilder);

		Vector2D startTemp = textPos;
		layoutBuilder->Reset(this);
		layoutBuilder->OnNewLine(params, (void*)str, sizeof(CHAR_T) == sizeof(wchar_t), 0, textPos, startTemp);

		const float width = GetStringWidth(str, params);
		float offset = 0.0f;

		dbgMeshBuilder.Color4f(1.0f, 0.5f, 0.0f, 1.0f);
		dbgMeshBuilder.Line2fv(startTemp, startTemp + Vector2D(width, 0.0f));
	}
#endif

	layoutBuilder->Reset(this);
	Vector2D curStartPos = textPos;
	bool hasNewLine = true;
	int lineNumber = 0;

	auto DrawFontCharacter = [&](const CHAR_T* str, const FontStyleParam& fontStyle) -> bool {
		//
		// reset startpos
		//
		if (hasNewLine)
		{
			layoutBuilder->OnNewLine(fontStyle, (void*)str, isWideChar, lineNumber, textPos, curStartPos);
			hasNewLine = false;
		}

		if (*str == '\n')
		{
			lineNumber++;
			hasNewLine = true;
			return true;
		}

		if (!IsVisibleChar(*str))
			return true;

		//
		// Render part - text filling
		//
		FontChar chr;
		GetScaledCharacter(chr, *str, fontStyle);

		// build default character pos and size
		const float baseLine = GetBaselineOffs(fontStyle);
		Vector2D cPos(curStartPos.x + chr.ofsX, curStartPos.y - baseLine + chr.ofsY);
		Vector2D cSize(chr.x1 - chr.x0, chr.y1 - chr.y0);

		if (m_flags.sdf) // only scale SDF characters
			cSize *= m_scale * fontStyle.scale;

		if (!layoutBuilder->LayoutChar(fontStyle, (void*)str, isWideChar, chr, curStartPos, cPos, cSize))
		{
#ifdef ENABLE_FONT_DEBUG_DRAWING
			if (r_font_debug.GetBool())
			{
				AARectangle charRect(cPos, cPos + cSize);
				dbgMeshBuilder.Color4f(1.0f, 0.0f, 0.0f, 1.0f);
				dbgMeshBuilder.Line2fv(charRect.GetLeftTop(), charRect.GetRightTop());
				dbgMeshBuilder.Line2fv(charRect.GetRightTop(), charRect.GetRightBottom());
				dbgMeshBuilder.Line2fv(charRect.GetRightBottom(), charRect.GetLeftBottom());
				dbgMeshBuilder.Line2fv(charRect.GetLeftBottom(), charRect.GetLeftTop());
			}
#endif
			return false;
		}

		if (fontStyle.styleFlag & TEXT_STYLE_FROM_CAP)
			cPos.y = curStartPos.y - (cSize.y - baseLine) + chr.ofsY;

		// set character color
		builder.Color4fv(fontStyle.textColor);

		AARectangle charRect(cPos, cPos + cSize);

#ifdef ENABLE_FONT_DEBUG_DRAWING
		if (r_font_debug.GetBool())
		{
			builder.Color4fv(fontStyle.textColor.v * Vector4D(color_white.v.xyz(), 0.75f));

			dbgMeshBuilder.Color4f(1.0f, 1.0f, 1.0f, 0.5f);
			dbgMeshBuilder.Line2fv(charRect.GetLeftTop(), charRect.GetRightTop());
			dbgMeshBuilder.Line2fv(charRect.GetRightTop(), charRect.GetRightBottom());
			dbgMeshBuilder.Line2fv(charRect.GetRightBottom(), charRect.GetLeftBottom());
			dbgMeshBuilder.Line2fv(charRect.GetLeftBottom(), charRect.GetLeftTop());
		}
#endif

		const AARectangle charTexCoord(chr.x0 * m_invTexSize.x, chr.y0 * m_invTexSize.y, chr.x1 * m_invTexSize.x, chr.y1 * m_invTexSize.y);

		// use meshbuilder's index buffer optimization feature
		builder.TexturedQuad2(charRect.GetLeftTop(), charRect.GetRightTop(), charRect.GetLeftBottom(), charRect.GetRightBottom(),
			charTexCoord.GetLeftTop(), charTexCoord.GetRightTop(), charTexCoord.GetLeftBottom(), charTexCoord.GetRightBottom());

		return true;
	};

	if (params.styleFlag & TEXT_STYLE_USE_TAGS)
	{
		ProcessTaggedText(str, params, DrawFontCharacter);
	}
	else
	{
		for (; *str; ++str)
		{
			if (!DrawFontCharacter(str, params))
				break;
		}
	}

#ifdef ENABLE_FONT_DEBUG_DRAWING
	if (r_font_debug.GetBool())
	{
		RenderDrawCmd drawCmd;
		drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

		MatSysDefaultRenderPass defaultRenderPass;
		defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
		RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);

		if (dbgMeshBuilder.End(drawCmd))
			g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
	}
#endif
}

// renders text (wide char)
void CFont::SetupRenderText(const wchar_t* pszText, const Vector2D& start, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	if (GetTextQuadsCount(pszText, params) == 0)
		return;

	IDynamicMeshPtr dynMesh = g_matSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	RenderDrawCmd drawCmd;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
	BuildCharVertexBuffer(meshBuilder, pszText, start, params, rendPassRecorder);
	if (meshBuilder.End(drawCmd))
		SetupDrawTextMeshBuffer(drawCmd, params, rendPassRecorder);
}

// renders text (ASCII)
void CFont::SetupRenderText(const char* pszText, const Vector2D& start, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	if (GetTextQuadsCount(pszText, params) == 0)
		return;

	IDynamicMeshPtr dynMesh = g_matSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	RenderDrawCmd drawCmd;
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
	BuildCharVertexBuffer(meshBuilder, pszText, start, params, rendPassRecorder);
	if (meshBuilder.End(drawCmd))
		SetupDrawTextMeshBuffer(drawCmd, params, rendPassRecorder);
}

void CFont::SetupDrawTextMeshBuffer(RenderDrawCmd& drawCmd, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	CEqFontCache* fontCache = ((CEqFontCache*)g_fontCache);

	drawCmd.SetMaterial(fontCache->m_sdfMaterial);
	MatVec4Proxy& sdfRange = fontCache->m_fontParams;
	MatVec4Proxy& baseColor = fontCache->m_fontBaseColor;
	MatVec4Proxy& shadowColor = fontCache->m_shadowColor;
	MatVec4Proxy& shadowSdfRange = fontCache->m_shadowParams;
	MatVec2Proxy& shadowOffset = fontCache->m_shadowOffset;

	baseColor.Set(color_white);

	// draw shadow
	// TODO: shadow color should be separate from text vertices color!!!
	if ((params.styleFlag & TEXT_STYLE_SHADOW) && params.shadowAlpha > 0.0f)
	{
		shadowColor.Set(ColorRGBA(params.shadowColor, params.shadowAlpha));
		shadowOffset.Set(params.shadowOffset / m_fontTexture->GetSize().xy());

		if (m_flags.sdf)
		{
			// shadow width
			const float sdfEndClamped = clamp(r_font_sdfRange.GetFloat() + params.shadowWeight, 0.0f, 1.0f - r_font_sdfStart.GetFloat());
			shadowSdfRange.Set(Vector4D(r_font_sdfStart.GetFloat() - params.shadowWeight, sdfEndClamped, 0.0f, 0.0f));
		}
		else
			shadowSdfRange.Set(Vector4D(0.0f, 1.0f, 0.0f, 1.0f));
	}
	else
		shadowColor.Set(vec4_zero);

	if (m_flags.sdf)
	{
		const float sdfEndClamped = clamp(r_font_sdfRange.GetFloat() + params.textWeight, 0.0f, 1.0f - r_font_sdfStart.GetFloat());
		sdfRange.Set(Vector4D(r_font_sdfStart.GetFloat() - params.textWeight, sdfEndClamped, 1.0f, 0.0f));
	}
	else
		sdfRange.Set(Vector4D(0.0f, 1.0f, 1.0f, 1.0f));

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.texture = m_fontTexture;

	RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);
	g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
}

//
// returns font character information
//
const FontChar&	CFont::GetFontCharById( const int chrId ) const
{
	static FontChar null_default;

	auto it = m_charMap.find(chrId);

	if(it.atEnd())
		return null_default;
	
	return *it;
}

//
// returns the scaled character
//
void CFont::GetScaledCharacter( FontChar& chr, const int chrId, const FontStyleParam& params) const
{
	int charId = chrId;
	if (params.styleFlag & TEXT_STYLE_UPPERCASE)
		charId = CType::UpperChar<wchar_t>(chrId);
	else if (params.styleFlag & TEXT_STYLE_LOWERCASE)
		charId = CType::LowerChar<wchar_t>(chrId);

	chr = GetFontCharById(charId);

	if(m_flags.sdf) // only scale SDF characters
	{
		chr.advX = chr.advX * params.scale.x;
		chr.ofsX = chr.ofsX * params.scale.x;
		chr.ofsY = chr.ofsY * params.scale.y;
	}
}

//
// returns maximum of possible quads to be allocated
//
template <typename CHAR_T>
int CFont::GetTextQuadsCount(const CHAR_T* str, const FontStyleParam& params) const
{
	int n = 0;

	ECharMode charMode = ECharMode::NORMAL;
	int prevChar = 0;
	int charIdx = 0;

	while (*str)
	{
		prevChar = charIdx;
		charIdx = *str;

		// skip fasttags
		if( (params.styleFlag & TEXT_STYLE_USE_TAGS) &&
			charMode == ECharMode::NORMAL &&
			charIdx == '&')
		{
			charMode = ECharMode::TAG;
			str++;
			continue;
		}

		if(charMode == ECharMode::TAG)
		{
			if(charIdx == ';' || prevChar == '&' && charIdx == '&')
				charMode = ECharMode::NORMAL;

			str++;

			continue;
		}

		if(IsVisibleChar(charIdx))
			n++;

		str++;
	}

	return n;
}

bool CFont::LoadFont( const char* filenamePrefix )
{
	m_name = filenamePrefix;

	const EqString finalFileName = _Es(FONT_DEFAULT_PATH) + m_name + _Es(".fnt");

	KVSection kvs;
	if( !KV_LoadFromFile(finalFileName, -1, kvs) )
	{
		MsgError("ERROR: Can't open font file '%s' (%s)\n", finalFileName.ToCString(), filenamePrefix);
		return false;
	}

	const KVSection* fontSec = kvs.FindSection("Font", KV_FLAG_SECTION);
	if(fontSec)
	{
		const int charWide = KV_GetValueInt(fontSec->FindSection("charWidth"), 0, 8);
		const int charTall = KV_GetValueInt(fontSec->FindSection("charHeight"), 0, 8);
		const int charsPerLine = KV_GetValueInt(fontSec->FindSection("charsPerLine"), 0, 16);
		const float interval = KV_GetValueFloat(fontSec->FindSection("interval"), 0, 0.75);

		{
			const EqString textureName = KV_GetValueString(fontSec->FindSection("texture"), 0, filenamePrefix);
			const bool filterTexture = KV_GetValueBool(fontSec->FindSection("filterFontTexture"));
			SamplerStateParams samplerParams(filterTexture ? TEXFILTER_LINEAR : TEXFILTER_NEAREST, TEXADDRESS_WRAP);
			m_fontTexture = g_texLoader->LoadTextureFromFileSync(textureName, samplerParams, TEXFLAG_IGNORE_QUALITY, finalFileName);
		}

		if (m_fontTexture == nullptr)
		{
			MsgError("ERROR: no texture for font '%s'\n", finalFileName.GetData());
			return false;
		}

		m_invTexSize = 1.0f / m_fontTexture->GetSize().xy();

		m_spacing = KV_GetValueFloat(fontSec->FindSection("spacing"));
		m_baseline = charTall;
		m_lineHeight = charTall + 4;

		int line = 0;
		int lChars = 0;
		for (int i = 0; i < 256; i++)
		{
			if (lChars == charsPerLine)
			{
				line++;
				lChars = 0;
			}

			FontChar& chr = m_charMap[i];

			const float CurCharPos_x = lChars * charTall;
			const float CurCharPos_y = line * charTall;

			chr.x0 = CurCharPos_x + interval; // LineCharCount * interval
			chr.x1 = CurCharPos_x + interval + charWide;
			chr.y0 = CurCharPos_y + interval; // Line Count * height
			chr.y1 = CurCharPos_y + interval + charTall;

			chr.advX = (chr.x1 - chr.x0) + m_spacing;
			chr.ofsX = 0;
			chr.ofsY = 0;

			lChars++;
		}

		return true;
	}

	fontSec = kvs.FindSection("eqFont");
	if (fontSec)
	{
		m_flags.sdf = KV_GetValueBool(fontSec->FindSection("isSDF"));
		m_flags.bold = KV_GetValueBool(fontSec->FindSection("bold"));

		{
			const EqString textureName = KV_GetValueString(fontSec->FindSection("texture"), 0, filenamePrefix);
			const bool filterTexture = KV_GetValueBool(fontSec->FindSection("filter")) || m_flags.sdf;
			SamplerStateParams samplerParams(filterTexture ? TEXFILTER_LINEAR : TEXFILTER_NEAREST, TEXADDRESS_WRAP);
			m_fontTexture = g_texLoader->LoadTextureFromFileSync(textureName, samplerParams, TEXFLAG_IGNORE_QUALITY, finalFileName);
		}

		if (m_fontTexture == nullptr)
		{
			MsgError("ERROR: no texture for font '%s'\n", finalFileName.GetData());
			return false;
		}

		m_invTexSize = 1.0f / m_fontTexture->GetSize().xy();
		m_spacing = 0.0f;

		if (m_flags.sdf)
			m_scale = KV_GetVector2D(fontSec->FindSection("scale"));

		m_baseline = KV_GetValueFloat(fontSec->FindSection("baseline")) * m_scale.y;
		m_lineHeight = KV_GetValueFloat(fontSec->FindSection("lineheight")) * m_scale.y;

		for (const KVSection& charSec : fontSec->Keys())
		{
			// x y w h ox oy advanceX
			// 0 1 2 3 4  5  6

			FontChar fontChar;
			const int valueCount = charSec.GetValues(
				fontChar.x0,
				fontChar.y0,
				fontChar.x1,
				fontChar.y1,
				fontChar.ofsX,
				fontChar.ofsY,
				fontChar.advX
			);

			if(valueCount != 7)
			{
				if(valueCount > 2)
				{
					ASSERT_FAIL("Invalid font file %s", filenamePrefix);
				}
				continue;
			}

			fontChar.x1 += fontChar.x0;
			fontChar.y1 += fontChar.y0;

			fontChar.ofsX *= m_scale.x;
			fontChar.ofsY *= m_scale.y;
			fontChar.advX *= m_scale.x;

			const int charIdx = atoi(charSec.GetName());
			m_charMap.insert(charIdx, fontChar);
		}

		return true;
	}

	MsgError("ERROR: '%s' is not a valid font file\n", finalFileName.ToCString());

	return false;
}