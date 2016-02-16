//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts
//////////////////////////////////////////////////////////////////////////////////

/*
TODO:
		- New font system, font file format
		- Support utf16/wchar_t translated strings
		- Make in-engine font generator
		- Text rendering services
		- Text rendering shaders and effects
*/

#include "DebugInterface.h"
#include "math/Rectangle.h"
#include "materialsystem/IMaterialSystem.h"
#include "Font.h"
#include "utils/DkList.h"
#include "utils/strtools.h"

#include <stdlib.h>

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
		curTextPos.y += m_font->GetLineHeight();

	curTextPos.x = textStart.x;

	float newlineStringWidth;
	
	if(isWideChar)
		newlineStringWidth = m_font->GetStringWidth( (wchar_t*)strCurPos, params.styleFlag, -1, '\n' );
	else
		newlineStringWidth = m_font->GetStringWidth( (char*)strCurPos, params.styleFlag, -1, '\n' );

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
	m_vertexBuffer = NULL;
	m_fontTexture = NULL;
	m_numVerts = 0;

	m_textColor = color4_white;
	m_spacing = 0.0f;

	m_baseline = 0.0f;
	m_lineHeight = 0.0f;
	m_scale = Vector2D(1.0f);
}

CFont::~CFont()
{
	if( m_vertexBuffer )
		free( m_vertexBuffer );
}

const char*	CFont::GetName() const
{
	return m_name.c_str();
}

float CFont::GetStringWidth( const char* str, int fontStyleFlags, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = strlen(str);

	return _GetStringWidth(str, fontStyleFlags, charCount, breakOnChar);
}

float CFont::GetStringWidth( const wchar_t* str, int fontStyleFlags, int charCount, int breakOnChar) const
{
    if (charCount < 0)
		charCount = wcslen(str);

	return _GetStringWidth(str, fontStyleFlags, charCount, breakOnChar);
}

