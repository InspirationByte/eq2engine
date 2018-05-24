//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//////////////////////////////////////////////////////////////////////////////////

/*
TODO:
		- Text rendering shaders and effects
*/

#include "DebugInterface.h"
#include "math/Rectangle.h"
#include "materialsystem/IMaterialSystem.h"
#include "materialsystem/MeshBuilder.h"
#include "Font.h"
#include "utils/DkList.h"
#include "utils/strtools.h"
#include "utils/DkLinkedList.h"
#include "FontCache.h"
#include <stdlib.h>

#include "ConVar.h"

#pragma todo("Rework font system - Add generator, better rotation support, effects, and text alignment/bounding")

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
	CFont* font = (CFont*)m_font;

	if(params.styleFlag & TEXT_STYLE_MONOSPACE)
		curTextPos.x += cSize.x+font->m_spacing;
	else
		curTextPos.x += chr.advX + font->m_spacing;

	return true;
}

static CPlainTextLayoutBuilder s_defaultTextLayout;

//-----------------------------------------------------------------------------------------
CFont::CFont()
{
	m_fontTexture = NULL;

	//m_vertexBuffer = NULL;
	//m_numVerts = 0;

	m_textColor = color4_white;
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
	return m_name.c_str();
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

    for(int i = 0; i < charCount; i++)
	{
		int charIdx = str[i];

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
			if(charIdx == ';')
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
			totalWidth += chr.x1-chr.x0;
		else
			totalWidth += chr.advX; // chr.x1-chr.x0;
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

	if(params.layoutBuilder != NULL)
		layoutBuilder = params.layoutBuilder;

	layoutBuilder->Reset( this );

	Vector2D startPos = textPos;

	bool hasNewLine = true;
	int lineNumber = 0;

	DkLinkedList<eqFontStyleParam_t> states;
	states.addLast( params );	// push this param

	int charMode = CHARMODE_NORMAL;
	int tagType = TEXT_TAG_NONE;

	eqFontStyleParam_t parsedParams;

    while( *str )
	{
		int charIdx = *str;

		states.goToLast();
		const eqFontStyleParam_t& stateParams = states.getCurrent();

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
			if( charIdx == '#')
			{
				parsedParams = stateParams;
				tagType = TEXT_TAG_COLOR;

				str++;

				// parse color string
				char hexcolor[6];
				for(int i = 0; i < 6 && *str; i++)
					hexcolor[i] = *str++;

				// This looks weird
				char* pend;
				ubyte color[3];

				char r[3] = {hexcolor[0], hexcolor[1], 0};
				char g[3] = {hexcolor[2], hexcolor[3], 0};
				char b[3] = {hexcolor[4], hexcolor[5], 0};

				color[0] = strtol(r, &pend, 16);
				color[1] = strtol(g, &pend, 16);
				color[2] = strtol(b, &pend, 16);

				parsedParams.textColor = ColorRGBA((float)color[0]/255.0f,(float)color[1]/255.0f,(float)color[2]/255.0f,stateParams.textColor.w);

				continue;
			}
			else if( charIdx == ';')
			{
				if(tagType == TEXT_TAG_NONE)
				{
					states.goToLast();
					states.removeCurrent();
					states.goToLast();
				}
				else
					states.addLast( parsedParams );

				tagType = TEXT_TAG_NONE;
				charMode = CHARMODE_NORMAL;
				str++;
			}

			continue;
		}

		if(states.getCount() == 0)
		{
			states.addLast( params ); // restore style
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

		Rectangle_t charRect(cPos, cPos+cSize);
		Rectangle_t charTexCoord(chr.x0*m_invTexSize.x, chr.y0*m_invTexSize.y,chr.x1*m_invTexSize.x, chr.y1*m_invTexSize.y);

		// set character color
		builder.Color4fv(stateParams.textColor);

		// use meshbuilder's index buffer optimization feature
		builder.TexturedQuad2(	charRect.GetLeftTop(), charRect.GetRightTop(), charRect.GetLeftBottom(), charRect.GetRightBottom(),
								charTexCoord.GetLeftTop(), charTexCoord.GetRightTop(), charTexCoord.GetLeftBottom(), charTexCoord.GetRightBottom());

		str++;
	
    } //while
}

ConVar r_font_sdf_start("r_font_sdf_start", "0.94");
ConVar r_font_sdf_range("r_font_sdf_range", "0.06");

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const wchar_t* pszText, const Vector2D& start, const eqFontStyleParam_t& params)
{
	int vertCount = GetTextQuadsCount(pszText, params) * 6;
	if (vertCount == 0)
		return;

	IDynamicMesh* dynMesh = materials->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	// first we building vertex buffer
	meshBuilder.Begin( PRIM_TRIANGLE_STRIP );
	BuildCharVertexBuffer(meshBuilder, pszText, start, params);
	meshBuilder.End(false);

	//
	// render
	//
	RasterizerStateParams_t raster;
	raster.scissor = (params.styleFlag & TEXT_STYLE_SCISSOR) > 0;
	BlendStateParam_t blending;

	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->SetDepthStates(false,false);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(raster);

	g_pShaderAPI->SetTexture(m_fontTexture, NULL, 0);

	CEqFontCache* fontCache = ((CEqFontCache*)g_fontCache);

	IMaterial* fontMaterial = m_flags.sdf ? fontCache->m_sdfMaterial : materials->GetDefaultMaterial();

	IMatVar* sdfRange = fontCache->m_sdfRange;

	// draw shadow
	if(params.styleFlag & TEXT_STYLE_SHADOW)
	{
		materials->SetMatrix(MATRIXMODE_WORLD, translate(params.shadowOffset,params.shadowOffset,0.0f));
		materials->SetAmbientColor(ColorRGBA(0,0,0,params.shadowAlpha));

		// shadow width
		float sdfEndClamped = clamp(r_font_sdf_range.GetFloat()+params.shadowWidth, 0.0f, 1.0f - r_font_sdf_start.GetFloat());
		sdfRange->SetVector2(Vector2D(r_font_sdf_start.GetFloat()-params.shadowWidth, sdfEndClamped));

		materials->BindMaterial(fontMaterial);

		dynMesh->Render();
	}

	float sdfEndClamped = clamp(r_font_sdf_range.GetFloat(), 0.0f, 1.0f - r_font_sdf_start.GetFloat());
	sdfRange->SetVector2(Vector2D(r_font_sdf_start.GetFloat(), sdfEndClamped));

	materials->SetAmbientColor(color4_white);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->BindMaterial(fontMaterial);

	dynMesh->Render();
}

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const char* pszText, const Vector2D& start, const eqFontStyleParam_t& params)
{
	int vertCount = GetTextQuadsCount(pszText, params) * 6;
	if (vertCount == 0)
		return;

	IDynamicMesh* dynMesh = materials->GetDynamicMesh();
	CMeshBuilder meshBuilder(dynMesh);

	// first we building vertex buffer
	meshBuilder.Begin( PRIM_TRIANGLE_STRIP );
	BuildCharVertexBuffer(meshBuilder, pszText, start, params);
	meshBuilder.End(false);

	//
	// render
	//
	RasterizerStateParams_t raster;
	BlendStateParam_t blending;

	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->SetDepthStates(false,false);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(raster);

	g_pShaderAPI->SetTexture(m_fontTexture, NULL, 0);

	CEqFontCache* fontCache = ((CEqFontCache*)g_fontCache);

	IMaterial* fontMaterial = m_flags.sdf ? fontCache->m_sdfMaterial : materials->GetDefaultMaterial();
	
	IMatVar* sdfRange = fontCache->m_sdfRange;

	// draw shadow
	if(params.styleFlag & TEXT_STYLE_SHADOW)
	{
		materials->SetMatrix(MATRIXMODE_WORLD, translate(params.shadowOffset,params.shadowOffset,0.0f));
		materials->SetAmbientColor(ColorRGBA(0,0,0,params.shadowAlpha));

		// shadow width
		float sdfEndClamped = clamp(r_font_sdf_range.GetFloat()+params.shadowWidth, 0.0f, 1.0f - r_font_sdf_start.GetFloat());
		sdfRange->SetVector2(Vector2D(r_font_sdf_start.GetFloat()-params.shadowWidth, sdfEndClamped));

		materials->BindMaterial(fontMaterial);

		dynMesh->Render();
	}

	float sdfEndClamped = clamp(r_font_sdf_range.GetFloat(), 0.0f, 1.0f - r_font_sdf_start.GetFloat());
	sdfRange->SetVector2(Vector2D(r_font_sdf_start.GetFloat(), sdfEndClamped));

	materials->SetAmbientColor(color4_white);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->BindMaterial(fontMaterial);

	dynMesh->Render();
}

