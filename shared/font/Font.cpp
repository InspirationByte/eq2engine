//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//////////////////////////////////////////////////////////////////////////////////

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

DECLARE_CVAR(r_font_sdfStart, "0.94", nullptr, CV_CHEAT);
DECLARE_CVAR(r_font_sdfRange, "0.06", nullptr, CV_CHEAT);

static bool IsVisibleChar( int ch )
{
	return	(ch != '\n') &&
			(ch != '\r');
}

static int ApplyCharCase( int chrId, const FontStyleParam& params )
{
	if (params.styleFlag & TEXT_STYLE_UPPERCASE)
		return CType::UpperChar<wchar_t>(chrId);
	else if (params.styleFlag & TEXT_STYLE_LOWERCASE)
		return CType::LowerChar<wchar_t>(chrId);
	return chrId;
}

static uint KerningPairKey( int firstChrId, int secondChrId )
{
	return (static_cast<uint>(firstChrId & 0xFFFF) << 16) | static_cast<uint>(secondChrId & 0xFFFF);
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
	if(IsDistanceField()) // only scale SDF characters
		return m_lineHeight * params.scale.y;

	return m_lineHeight;
}

float CFont::GetBaselineOffs( const FontStyleParam& params ) const
{
	if(IsDistanceField()) // only scale SDF characters
		return m_baseline * params.scale.y;

	return m_baseline;
}