template <typename CHAR_T>
float CFont::_GetStringWidth( const CHAR_T* str, int styleFlags, int charCount, int breakOnChar) const
{
    float totalWidth = 0.0f;

	// parse
	int charMode = CHARMODE_NORMAL;

    for(int i = 0; i < charCount; i++)
	{
		int charIdx = str[i];

		// skip fasttags
		if( (styleFlags & TEXT_STYLE_USE_TAGS) &&
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

		const eqFontChar_t& chr = GetFontCharById( charIdx );

		if( styleFlags & TEXT_STYLE_MONOSPACE)
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
int CFont::BuildCharVertexBuffer(Vertex2D_t *dest, const CHAR_T* str, const Vector2D& textPos, const eqFontStyleParam_t& params)
{
	const bool isWideChar = std::is_same<CHAR_T,wchar_t>::value;

	ITextLayoutBuilder* layoutBuilder = &s_defaultTextLayout;

	if(params.layoutBuilder != NULL)
		layoutBuilder = params.layoutBuilder;

	layoutBuilder->Reset( this );

	int numVertsToDraw = 0;

	Vector2D startPos = textPos;

	bool hasNewLine = true;
	int lineNumber = 0;

	DkLinkedList<eqFontStyleParam_t> states;
	states.addLast( params );	// push this param

	eqFontStyleParam_t parsedParams = params;

	int charMode = CHARMODE_NORMAL;
	int tagType = TEXT_TAG_NONE;

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

		//
		// Render part - text filling
		//
		const eqFontChar_t& chr = GetFontCharById(charIdx);

		// build default character pos and size
		Vector2D cPos(
			startPos.x + chr.ofsX, 
			startPos.y - m_baseline + chr.ofsY);

		Vector2D cSize(
			chr.x1-chr.x0,
			chr.y1-chr.y0);

		//if(stateParams.styleFlag & TEXT_STYLE_FROM_CAP)
		//	cPos.y = startPos.y - (cSize.y-m_baseline) + chr.ofsY;

		if(!layoutBuilder->LayoutChar(stateParams, (void*)str, isWideChar, chr, startPos, cPos, cSize))
			break;

		if(stateParams.styleFlag & TEXT_STYLE_FROM_CAP)
			cPos.y = startPos.y - (cSize.y-m_baseline) + chr.ofsY;

		dest[0].m_vPosition = Vector2D(cPos.x, cPos.y);
		dest[0].m_vTexCoord = Vector2D(chr.x0, chr.y0)*m_invTexSize;
		dest[1].m_vPosition = Vector2D(cPos.x + cSize.x, cPos.y);
		dest[1].m_vTexCoord = Vector2D(chr.x1, chr.y0)*m_invTexSize;
		dest[2].m_vPosition = Vector2D(cPos.x, cPos.y + cSize.y);
		dest[2].m_vTexCoord = Vector2D(chr.x0, chr.y1)*m_invTexSize;

		dest[3].m_vPosition = Vector2D(cPos.x, cPos.y + cSize.y);
		dest[3].m_vTexCoord = Vector2D(chr.x0, chr.y1)*m_invTexSize;
		dest[4].m_vPosition = Vector2D(cPos.x + cSize.x, cPos.y);
		dest[4].m_vTexCoord = Vector2D(chr.x1, chr.y0)*m_invTexSize;
		dest[5].m_vPosition = Vector2D(cPos.x + cSize.x, cPos.y + cSize.y);
		dest[5].m_vTexCoord = Vector2D(chr.x1, chr.y1)*m_invTexSize;

		dest[0].m_vColor = stateParams.textColor;
		dest[1].m_vColor = stateParams.textColor;
		dest[2].m_vColor = stateParams.textColor;
		dest[3].m_vColor = stateParams.textColor;
		dest[4].m_vColor = stateParams.textColor;
		dest[5].m_vColor = stateParams.textColor;

		numVertsToDraw	+= 6;
		dest			+= 6;

		str++;
	
    } //while

	return numVertsToDraw;
}

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const wchar_t* pszText, const Vector2D& start, const eqFontStyleParam_t& params)
{
	int vertCount = GetTextQuadsCount(pszText, params.styleFlag) * 6;
	if (vertCount == 0)
		return;

	// realloc buffer if needed
	if (vertCount > m_numVerts)
	{
		Vertex2D_t* newBuf = (Vertex2D_t *) realloc(m_vertexBuffer, vertCount * sizeof(Vertex2D_t));

		if(newBuf)
		{
			m_vertexBuffer = newBuf;
			m_numVerts = vertCount;
		}
	}

	// first we building vertex buffer
	int numVertsToDraw = BuildCharVertexBuffer(m_vertexBuffer, pszText, start, params);

	ASSERTMSG(numVertsToDraw <= m_numVerts, varargs("Font:RenderText error: numVertsToDraw > m_numVerts (%d > %d)", numVertsToDraw, m_numVerts));

	//
	// FFP render part
	//
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	
	// draw shadow
	if(params.styleFlag & TEXT_STYLE_SHADOW)
	{
		materials->SetMatrix(MATRIXMODE_WORLD, translate(params.shadowOffset,params.shadowOffset,0.0f));
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, ColorRGBA(0,0,0,params.shadowAlpha), &blending);
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, color4_white, &blending);
}

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const char* pszText, const Vector2D& start, const eqFontStyleParam_t& params)
{
	int vertCount = GetTextQuadsCount(pszText, params.styleFlag) * 6;
	if (vertCount == 0)
		return;

	// realloc buffer if needed
	if (vertCount > m_numVerts)
	{
		Vertex2D_t* newBuf = (Vertex2D_t *) realloc(m_vertexBuffer, vertCount * sizeof(Vertex2D_t));

		if(newBuf)
		{
			m_vertexBuffer = newBuf;
			m_numVerts = vertCount;
		}
	}

	// first we building vertex buffer
	int numVertsToDraw = BuildCharVertexBuffer(m_vertexBuffer, pszText, start, params);

	ASSERTMSG(numVertsToDraw <= m_numVerts, varargs("Font:RenderText error: numVertsToDraw > m_numVerts (%d > %d)", numVertsToDraw, m_numVerts));

	//
	// FFP render part
	//
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	
	// draw shadow
	if(params.styleFlag & TEXT_STYLE_SHADOW)
	{
		materials->SetMatrix(MATRIXMODE_WORLD, translate(params.shadowOffset,params.shadowOffset,0.0f));
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, ColorRGBA(0,0,0,params.shadowAlpha), &blending);
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, color4_white, &blending);
}

//
// returns font character information
//
const eqFontChar_t& CFont::GetFontCharById( const int chrId ) const
{
	static eqFontChar_t null_default;

	if(m_charMap.count(chrId) == 0)
		return null_default;
	
	// Presonally, I hate exceptions bullshit
	return m_charMap.at(chrId);
}

