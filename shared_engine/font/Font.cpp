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

enum ECharMode
{
	CHARMODE_NORMAL = 0,
	CHARMODE_TAG,
};

enum ETextTagType
{
	TEXT_TAG_NONE = 0,
	TEXT_TAG_COLOR,
};

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
		charCount = strlen(str);

	return _GetStringWidth(str, params, charCount, breakOnChar);
}

float CFont::GetStringWidth( const wchar_t* str, const FontStyleParam& params, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = wcslen(str);

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

template <typename CHAR_T>
float CFont::_GetStringWidth( const CHAR_T* str, const FontStyleParam& params, int charCount, int breakOnChar) const
{
    float totalWidth = 0.0f;

	// parse
	int charMode = CHARMODE_NORMAL;
	int prevChar = 0;
	int charIdx = 0;
    for(int i = 0; i < charCount; i++)
	{
		prevChar = charIdx;
		charIdx = str[i];

		// skip fasttags
		if( (params.styleFlag & TEXT_STYLE_USE_TAGS) &&
			charMode == CHARMODE_NORMAL &&
			charIdx == '&')
		{
			charMode = CHARMODE_TAG;
			continue;
		}

		if(charMode == CHARMODE_TAG)
		{
			if(charIdx == ';' || prevChar == '&' && charIdx == '&')
				charMode = CHARMODE_NORMAL;

			continue;
		}

		if(breakOnChar != -1 && charIdx == breakOnChar)
			break;

		if(!IsVisibleChar(charIdx))
			continue;

		FontChar chr;
		GetScaledCharacter( chr, charIdx, params.scale );

		if( params.styleFlag & TEXT_STYLE_MONOSPACE)
			totalWidth += (chr.x1 - chr.x0) + m_spacing;
		else
			totalWidth += chr.advX + m_spacing; // chr.x1-chr.x0;
	}

    return totalWidth;
}

//
// Fills text buffer and processes tags
//
template <typename CHAR_T>
void CFont::BuildCharVertexBuffer(CMeshBuilder& builder, const CHAR_T* str, const Vector2D& textPos, const FontStyleParam& params)
{
	const bool isWideChar = std::is_same<CHAR_T,wchar_t>::value;

	ITextLayoutBuilder* layoutBuilder = &s_defaultTextLayout;

	if(params.layoutBuilder)
		layoutBuilder = params.layoutBuilder;

	layoutBuilder->Reset( this );

	Vector2D startPos = textPos;

	bool hasNewLine = true;
	int lineNumber = 0;

	FixedList<FontStyleParam, 8> states;
	states.append( params );	// push this param

	int charMode = CHARMODE_NORMAL;
	int tagType = TEXT_TAG_NONE;
	int prevChar = 0;
	int charIdx = 0;

	FontStyleParam parsedParams;

    while( *str )
	{
		prevChar = charIdx;
		charIdx = *str;

		const FontStyleParam& stateParams = states.back();

		//
		// Preprocessing part - text color and mode
		//
		if( (params.styleFlag & TEXT_STYLE_USE_TAGS) && 
			charMode == CHARMODE_NORMAL &&
			charIdx == '&')
		{
			charMode = CHARMODE_TAG;
			str++;
			continue;
		}

		if(charMode == CHARMODE_TAG)
		{
			if (prevChar == '&' && charIdx == '&')
			{
				// escape
				charMode = CHARMODE_NORMAL;
			}
			else if (charIdx == '#')
			{
				parsedParams = stateParams;
				tagType = TEXT_TAG_COLOR;

				str++;
				parsedParams.textColor = ColorRGBA(hexToColor3(str), stateParams.textColor.a);
				str += 6;

				continue;
			}
			else if (charIdx == ';')
			{
				if (tagType == TEXT_TAG_NONE)
				{
					if(states.getCount())
						states.popBack();
				}
				else
				{
					states.append(parsedParams);
				}

				tagType = TEXT_TAG_NONE;
				charMode = CHARMODE_NORMAL;
				prevChar = charIdx;
				str++;
				continue;
			}			
		}

		if(states.getCount() == 0)
		{
			states.append( params ); // restore style
			continue;
		}

		//
		// reset startpos
		//
		if(hasNewLine)
		{
			layoutBuilder->OnNewLine(stateParams, (void*)str, isWideChar, lineNumber, textPos, startPos);

			hasNewLine = false;
		}

		if (charIdx == '\n')	// NEWLINE
		{
			lineNumber++;
			hasNewLine = true;
			str++;

			continue;
		}

		if(!IsVisibleChar(charIdx))
		{
			str++;
			continue;
		}

		float baseLine = GetBaselineOffs(stateParams);

		//
		// Render part - text filling
		//
		FontChar chr;
		GetScaledCharacter( chr, charIdx, stateParams.scale );

		// build default character pos and size
		Vector2D cPos(
			startPos.x + chr.ofsX, 
			startPos.y - baseLine + chr.ofsY);

		Vector2D cSize(
			chr.x1-chr.x0,
			chr.y1-chr.y0);

		if(m_flags.sdf) // only scale SDF characters
			cSize *= m_scale * stateParams.scale;

		//if(stateParams.styleFlag & TEXT_STYLE_FROM_CAP)
		//	cPos.y = startPos.y - (cSize.y-baseLine) + chr.ofsY;

		if(!layoutBuilder->LayoutChar(stateParams, (void*)str, isWideChar, chr, startPos, cPos, cSize))
			break;

		if(stateParams.styleFlag & TEXT_STYLE_FROM_CAP)
			cPos.y = startPos.y - (cSize.y-baseLine) + chr.ofsY;

		AARectangle charRect(cPos, cPos+cSize);
		AARectangle charTexCoord(chr.x0*m_invTexSize.x, chr.y0*m_invTexSize.y,chr.x1*m_invTexSize.x, chr.y1*m_invTexSize.y);

		// set character color
		builder.Color4fv(stateParams.textColor);

		// use meshbuilder's index buffer optimization feature
		builder.TexturedQuad2(	charRect.GetLeftTop(), charRect.GetRightTop(), charRect.GetLeftBottom(), charRect.GetRightBottom(),
								charTexCoord.GetLeftTop(), charTexCoord.GetRightTop(), charTexCoord.GetLeftBottom(), charTexCoord.GetRightBottom());

		str++;
	
    } //while
}

// TODO: really a font parameters!!!
DECLARE_CVAR(r_font_sdf_start, "0.94", nullptr, CV_CHEAT);
DECLARE_CVAR(r_font_sdf_range, "0.06", nullptr, CV_CHEAT);
DECLARE_CVAR(r_font_debug, "0", nullptr, CV_CHEAT);

// renders text (wide char)
void CFont::SetupRenderText(const wchar_t* pszText, const Vector2D& start, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	int vertCount = GetTextQuadsCount(pszText, params) * 6;
	if (vertCount == 0)
		return;

	IDynamicMeshPtr dynMesh = g_matSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	if (r_font_debug.GetBool())
	{
		RenderDrawCmd drawCmd;
		drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

		MatSysDefaultRenderPass defaultRenderPass;
		defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
		RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);

		// set character color
		meshBuilder.Begin(PRIM_LINES);
		meshBuilder.Color4f(1.0f, 0.0f, 0.0f, 0.8f);
		meshBuilder.Line2fv(start, start + IVector2D(512, 0));

		if (meshBuilder.End(drawCmd))
			g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);
	}

	RenderDrawCmd drawCmd;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
	BuildCharVertexBuffer(meshBuilder, pszText, start, params);
	if (meshBuilder.End(drawCmd))
		SetupDrawTextMeshBuffer(drawCmd, params, rendPassRecorder);
}