float CFont::GetKerning( int firstChrId, int secondChrId, const FontStyleParam& params ) const
{
	if (m_kerning.isEmpty() || firstChrId == 0)
		return 0.0f;

	auto it = m_kerning.find(KerningPairKey(ApplyCharCase(firstChrId, params), ApplyCharCase(secondChrId, params)));
	if (it.atEnd())
		return 0.0f;

	// kerning is stored in ems, same as advance
	return *it * params.scale.x;
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
	int prevChr = 0;
	auto AddCharWidth = [&](const CHAR_T* str, const FontStyleParam& fontStyle) -> bool {
		if (count >= charCount)
			return false;

		if (breakOnChar != -1 && *str == breakOnChar)
			return false;

		if (!IsVisibleChar(*str))
		{
			prevChr = 0;
			return true;
		}

		totalWidth += GetKerning(prevChr, *str, fontStyle);
		prevChr = *str;

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
	int prevChr = 0;

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
			prevChr = 0;
			return true;
		}

		if (!IsVisibleChar(*str))
			return true;

		curStartPos.x += GetKerning(prevChr, *str, fontStyle);
		prevChr = *str;

		//
		// Render part - text filling
		//
		FontChar chr;
		GetScaledCharacter(chr, *str, fontStyle);

		// build default character pos and size
		const float baseLine = GetBaselineOffs(fontStyle);
		Vector2D cPos(curStartPos.x + chr.ofsX, curStartPos.y - baseLine + chr.ofsY);
		Vector2D cSize(chr.x1 - chr.x0, chr.y1 - chr.y0);

		if (IsDistanceField()) // only scale SDF characters
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
	MatVec4Proxy& msdfParams = fontCache->m_msdfParams;

	baseColor.Set(color_white);

	if (IsMSDF())
	{
		// FontParams.w >= 2 selects the eqfont2 distance field path in the shader (3 - use alpha SDF).
		// Weights are offsets of the edge in normalized distance (0.5 is the glyph edge)
		sdfRange.Set(Vector4D(params.textWeight, 0.0f, 1.0f, 2.0f));
		msdfParams.Set(Vector4D(m_distanceRange * m_invTexSize, 0.0f, 0.0f));

		if ((params.styleFlag & TEXT_STYLE_SHADOW) && params.shadowAlpha > 0.0f)
		{
			// shadow offset is in screen pixels, convert it to atlas texture coordinates
			const Vector2D screenPixelsPerAtlasPixel = m_scale * params.scale;
			const Vector2D atlasPixelsPerScreenPixel(1.0f / max(screenPixelsPerAtlasPixel.x, F_EPS), 1.0f / max(screenPixelsPerAtlasPixel.y, F_EPS));
			shadowColor.Set(ColorRGBA(params.shadowColor, params.shadowAlpha));
			shadowOffset.Set(params.shadowOffset * atlasPixelsPerScreenPixel * m_invTexSize);

			// shadow weight expands and softens the shadow like legacy SDF fonts do.
			// MTSDF keeps a true SDF in alpha which has no corner artifacts when expanded
			shadowSdfRange.Set(Vector4D(params.shadowWeight, params.shadowWeight, 0.0f, (m_type == FONT_TYPE_MTSDF) ? 3.0f : 2.0f));
		}
		else
			shadowColor.Set(vec4_zero);

		MatSysDefaultRenderPass defaultRenderPass;
		defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
		defaultRenderPass.texture = m_fontTexture;

		RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
		return;
	}

	msdfParams.Set(vec4_zero);

	// draw shadow
	if ((params.styleFlag & TEXT_STYLE_SHADOW) && params.shadowAlpha > 0.0f)
	{
		shadowColor.Set(ColorRGBA(params.shadowColor, params.shadowAlpha));
		shadowOffset.Set(params.shadowOffset / m_fontTexture->GetSize().xy());

		if (IsDistanceField())
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

	if (IsDistanceField())
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

	if(IsDistanceField()) // only scale SDF characters
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

	if (const KVSection* fontSec = kvs.FindSection("Font", KV_FLAG_SECTION))
		return LoadFontGrid(fontSec, finalFileName);

	if (const KVSection* fontSec = kvs.FindSection("eqFont"))
		return LoadFontV1(fontSec, finalFileName);

	if (const KVSection* fontSec = kvs.FindSection("eqfont2", KV_FLAG_SECTION))
		return LoadFontV2(fontSec, finalFileName);

	MsgError("ERROR: '%s' is not a valid font file\n", finalFileName.ToCString());

	return false;
}

// loads the texture from the 'texture' key (font name by default)
bool CFont::LoadFontTexture(const KVSection* fontSec, const SamplerStateParams& samplerParams, const char* finalFileName)
{
	const EqString textureName = KV_GetValueString(fontSec->FindSection("texture"), 0, m_name.ToCString());
	m_fontTexture = g_texLoader->LoadTextureFromFileSync(textureName, samplerParams, TEXFLAG_IGNORE_QUALITY, finalFileName);

	if (m_fontTexture == nullptr)
	{
		MsgError("ERROR: no texture for font '%s'\n", finalFileName);
		return false;
	}

	m_invTexSize = 1.0f / m_fontTexture->GetSize().xy();
	return true;
}

// 'Font' - fixed size glyph grid of 256 characters
bool CFont::LoadFontGrid(const KVSection* fontSec, const char* finalFileName)
{
	const int charWide = KV_GetValueInt(fontSec->FindSection("charWidth"), 0, 8);
	const int charTall = KV_GetValueInt(fontSec->FindSection("charHeight"), 0, 8);
	const int charsPerLine = KV_GetValueInt(fontSec->FindSection("charsPerLine"), 0, 16);
	const float interval = KV_GetValueFloat(fontSec->FindSection("interval"), 0, 0.75);

	// grid fonts are scaled by FontStyleParam::scale and shaded like legacy SDF fonts
	m_type = FONT_TYPE_SDF;

	const bool filterTexture = KV_GetValueBool(fontSec->FindSection("filterFontTexture"));
	if (!LoadFontTexture(fontSec, SamplerStateParams(filterTexture ? TEXFILTER_LINEAR : TEXFILTER_NEAREST, TEXADDRESS_WRAP), finalFileName))
		return false;

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

// 'eqFont' - BMFont export, bitmap or legacy SDF
bool CFont::LoadFontV1(const KVSection* fontSec, const char* finalFileName)
{
	m_type = KV_GetValueBool(fontSec->FindSection("isSDF")) ? FONT_TYPE_SDF : FONT_TYPE_BITMAP;

	const bool filterTexture = KV_GetValueBool(fontSec->FindSection("filter")) || IsDistanceField();
	if (!LoadFontTexture(fontSec, SamplerStateParams(filterTexture ? TEXFILTER_LINEAR : TEXFILTER_NEAREST, TEXADDRESS_WRAP), finalFileName))
		return false;

	m_spacing = 0.0f;

	if (IsDistanceField())
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
				ASSERT_FAIL("Invalid font file %s", m_name.ToCString());
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

// 'eqfont2' is produced from msdf-atlas-gen output by tools/fontconv/msdf_to_eqfont.py.
// Metrics are in ems with Y axis down and origin at the baseline, atlas bounds are in pixels.
bool CFont::LoadFontV2(const KVSection* fontSec, const char* finalFileName)
{
	const EqStringRef atlasType = KV_GetValueString(fontSec->FindSection("type"), 0, "mtsdf");
	if (atlasType == "mtsdf")
		m_type = FONT_TYPE_MTSDF;
	else if (atlasType == "msdf" || atlasType == "sdf" || atlasType == "psdf")
		m_type = FONT_TYPE_MSDF;
	else
	{
		MsgError("ERROR: font '%s' has unsupported atlas type '%s'\n", finalFileName, atlasType.ToCString());
		return false;
	}

	const float atlasEmSize = KV_GetValueFloat(fontSec->FindSection("size"), 0, 0.0f);
	if (atlasEmSize <= 0.0f)
	{
		MsgError("ERROR: font '%s' has invalid atlas size\n", finalFileName);
		return false;
	}

	if (!LoadFontTexture(fontSec, SamplerStateParams(TEXFILTER_LINEAR, TEXADDRESS_CLAMP), finalFileName))
		return false;

	m_distanceRange = KV_GetValueFloat(fontSec->FindSection("distanceRange"), 0, 2.0f);
	m_spacing = 0.0f;

	// glyph quads are sized from atlas pixels, FontStyleParam::scale is the em size in pixels
	m_scale = Vector2D(1.0f / atlasEmSize);

	const float ascender = KV_GetValueFloat(fontSec->FindSection("ascender"));
	m_baseline = ascender;
	m_lineHeight = KV_GetValueFloat(fontSec->FindSection("lineheight"), 0, 1.0f);

	for (const KVSection& keySec : fontSec->Keys())
	{
		if (!CString::CompareCaseIns(keySec.GetName(), "glyph"))
		{
			// unicode advance [planeLeft planeTop planeRight planeBottom atlasLeft atlasTop atlasRight atlasBottom]
			int charIdx = 0;
			float advance = 0.0f;
			float planeLeft = 0.0f, planeTop = 0.0f, planeRight = 0.0f, planeBottom = 0.0f;
			float atlasLeft = 0.0f, atlasTop = 0.0f, atlasRight = 0.0f, atlasBottom = 0.0f;

			const int valueCount = keySec.GetValues(charIdx, advance,
				planeLeft, planeTop, planeRight, planeBottom,
				atlasLeft, atlasTop, atlasRight, atlasBottom);

			if (valueCount != 2 && valueCount != 10)
			{
				MsgError("ERROR: font '%s' has invalid glyph entry\n", finalFileName);
				continue;
			}

			FontChar fontChar;
			fontChar.advX = advance;
			if (valueCount == 10)
			{
				fontChar.x0 = atlasLeft;
				fontChar.y0 = atlasTop;
				fontChar.x1 = atlasRight;
				fontChar.y1 = atlasBottom;

				// text drawing subtracts the baseline offset, compensate it here
				fontChar.ofsX = planeLeft;
				fontChar.ofsY = planeTop + ascender;
			}

			m_charMap.insert(charIdx, fontChar);
		}
		else if (!CString::CompareCaseIns(keySec.GetName(), "kern"))
		{
			int firstChrId = 0;
			int secondChrId = 0;
			float kerning = 0.0f;
			if (keySec.GetValues(firstChrId, secondChrId, kerning) == 3)
				m_kerning.insert(KerningPairKey(firstChrId, secondChrId), kerning);
		}
	}

	return true;
}