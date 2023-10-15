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

bool IsVisibleChar( int ch )
{
	return	(ch != '\n') && 
			(ch != '\r');
}

//-----------------------------------------------------------------------------------------

class CPlainTextLayoutBuilder : public ITextLayoutBuilder
{
public:
	// controls the newline. For different text orientations
	void	OnNewLine(	const eqFontStyleParam_t& params, 
						void* strCurPos, bool isWideChar,
						int lineNumber,
						const Vector2D& textStart,
						Vector2D& curTextPos );

	// for special layouts like rectangles
	// if false then stops output, and don't render this char
	bool	LayoutChar( const eqFontStyleParam_t& params,
						void* strCurPos, bool isWideChar,
						const eqFontChar_t& chr,
						Vector2D& curTextPos,
						Vector2D& cPos, Vector2D& cSize );
};

void CPlainTextLayoutBuilder::OnNewLine(const eqFontStyleParam_t& params, 
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

bool CPlainTextLayoutBuilder::LayoutChar(	const eqFontStyleParam_t& params,
											void* strCurPos, bool isWideChar,
											const eqFontChar_t& chr,
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
	m_textColor = color_white;
	m_spacing = 0.0f;
	m_scale = 1.0f;

	m_baseline = 0.0f;
	m_lineHeight = 0.0f;

	memset(&m_flags, 1, sizeof(m_flags));
}

CFont::~CFont()
{
	//if( m_vertexBuffer )
	//	free( m_vertexBuffer );
}

const char*	CFont::GetName() const
{
	return m_name.ToCString();
}

float CFont::GetStringWidth( const char* str, const eqFontStyleParam_t& params, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = strlen(str);

	return _GetStringWidth(str, params, charCount, breakOnChar);
}

float CFont::GetStringWidth( const wchar_t* str, const eqFontStyleParam_t& params, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = wcslen(str);

	return _GetStringWidth(str, params, charCount, breakOnChar);
}

float CFont::GetLineHeight( const eqFontStyleParam_t& params ) const
{
	if(m_flags.sdf) // only scale SDF characters
		return m_lineHeight * params.scale.y;

	return m_lineHeight;
}

float CFont::GetBaselineOffs( const eqFontStyleParam_t& params ) const
{
	if(m_flags.sdf) // only scale SDF characters
		return m_baseline * params.scale.y;

	return m_baseline;
}

template <typename CHAR_T>
float CFont::_GetStringWidth( const CHAR_T* str, const eqFontStyleParam_t& params, int charCount, int breakOnChar) const
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

		eqFontChar_t chr;
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
void CFont::BuildCharVertexBuffer(CMeshBuilder& builder, const CHAR_T* str, const Vector2D& textPos, const eqFontStyleParam_t& params)
{
	const bool isWideChar = std::is_same<CHAR_T,wchar_t>::value;

	ITextLayoutBuilder* layoutBuilder = &s_defaultTextLayout;

	if(params.layoutBuilder)
		layoutBuilder = params.layoutBuilder;

	layoutBuilder->Reset( this );

	Vector2D startPos = textPos;

	bool hasNewLine = true;
	int lineNumber = 0;

	FixedList<eqFontStyleParam_t, 8> states;
	states.append( params );	// push this param

	int charMode = CHARMODE_NORMAL;
	int tagType = TEXT_TAG_NONE;
	int prevChar = 0;
	int charIdx = 0;

	eqFontStyleParam_t parsedParams;

    while( *str )
	{
		prevChar = charIdx;
		charIdx = *str;

		const eqFontStyleParam_t& stateParams = states.back();

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
				parsedParams.textColor = ColorRGBA(hexToColor3(str), stateParams.textColor.w);
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
		eqFontChar_t chr;
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

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const wchar_t* pszText, const Vector2D& start, const eqFontStyleParam_t& params)
{
	int vertCount = GetTextQuadsCount(pszText, params) * 6;
	if (vertCount == 0)
		return;

	IDynamicMesh* dynMesh = g_matSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	if (r_font_debug.GetBool())
	{
		RasterizerStateParams raster;
		raster.scissor = (params.styleFlag & TEXT_STYLE_SCISSOR);
		BlendStateParams blending;
		blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
		blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

		g_matSystem->SetDepthStates(false, false);
		g_matSystem->SetBlendingStates(blending);
		g_matSystem->SetRasterizerStates(raster);

		g_matSystem->SetAmbientColor(color_white);
		g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

		g_matSystem->FindGlobalMaterialVar<MatTextureProxy>(StringToHashConst("basetexture")).Set(nullptr);
		g_matSystem->BindMaterial(g_matSystem->GetDefaultMaterial());

		// set character color
		meshBuilder.Color4f(1.0f, 0.0f, 0.0f, 0.8f);

		meshBuilder.Begin(PRIM_LINES);

		meshBuilder.Line2fv(start, start + IVector2D(512, 0));

		meshBuilder.End();
	}

	// first we building vertex buffer
	meshBuilder.Begin( PRIM_TRIANGLE_STRIP );
	BuildCharVertexBuffer(meshBuilder, pszText, start, params);
	meshBuilder.End(false);

	DrawTextMeshBuffer(dynMesh, params);
}

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const char* pszText, const Vector2D& start, const eqFontStyleParam_t& params)
{
	int vertCount = GetTextQuadsCount(pszText, params) * 6;
	if (vertCount == 0)
		return;

	IDynamicMesh* dynMesh = g_matSystem->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	// first we building vertex buffer
	meshBuilder.Begin( PRIM_TRIANGLE_STRIP );
	BuildCharVertexBuffer(meshBuilder, pszText, start, params);
	meshBuilder.End(false);

	DrawTextMeshBuffer(dynMesh, params);
}

void CFont::DrawTextMeshBuffer(IDynamicMesh* mesh, const eqFontStyleParam_t& params)
{
	RasterizerStateParams raster;
	raster.scissor = (params.styleFlag & TEXT_STYLE_SCISSOR);
	BlendStateParams blending;

	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_matSystem->SetDepthStates(false,false);
	g_matSystem->SetBlendingStates(blending);
	g_matSystem->SetRasterizerStates(raster);

	g_matSystem->FindGlobalMaterialVar<MatTextureProxy>(StringToHashConst("basetexture")).Set(m_fontTexture);

	CEqFontCache* fontCache = ((CEqFontCache*)g_fontCache);

	IMaterial* fontMaterial = fontCache->m_sdfMaterial;//m_flags.sdf ? fontCache->m_sdfMaterial : materials->GetDefaultMaterial();

	MatVec3Proxy sdfRange = fontCache->m_fontParams;

	// draw shadow
	// TODO: shadow color should be separate from text vertices color!!!
	if ((params.styleFlag & TEXT_STYLE_SHADOW) && params.shadowAlpha > 0.0f)
	{
		g_matSystem->SetMatrix(MATRIXMODE_WORLD, translate(params.shadowOffset,params.shadowOffset,0.0f));
		g_matSystem->SetAmbientColor(ColorRGBA(params.shadowColor,params.shadowAlpha));

		if (m_flags.sdf)
		{
			// shadow width
			float sdfEndClamped = clamp(r_font_sdf_range.GetFloat() + params.shadowWeight, 0.0f, 1.0f - r_font_sdf_start.GetFloat());
			sdfRange.Set(Vector3D(r_font_sdf_start.GetFloat() - params.shadowWeight, sdfEndClamped, 0.0f));
		}
		else
			sdfRange.Set(Vector3D(0.0f, 1.0f, 0.0f));

		g_matSystem->BindMaterial(fontMaterial);

		mesh->Render();
	}

	if (m_flags.sdf)
	{
		float sdfEndClamped = clamp(r_font_sdf_range.GetFloat() + params.textWeight, 0.0f, 1.0f - r_font_sdf_start.GetFloat());
		sdfRange.Set(Vector3D(r_font_sdf_start.GetFloat() - params.textWeight, sdfEndClamped, 1.0f));
	}
	else
		sdfRange.Set(Vector3D(0.0f, 1.0f, 1.0f));

	g_matSystem->SetAmbientColor(color_white);
	g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

	g_matSystem->BindMaterial(fontMaterial);

	mesh->Render();
}

//
// returns font character information
//
const eqFontChar_t&	CFont::GetFontCharById( const int chrId ) const
{
	static eqFontChar_t null_default;

	auto it = m_charMap.find(chrId);

	if(it.atEnd())
		return null_default;
	
	return *it;
}

//
// returns the scaled character
//
void CFont::GetScaledCharacter( eqFontChar_t& chr, const int chrId, const Vector2D& scale) const
{
	chr = GetFontCharById(chrId);

	if(m_flags.sdf) // only scale SDF characters
	{
		chr.advX = chr.advX*scale.x;
		chr.ofsX = chr.ofsX*scale.x;
		chr.ofsY = chr.ofsY*scale.y;
	}
}

//
// returns maximum of possible quads to be allocated
//
template <typename CHAR_T>
int CFont::GetTextQuadsCount(const CHAR_T* str, const eqFontStyleParam_t& params) const
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

	EqString finalFileName = _Es(FONT_DEFAULT_PATH) + m_name + _Es(".fnt");

	KeyValues pKV;
	if( pKV.LoadFromFile( finalFileName.GetData() ) )
	{
		KVSection* fontSec = pKV.GetRootSection()->FindSection("Font", KV_FLAG_SECTION);

		if(!fontSec)	// load new format
		{
			fontSec = pKV.GetRootSection()->FindSection("eqfont");

			if(!fontSec)
			{
				MsgError("ERROR: '%s' not a font file!\n",finalFileName.GetData());
				return false;
			}

			m_flags.sdf = KV_GetValueBool( fontSec->FindSection("isSDF") );
			m_flags.bold = KV_GetValueBool( fontSec->FindSection("bold") );

			bool filter_font = KV_GetValueBool( fontSec->FindSection("filter") ) || m_flags.sdf;

			m_spacing = 0.0f;
			{
				SamplerStateParams samplerParams;
				SamplerStateParams_Make(samplerParams, g_renderAPI->GetCaps(), filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST, TEXADDRESS_WRAP, TEXADDRESS_WRAP, TEXADDRESS_WRAP);

				m_fontTexture = g_texLoader->LoadTextureFromFileSync(KV_GetValueString(fontSec->FindSection("texture")), samplerParams, TEXFLAG_NOQUALITYLOD);
			}

			if(m_flags.sdf)
				m_scale = KV_GetVector2D( fontSec->FindSection("scale") );

			m_baseline = KV_GetValueFloat( fontSec->FindSection("baseline") ) * m_scale.y;
			m_lineHeight = KV_GetValueFloat( fontSec->FindSection("lineheight") ) * m_scale.y;

			m_invTexSize = 1.0f;
			
			if(m_fontTexture != nullptr)
			{
				m_invTexSize = 1.0f / Vector2D(m_fontTexture->GetWidth(), m_fontTexture->GetHeight());
			}

			for(int i = 0; i < fontSec->keys.numElem(); i++)
			{
				KVSection* k = fontSec->keys[i];

				if(k->values.numElem() < 7)
					continue;

				const int charIdx = atoi(k->name);

				eqFontChar_t& fontChar = m_charMap[charIdx];

				// x y w h ox oy advanceX
				// 0 1 2 3 4  5  6

				fontChar.x0 = KV_GetValueFloat(k, 0);
				fontChar.y0 = KV_GetValueFloat(k, 1);

				fontChar.x1 = fontChar.x0 + KV_GetValueFloat(k, 2);
				fontChar.y1 = fontChar.y0 + KV_GetValueFloat(k, 3);

				fontChar.ofsX = KV_GetValueFloat(k, 4)*m_scale.x;
				fontChar.ofsY = KV_GetValueFloat(k, 5)*m_scale.y;
				fontChar.advX = KV_GetValueFloat(k, 6)*m_scale.x;
				
				if( g_renderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9 )
				{
					// fix half texel on DX9
					fontChar.x0 = fontChar.x0 - 0.5f;
					fontChar.y0 = fontChar.y0 - 0.5f;
					fontChar.x1 = fontChar.x1 - 0.5f;
					fontChar.y1 = fontChar.y1 - 0.5f;
				}
			}
		}
		else
		{
			EqString texname = KV_GetValueString( fontSec->FindSection("Texture"), 0, filenamePrefix );

			bool isAutogeneratedFont = false;

			bool filter_font = KV_GetValueBool( fontSec->FindSection("FilterFontTexture") );

			float fSpacing = KV_GetValueFloat( fontSec->FindSection("spacing") );

			// the monospace fonts for debug overlays and other purposes
			KVSection* pFontSizeSection = fontSec->FindSection("FontSize", KV_FLAG_SECTION);
			if(pFontSizeSection)
			{
				isAutogeneratedFont = true;

				m_spacing = fSpacing;

				int wide = KV_GetValueInt(pFontSizeSection->FindSection("width"), 0, 8);
				int tall = KV_GetValueInt(pFontSizeSection->FindSection("height"), 0, 8);

				m_baseline = tall;
				m_lineHeight = tall+4;

				int charsperline =  KV_GetValueInt(pFontSizeSection->FindSection("charsperline"), 0, 16);

				float interval = KV_GetValueFloat(pFontSizeSection->FindSection("interval"), 0, 0.75);

				{
					SamplerStateParams samplerParams;
					SamplerStateParams_Make(samplerParams, g_renderAPI->GetCaps(), filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST, TEXADDRESS_WRAP, TEXADDRESS_WRAP, TEXADDRESS_WRAP);

					m_fontTexture = g_texLoader->LoadTextureFromFileSync(texname, samplerParams, TEXFLAG_NOQUALITYLOD);
				}

				if(m_fontTexture == nullptr)
				{
					MsgError("ERROR: no texture for font '%s'\n",finalFileName.GetData());
					return false;
				}

				m_invTexSize = Vector2D(1.0f/m_fontTexture->GetWidth(), 1.0f/m_fontTexture->GetHeight());

				int line = 0;
				int lChars = 0;
				for(int i = 0; i < 256; i++)
				{
					if(lChars == charsperline)
					{
						line++;
						lChars = 0;
					}

					eqFontChar_t& chr = m_charMap[i];

					float CurCharPos_x = lChars * tall;
					float CurCharPos_y = line * tall;

					chr.x0 = CurCharPos_x + interval; // LineCharCount * interval
					chr.x1 = CurCharPos_x + interval + wide;
					chr.y0 = CurCharPos_y + interval; // Line Count * height
					chr.y1 = CurCharPos_y + interval + tall;

					chr.advX = (chr.x1 - chr.x0) + fSpacing;
					chr.ofsX = 0;
					chr.ofsY = 0;

					lChars++;
				}

			}
		}

		return true;
	}
	else
	{
		MsgError("ERROR: Can't open font file '%s' (%s)\n", finalFileName.ToCString(), filenamePrefix);
	}

	return false;
}