// renders text (ASCII)
void CFont::SetupRenderText(const char* pszText, const Vector2D& start, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	int vertCount = GetTextQuadsCount(pszText, params) * 6;
	if (vertCount == 0)
		return;

	IDynamicMeshPtr dynMesh = g_matSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	RenderDrawCmd drawCmd;
	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
	BuildCharVertexBuffer(meshBuilder, pszText, start, params);
	if (meshBuilder.End(drawCmd))
		SetupDrawTextMeshBuffer(drawCmd, params, rendPassRecorder);
}

void CFont::SetupDrawTextMeshBuffer(RenderDrawCmd& drawCmd, const FontStyleParam& params, IGPURenderPassRecorder* rendPassRecorder)
{
	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.texture = m_fontTexture;

	RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);

	// TODO: defaultRenderPass.scissor (params.styleFlag & TEXT_STYLE_SCISSOR)

	CEqFontCache* fontCache = ((CEqFontCache*)g_fontCache);

	drawCmd.SetMaterial(fontCache->m_sdfMaterial);
	MatVec3Proxy sdfRange = fontCache->m_fontParams;
	MatVec4Proxy baseColor = fontCache->m_fontBaseColor;
	MatVec4Proxy shadowColor = fontCache->m_shadowColor;
	MatVec3Proxy shadowSdfRange = fontCache->m_shadowParams;
	MatVec2Proxy shadowOffset = fontCache->m_shadowOffset;

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
			const float sdfEndClamped = clamp(r_font_sdf_range.GetFloat() + params.shadowWeight, 0.0f, 1.0f - r_font_sdf_start.GetFloat());
			shadowSdfRange.Set(Vector3D(r_font_sdf_start.GetFloat() - params.shadowWeight, sdfEndClamped, 0.0f));
		}
		else
			shadowSdfRange.Set(Vector3D(0.0f, 1.0f, 0.0f));
	}
	else
		shadowColor.Set(vec4_zero);

	if (m_flags.sdf)
	{
		const float sdfEndClamped = clamp(r_font_sdf_range.GetFloat() + params.textWeight, 0.0f, 1.0f - r_font_sdf_start.GetFloat());
		sdfRange.Set(Vector3D(r_font_sdf_start.GetFloat() - params.textWeight, sdfEndClamped, 1.0f));
	}
	else
		sdfRange.Set(Vector3D(0.0f, 1.0f, 1.0f));

	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);
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
void CFont::GetScaledCharacter( FontChar& chr, const int chrId, const Vector2D& scale) const
{
	chr = GetFontCharById(chrId);

	if(m_flags.sdf) // only scale SDF characters
	{
		chr.advX = chr.advX * scale.x;
		chr.ofsX = chr.ofsX * scale.x;
		chr.ofsY = chr.ofsY * scale.y;
	}
}