//
// returns maximum of possible quads to be allocated
//
template <typename CHAR_T>
int CFont::GetTextQuadsCount(const CHAR_T* str, int styleFlags) const
{
	int n = 0;

	int charMode = CHARMODE_NORMAL;
	while (*str)
	{
		int charIdx = *str;

		// skip fasttags
		if( (styleFlags & TEXT_STYLE_USE_TAGS) &&
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

/*
//------------------------------------------------------------------------------
//
// LEGACY - fills ASCII text within rectangle
//
int CFont::RectangularFillTextBuffer(Vertex2D_t *dest, const char *str, float x, float y, float charWidth, float charHeight, Rectangle_t &rect, int* nLinesCount, bool enableWidthRatio, float fOffset)
{
	float startx = x;

	if(nLinesCount != NULL)
		(*nLinesCount) = 1;

	int numVertsToDraw = 0;

    while (*str)
	{
		const eqFontChar_t& chr = GetFontCharById(*(unsigned char*)str);//m_FontChars[*(unsigned char*)str];
		float cw = charWidth;

		if(enableWidthRatio)
			cw *= chr.ratio;

		Rectangle_t charRect(x,y,x + cw,y + charHeight);

		if (*str == '\n')
		{
			if(nLinesCount != NULL)
				(*nLinesCount)++;

			y += charHeight;
			x = startx;
		}
		else
		{
			if(!rect.IsFullyInside(charRect))
			{
				if(nLinesCount != NULL)
					(*nLinesCount)++;

				y += charHeight;
				x = startx;

				if(y + charHeight > rect.vrightBottom.y)
				{
					str++;
					continue;
				}
			}

			dest[0].m_vPosition = Vector2D(x, y);
			dest[0].m_vTexCoord = Vector2D(chr.x0, chr.y0);
			dest[1].m_vPosition = Vector2D(x + cw, y);
			dest[1].m_vTexCoord = Vector2D(chr.x1, chr.y0);
			dest[2].m_vPosition = Vector2D(x, y + charHeight);
			dest[2].m_vTexCoord = Vector2D(chr.x0, chr.y1);

			dest[3].m_vPosition = Vector2D(x, y + charHeight);
			dest[3].m_vTexCoord = Vector2D(chr.x0, chr.y1);
			dest[4].m_vPosition = Vector2D(x + cw, y);
			dest[4].m_vTexCoord = Vector2D(chr.x1, chr.y0);
			dest[5].m_vPosition = Vector2D(x + cw, y + charHeight);
			dest[5].m_vTexCoord = Vector2D(chr.x1, chr.y1);

			dest[0].m_vColor = m_textColor;
			dest[1].m_vColor = m_textColor;
			dest[2].m_vColor = m_textColor;
			dest[3].m_vColor = m_textColor;
			dest[4].m_vColor = m_textColor;
			dest[5].m_vColor = m_textColor;

			dest			+= 6;
			numVertsToDraw	+= 6;
			x += cw + m_spacing;
		}

		str++;
    } //while

	return numVertsToDraw;
}

//
// LEGACY - fills ASCII text
//
int CFont::FillTextBuffer(Vertex2D_t *dest, const char *str, float x, float y, float charWidth, float charHeight, ETextOrientation textOrientation, bool enableWidthRatio, int styleFlags, float fOffset)
{
	int numVertsToDraw = 0;

	float startx = x;

    while (*str)
	{
		switch(textOrientation)
		{
			case TEXT_ORIENT_UP:
			case TEXT_ORIENT_DOWN:
			{
				if (*str == '\n')
				{
					// Do nothing in this case
				}
				else
				{
					const eqFontChar_t& chr = GetFontCharById(*(ubyte*) str); //m_FontChars[*(ubyte*) str];

					float cw = charWidth;

					if(enableWidthRatio)
						cw *= chr.ratio;

					float ch = charHeight;

					dest[0].m_vPosition = Vector2D(x-fOffset, y-fOffset);
					dest[0].m_vTexCoord = Vector2D(chr.x0, chr.y0);
					dest[1].m_vPosition = Vector2D(x + cw + fOffset, y-fOffset);
					dest[1].m_vTexCoord = Vector2D(chr.x1, chr.y0);
					dest[2].m_vPosition = Vector2D(x-fOffset, y + charHeight+fOffset);
					dest[2].m_vTexCoord = Vector2D(chr.x0, chr.y1);

					dest[3].m_vPosition = Vector2D(x-fOffset, y + charHeight+fOffset);
					dest[3].m_vTexCoord = Vector2D(chr.x0, chr.y1);
					dest[4].m_vPosition = Vector2D(x + cw+fOffset, y-fOffset);
					dest[4].m_vTexCoord = Vector2D(chr.x1, chr.y0);
					dest[5].m_vPosition = Vector2D(x + cw+fOffset, y + charHeight+fOffset);
					dest[5].m_vTexCoord = Vector2D(chr.x1, chr.y1);

					dest[0].m_vColor = m_textColor;
					dest[1].m_vColor = m_textColor;
					dest[2].m_vColor = m_textColor;
					dest[3].m_vColor = m_textColor;
					dest[4].m_vColor = m_textColor;
					dest[5].m_vColor = m_textColor;

					numVertsToDraw	+= 6;
					dest			+= 6;

					y += (textOrientation == TEXT_ORIENT_DOWN) ? (ch+m_spacing) : -(ch+m_spacing);
				}
				break;
			}
			case TEXT_ORIENT_RIGHT:
			default:
			{
				if (*str == '\n')
				{
					y += charHeight;
					x = startx;
				}
				else
				{
					const eqFontChar_t& chr = GetFontCharById(*(ubyte*) str); //m_FontChars[*(ubyte*) str];

					float cw = charWidth;
					if(enableWidthRatio)
						cw *= chr.ratio;

					dest[0].m_vPosition = Vector2D(x-fOffset, y-fOffset);
					dest[0].m_vTexCoord = Vector2D(chr.x0, chr.y0);
					dest[1].m_vPosition = Vector2D(x + cw + fOffset, y-fOffset);
					dest[1].m_vTexCoord = Vector2D(chr.x1, chr.y0);
					dest[2].m_vPosition = Vector2D(x-fOffset, y + charHeight+fOffset);
					dest[2].m_vTexCoord = Vector2D(chr.x0, chr.y1);

					dest[3].m_vPosition = Vector2D(x-fOffset, y + charHeight+fOffset);
					dest[3].m_vTexCoord = Vector2D(chr.x0, chr.y1);
					dest[4].m_vPosition = Vector2D(x + cw+fOffset, y-fOffset);
					dest[4].m_vTexCoord = Vector2D(chr.x1, chr.y0);
					dest[5].m_vPosition = Vector2D(x + cw+fOffset, y + charHeight+fOffset);
					dest[5].m_vTexCoord = Vector2D(chr.x1, chr.y1);

					dest[0].m_vColor = m_textColor;
					dest[1].m_vColor = m_textColor;
					dest[2].m_vColor = m_textColor;
					dest[3].m_vColor = m_textColor;
					dest[4].m_vColor = m_textColor;
					dest[5].m_vColor = m_textColor;

					numVertsToDraw	+= 6;
					dest			+= 6;
					x += cw+m_spacing;
				}
				break;
			}
		} //switch

		str++;
    } //while

	return numVertsToDraw;
}*/

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

			bool filter_font = KV_GetValueBool( fontSec->FindKeyBase("filter") );

			m_spacing = 0.0f;
			m_fontTexture = g_pShaderAPI->LoadTexture(KV_GetValueString(fontSec->FindKeyBase("texture")), filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

			m_baseline = KV_GetValueFloat( fontSec->FindKeyBase("baseline") );
			m_lineHeight = KV_GetValueFloat( fontSec->FindKeyBase("lineheight") );
			m_scale = KV_GetVector2D( fontSec->FindKeyBase("scale") );

			m_invTexSize = Vector2D(1.0f, 1.0f);
			
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

				fontChar.ofsX = KV_GetValueFloat(k, 4);
				fontChar.ofsY = KV_GetValueFloat(k, 5);
				fontChar.advX = KV_GetValueFloat(k, 6);
				
				if( g_pShaderAPI->GetShaderAPIClass() == SHADERAPI_DIRECT3D9 )
				{
					// fix half texel on DX9
					fontChar.x0 -= 0.5f;
					fontChar.y0 -= 0.5f;
					fontChar.x1 -= 0.5f;
					fontChar.y1 -= 0.5f;
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

				m_baseline = 8.0f;
				m_lineHeight = 8.0f;

				int charsperline =  KV_GetValueInt(pFontSizeSection->FindKeyBase("charsperline"), 0, 16);

				float interval = KV_GetValueFloat(pFontSizeSection->FindKeyBase("interval"), 0, 0.75);

				m_fontTexture = g_pShaderAPI->LoadTexture(texname.GetData(),filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

				if(m_fontTexture == NULL)
				{
					MsgError("ERROR: no texture for font '%s'\n",finalFileName.GetData());
					return false;
				}

				m_fontTexture->Ref_Grab();
				m_invTexSize = Vector2D(1.0f/m_fontTexture->GetWidth(), 1.0f/m_fontTexture->GetHeight());

				int line = 0;
				int lChars = 0;
				for(int i = 0; i < 256;i++)
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

		//MsgInfo("Font loaded: '%s'\n", filenamePrefix);

		return true;
	}
	else
	{
		MsgError("ERROR: Can't open font file '%s'\n", finalFileName.c_str());
	}

	return false;
}