//
// returns font character information
//
const eqFontChar_t&	CFont::GetFontCharById( const int chrId ) const
{
	static eqFontChar_t null_default;

	if(m_charMap.count(chrId) == 0)
		return null_default;

	return m_charMap.at(chrId);
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
	while (*str)
	{
		int charIdx = *str;

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
			if(charIdx == ';')
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
		kvkeybase_t* fontSec = pKV.GetRootSection()->FindKeyBase("Font", KV_FLAG_SECTION);

		if(!fontSec)	// load new format
		{
			fontSec = pKV.GetRootSection()->FindKeyBase("eqfont");

			if(!fontSec)
			{
				MsgError("ERROR: '%s' not a font file!\n",finalFileName.GetData());
				return false;
			}

			m_flags.sdf = KV_GetValueBool( fontSec->FindKeyBase("isSDF") );
			m_flags.bold = KV_GetValueBool( fontSec->FindKeyBase("bold") );

			bool filter_font = KV_GetValueBool( fontSec->FindKeyBase("filter") ) || m_flags.sdf;

			m_spacing = 0.0f;
			m_fontTexture = g_pShaderAPI->LoadTexture(KV_GetValueString(fontSec->FindKeyBase("texture")), filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,TEXADDRESS_WRAP, TEXFLAG_NOQUALITYLOD);

			if(m_flags.sdf)
				m_scale = KV_GetVector2D( fontSec->FindKeyBase("scale") );

			m_baseline = KV_GetValueFloat( fontSec->FindKeyBase("baseline") ) * m_scale.y;
			m_lineHeight = KV_GetValueFloat( fontSec->FindKeyBase("lineheight") ) * m_scale.y;

			m_invTexSize = 1.0f;
			
			if(m_fontTexture != NULL)
			{
				m_fontTexture->Ref_Grab();

				m_invTexSize = Vector2D(1.0f/m_fontTexture->GetWidth(), 1.0f/m_fontTexture->GetHeight());
			}

			for(int i = 0; i < fontSec->keys.numElem(); i++)
			{
				kvkeybase_t* k = fontSec->keys[i];

				if(k->values.numElem() < 7)
					continue;

				int charIdx = atoi(k->name);

				eqFontChar_t fontChar;

				// x y w h ox oy advanceX
				// 0 1 2 3 4  5  6

				fontChar.x0 = KV_GetValueFloat(k, 0);
				fontChar.y0 = KV_GetValueFloat(k, 1);

				fontChar.x1 = fontChar.x0 + KV_GetValueFloat(k, 2);
				fontChar.y1 = fontChar.y0 + KV_GetValueFloat(k, 3);

				fontChar.ofsX = KV_GetValueFloat(k, 4)*m_scale.x;
				fontChar.ofsY = KV_GetValueFloat(k, 5)*m_scale.y;
				fontChar.advX = KV_GetValueFloat(k, 6)*m_scale.x;
				
				if( g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9 )
				{
					// fix half texel on DX9
					fontChar.x0 = fontChar.x0 - 0.5f;
					fontChar.y0 = fontChar.y0 - 0.5f;
					fontChar.x1 = fontChar.x1 - 0.5f;
					fontChar.y1 = fontChar.y1 - 0.5f;
				}

				m_charMap[charIdx] = fontChar;
			}
		}
		else
		{
			EqString texname = KV_GetValueString( fontSec->FindKeyBase("Texture"), 0, filenamePrefix );

			bool isAutogeneratedFont = false;

			bool filter_font = KV_GetValueBool( fontSec->FindKeyBase("FilterFontTexture") );

			float fSpacing = KV_GetValueFloat( fontSec->FindKeyBase("spacing") );

			// the monospace fonts for debug overlays and other purposes
			kvkeybase_t* pFontSizeSection = fontSec->FindKeyBase("FontSize", KV_FLAG_SECTION);
			if(pFontSizeSection)
			{
				isAutogeneratedFont = true;

				m_spacing = fSpacing;

				int wide = KV_GetValueInt(pFontSizeSection->FindKeyBase("width"), 0, 8);
				int tall = KV_GetValueInt(pFontSizeSection->FindKeyBase("height"), 0, 8);

				m_baseline = tall;
				m_lineHeight = tall+4;

				int charsperline =  KV_GetValueInt(pFontSizeSection->FindKeyBase("charsperline"), 0, 16);

				float interval = KV_GetValueFloat(pFontSizeSection->FindKeyBase("interval"), 0, 0.75);

				m_fontTexture = g_pShaderAPI->LoadTexture(texname.GetData(),filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,TEXADDRESS_WRAP, TEXFLAG_NOQUALITYLOD);

				if(m_fontTexture == NULL)
				{
					MsgError("ERROR: no texture for font '%s'\n",finalFileName.GetData());
					return false;
				}

				m_fontTexture->Ref_Grab();
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

					m_charMap[i] = eqFontChar_t();

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
		MsgError("ERROR: Can't open font file '%s' (%s)\n", finalFileName.c_str(), filenamePrefix);
	}

	return false;
}