//
// returns maximum of possible quads to be allocated
//
template <typename CHAR_T>
int CFont::GetTextQuadsCount(const CHAR_T* str, const FontStyleParam& params) const
{
	int n = 0;

	int charMode = CHARMODE_NORMAL;
	int prevChar = 0;
	int charIdx = 0;

	while (*str)
	{
		prevChar = charIdx;
		charIdx = *str;

		// skip fasttags
		if( (params.styleFlag & TEXT_STYLE_USE_TAGS) &&
			charMode == CHARMODE_NORMAL &&
			charIdx == '&')
		{
			charMode = CHARMODE_TAG;
			str++;
			continue;
		}

		if(charMode == CHARMODE_TAG)
		{
			if(charIdx == ';' || prevChar == '&' && charIdx == '&')
				charMode = CHARMODE_NORMAL;

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

	KeyValues kvs;
	if( !kvs.LoadFromFile( finalFileName ) )
	{
		MsgError("ERROR: Can't open font file '%s' (%s)\n", finalFileName.ToCString(), filenamePrefix);
		return false;
	}

	const KVSection* fontSec = kvs.GetRootSection()->FindSection("Font", KV_FLAG_SECTION);
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

	fontSec = kvs.GetRootSection()->FindSection("eqFont");
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

		for (const KVSection* k : fontSec->keys)
		{
			if (k->values.numElem() < 7)
				continue;

			const int charIdx = atoi(k->name);

			// x y w h ox oy advanceX
			// 0 1 2 3 4  5  6

			FontChar& fontChar = m_charMap[charIdx];
			fontChar.x0 = KV_GetValueFloat(k, 0);
			fontChar.y0 = KV_GetValueFloat(k, 1);

			fontChar.x1 = fontChar.x0 + KV_GetValueFloat(k, 2);
			fontChar.y1 = fontChar.y0 + KV_GetValueFloat(k, 3);

			fontChar.ofsX = KV_GetValueFloat(k, 4) * m_scale.x;
			fontChar.ofsY = KV_GetValueFloat(k, 5) * m_scale.y;
			fontChar.advX = KV_GetValueFloat(k, 6) * m_scale.x;
		}

		return true;
	}

	MsgError("ERROR: '%s' is not a valid font file\n", finalFileName.ToCString());

	return false;
}