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
#pragma fixme("Use codepages instead of extended character range")

#define FONT_DEFAULT_PATH "resources/fonts/"

ConVar ui_debugfonts("ui_debugfonts", "0", "Enable font debug recangles drawing.", CV_CHEAT);

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

CFont::CFont()
{
	m_vertexBuffer = NULL;
	m_fontTexture = NULL;
	m_numVerts = 0;

	m_textColor = color4_white;
	m_spacing = 0.0f;

	memset(m_FontChars,0,sizeof(m_FontChars));
	m_extChars = NULL;
}

CFont::~CFont()
{
	if( m_vertexBuffer )
		free( m_vertexBuffer );

	delete [] m_extChars;
}

const char*	CFont::GetName() const
{
	return m_name.c_str();
}

//
// String length in screen units
//
float CFont::GetStringLength(const char *string, int length, bool enableWidthRatio) const
{
    if (length < 0)
		length = strlen(string);

    float len = 0;

    for (register int i = 0; i < length; i++)
	{
		if(enableWidthRatio)
			len += m_FontChars[(ubyte)string[i]].ratio;
		else
			len += 1.0f;
	}

    return len;
}

//
// String length in screen units - wide char with tag support
//
float CFont::GetStringLength(const wchar_t *string, int length, bool enableWidthRatio) const
{
    if (length < 0)
		length = wcslen(string);

    float len = 0;

	int charMode = CHARMODE_NORMAL;

    for (register int i = 0; i < length; i++)
	{
		int charIdx = string[i];

		// skip fasttags
		if( charMode == CHARMODE_NORMAL && charIdx == '&')
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

		if(enableWidthRatio)
		{
			const eqfontchar_t& chr = GetFontCharById( string[i] );

			len += chr.ratio;
		}
		else
			len += 1.0f;
	}

    return len;
}

//
// Fills text buffer and processes tags
//
int CFont::FillTextBufferW(Vertex2D_t *dest, const wchar_t *str, float x, float y, float charWidth, float charHeight, const eqFontStyleParam_t& params)
{
	int numVertsToDraw = 0;

	float startx = x;

	DkLinkedList<eqFontStyleParam_t> states;
	states.addLast( params );	// push this param

	eqFontStyleParam_t parsedParams = params;

	int charMode = CHARMODE_NORMAL;
	int tagType = TEXT_TAG_NONE;

    while( *str )
	{
		int charIdx = *str;

		states.goToLast();
		eqFontStyleParam_t stateParams = states.getCurrent();

		//
		// fast tags support
		//
		if( charMode == CHARMODE_NORMAL &&
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
		// Text filling
		//

		const eqfontchar_t& chr = GetFontCharById(charIdx);

		float cw = charWidth;
		float ch = charHeight;

		if(stateParams.styleFlag & TEXT_STYLE_WIDTHRATIO)
			cw *= chr.ratio;

		if (*str == L'\n')
		{
			switch(stateParams.orient)
			{
				case TEXT_ORIENT_UP:
				case TEXT_ORIENT_DOWN:
				{
					continue;
				}
				case TEXT_ORIENT_RIGHT:
				default:
				{
					y += charHeight;
					x = startx;
				}
			}

			str++;
			continue;
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

		dest[0].m_vColor = m_textColor*stateParams.textColor;
		dest[1].m_vColor = m_textColor*stateParams.textColor;
		dest[2].m_vColor = m_textColor*stateParams.textColor;
		dest[3].m_vColor = m_textColor*stateParams.textColor;
		dest[4].m_vColor = m_textColor*stateParams.textColor;
		dest[5].m_vColor = m_textColor*stateParams.textColor;

		switch(stateParams.orient)
		{
			case TEXT_ORIENT_UP:
			case TEXT_ORIENT_DOWN:
			{
				y += (stateParams.orient == TEXT_ORIENT_DOWN) ? (ch+m_spacing) : -(ch+m_spacing);
				break;
			}
			case TEXT_ORIENT_RIGHT:
			default:
			{
				x += cw+m_spacing;
			}
		}

		numVertsToDraw	+= 6;
		dest			+= 6;

		str++;
	
    } //while

	return numVertsToDraw;
}

//
// Renders new styled tagged text - wide chars only
//
void CFont::RenderText(const wchar_t* pszText,int x, int y, int cw, int ch, const eqFontStyleParam_t& params)
{
	int n = 6 * GetTextQuadsCount(pszText);
	if (n == 0)
		return;

	if (n > m_numVerts)
	{
		Vertex2D_t* newBuf = (Vertex2D_t *) realloc(m_vertexBuffer, n * sizeof(Vertex2D_t));

		if(newBuf)
		{
			m_vertexBuffer = newBuf;
			m_numVerts = n;
		}
	}

	if( params.align != TEXT_ALIGN_LEFT )
	{
		float strWidth = GetStringLength(pszText, -1, (params.styleFlag & TEXT_STYLE_WIDTHRATIO));

		if(params.align == TEXT_ALIGN_CENTER)
		{
			x -= strWidth * cw * 0.5f;
		}
		else if(params.align == TEXT_ALIGN_RIGHT)
		{
			x -= strWidth * cw;
		}
	}

	int numVertsToDraw = FillTextBufferW(m_vertexBuffer, pszText, x, y, cw, ch,params);

	ASSERTMSG(numVertsToDraw <= m_numVerts, varargs("Font:RenderText error: numVertsToDraw > m_numVerts (%d > %d)", numVertsToDraw, m_numVerts));

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	
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
const eqfontchar_t& CFont::GetFontCharById( const int chrId ) const
{
	if(chrId > 255)
	{
		if(chrId >= m_extCharsStart && chrId <= m_extCharsStart+m_extCharsLength)
		{
			return m_extChars[chrId-m_extCharsStart];
		}
		else
			return m_FontChars[' '];
	}
	else
		return m_FontChars[chrId];
}

//
// returns maximum of possible quads to be allocated
//
int CFont::GetTextQuadsCount(const wchar_t *str) const
{
	int n = 0;

	int charMode = CHARMODE_NORMAL;
	while (*str)
	{
		int charIdx = *str;

		// skip fasttags
		if( charMode == CHARMODE_NORMAL && charIdx == '&')
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

		if (*str != L'\n')
			n++;

		str++;
	}

	return n;
}

//-------------------------------------------------------------------------------------------------------------------------------------

//
// LEGACY - render ASCII text
//
void CFont::DrawText(const char* pszText,int x, int y, int cw, int ch, bool enableWidthRatio )
{
	int n = 6 * GetTextQuadsCount(pszText);
	if (n == 0)
		return;

	if (n > m_numVerts)
	{
		Vertex2D_t* newBuf = (Vertex2D_t *) realloc(m_vertexBuffer, n * sizeof(Vertex2D_t));

		if(newBuf)
		{
			m_vertexBuffer = newBuf;
			m_numVerts = n;
		}

	}

	int numVertsToDraw = FillTextBuffer(m_vertexBuffer, pszText, x, y, cw, ch, TEXT_ORIENT_RIGHT, enableWidthRatio);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, color4_white, &blending);
}

//
// LEGACY - render ASCII text with style
//
void CFont::DrawTextEx(const char* pszText,int x, int y, int cw, int ch, ETextOrientation textOrientation, int styleFlags, bool enableWidthRatio)
{
	int n = 6 * GetTextQuadsCount(pszText);
	if (n == 0)
		return;

	if (n > m_numVerts)
	{
		Vertex2D_t* newBuf = (Vertex2D_t *) realloc(m_vertexBuffer, n * sizeof(Vertex2D_t));

		if(newBuf)
		{
			m_vertexBuffer = newBuf;
			m_numVerts = n;
		}

	}

	if(styleFlags & TEXT_STYLE_CENTER)
	{
		float strHalfWidth = GetStringLength(pszText, -1, enableWidthRatio) * 0.5f;

		x -= strHalfWidth*cw;
	}

	int numVertsToDraw = FillTextBuffer(m_vertexBuffer, pszText, x, y, cw, ch,textOrientation, enableWidthRatio, styleFlags);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	
	if(styleFlags & TEXT_STYLE_SHADOW)
	{
		float shadowOffset = 1.0f;

		if(enableWidthRatio)
			shadowOffset = 2.0f;

		materials->SetMatrix(MATRIXMODE_WORLD, translate(shadowOffset,shadowOffset,0.0f));
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, n, m_fontTexture, ColorRGBA(0,0,0,0.45f), &blending);
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, color4_white, &blending);
}

//
// LEGACY - render ASCII text in rectangle
//
void CFont::DrawTextInRect(const char* pszText,Rectangle_t &clipRect, int cw, int ch, bool enableWidthRatio, int *nLinesCount)
{
	int n = 6 * GetTextQuadsCount(pszText);
	if (n == 0) return;

	if (n > m_numVerts)
	{
		Vertex2D_t* newBuf = (Vertex2D_t *) realloc(m_vertexBuffer, n * sizeof(Vertex2D_t));

		if(newBuf)
		{
			m_vertexBuffer = newBuf;
			m_numVerts = n;
		}
	}

	int numVertsToDraw = RectangularFillTextBuffer(m_vertexBuffer, pszText, clipRect.vleftTop.x+2, clipRect.vleftTop.y+2, cw, ch, clipRect,nLinesCount, enableWidthRatio);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, m_vertexBuffer, numVertsToDraw, m_fontTexture, color4_white, &blending);
}

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
		const eqfontchar_t& chr = m_FontChars[*(unsigned char*)str];
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
					const eqfontchar_t& chr = m_FontChars[*(ubyte*) str];

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
					const eqfontchar_t& chr = m_FontChars[*(ubyte*) str];

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
}

// sets color of next rendered text
void CFont::DrawSetColor(const ColorRGBA &color)
{
	m_textColor = color;
}

void CFont::DrawSetColor(float r, float g, float b, float a)
{
	m_textColor.x = r;
	m_textColor.y = g;
	m_textColor.z = b;
	m_textColor.w = a;
}

int CFont::GetTextQuadsCount(const char *str) const
{
	int n = 0;

	while (*str)
	{
		if (*str != '\n')
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

			bool filter_font = KV_GetValueBool( fontSec->FindKeyBase("filter") );

			m_spacing = 0.0f;
			m_fontTexture = g_pShaderAPI->LoadTexture(KV_GetValueString(fontSec->FindKeyBase("texture")), filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

			Vector2D invTexSize(1.0f, 1.0f);
			
			if(m_fontTexture != NULL)
			{
				m_fontTexture->Ref_Grab();

				invTexSize = Vector2D(1.0f/m_fontTexture->GetWidth(), 1.0f/m_fontTexture->GetHeight());
			}

			m_extCharsStart = INT_MAX;
			m_extCharsLength = 0;

			DkList<eqfontchar_t>	extChars;
			DkList<int>				extCharIds;

			for(int i = 0; i < fontSec->keys.numElem(); i++)
			{
				kvkeybase_t* k = fontSec->keys[i];

				if(k->values.numElem() < 7)
					continue;

				int charIdx = atoi(k->name);

				eqfontchar_t fontChar;

				// x y w h sx sy advanceX
				// 0 1 2 3 4  5  6

				fontChar.x0 = KV_GetValueFloat(k, 0);
				fontChar.y0 = KV_GetValueFloat(k, 1);

				fontChar.x1 = fontChar.x0 + KV_GetValueFloat(k, 2);
				fontChar.y1 = fontChar.y0 + KV_GetValueFloat(k, 3);

				fontChar.ratio = KV_GetValueFloat(k, 2);//*invTexSize.x;
				
				if( g_pShaderAPI->GetShaderAPIClass() != SHADERAPI_DIRECT3D9 )
				{
					// fix half texel on DX9
					fontChar.x0 += 0.5f;
					fontChar.y0 += 0.5f;

					fontChar.x1 += 0.5f;
					fontChar.y1 += 0.5f;
				}

				fontChar.x0 *= invTexSize.x;
				fontChar.y0 *= invTexSize.y;
				fontChar.x1 *= invTexSize.x;
				fontChar.y1 *= invTexSize.y;

				if(charIdx >= 0 && charIdx < 256)
				{
					m_FontChars[charIdx] = fontChar;
				}
				else
				{
					// need to know what range of extra characters we have
					if(charIdx < m_extCharsStart)
						m_extCharsStart = charIdx;

					if(charIdx > m_extCharsLength)
						m_extCharsLength = charIdx;

					extChars.append( fontChar );
					extCharIds.append( charIdx );
				}
			}

			// Map extended characters to range
			if( m_extCharsLength > 0 && extChars.numElem() > 0)
			{
				//MsgInfo("Font '%s' uses extended character range [%d..%d]\n", filenamePrefix, m_extCharsStart, m_extCharsLength);

				m_extCharsLength -= m_extCharsStart;

				m_extChars = new eqfontchar_t[ m_extCharsLength+1 ];	// this is a fast map

				// fill with empty chars first
				for(int i = 0; i < m_extCharsLength; i++)
					m_extChars[i] = m_FontChars[' '];

				// put them right in range
				for(int i = 0; i < extChars.numElem(); i++)
				{
					int charPos = extCharIds[i] - m_extCharsStart;
					m_extChars[ charPos ] = extChars[i];
				}
			}
		}
		else
		{
			EqString texname = KV_GetValueString( fontSec->FindKeyBase("Texture"), 0, filenamePrefix );

			bool isAutogeneratedFont = false;

			bool filter_font = KV_GetValueBool( fontSec->FindKeyBase("FilterFontTexture") );

			float fSpacing = KV_GetValueFloat( fontSec->FindKeyBase("spacing") );

			kvkeybase_t* pKey = fontSec->FindKeyBase("FontDimsTable");

			// Old font format used by humus
			if(pKey)
			{
				EqString fontFile_name = _Es(TEXTURE_DEFAULT_PATH) + KV_GetValueString(pKey);

				IFile* file = GetFileSystem()->Open( fontFile_name.GetData(), "rb" );

				if(!file)
				{
					MsgError("ERROR: '%s' failed to load:\n",finalFileName.GetData());
					MsgError("	Can't open file '%s'!\n", fontFile_name.c_str());
					return false;
				}

				m_spacing = fSpacing;

				isAutogeneratedFont = false;

				uint version = 0;

				file->Read(&version, 1, sizeof(int));
				file->Read(m_FontChars, 1, sizeof(eqfontchar_t)*256);
				GetFileSystem()->Close(file);

				m_fontTexture = g_pShaderAPI->LoadTexture(texname.GetData(),filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

				if(m_fontTexture != NULL)
					m_fontTexture->Ref_Grab();

				return true;
			}

			// the monospace fonts for debug overlays and other purposes
			kvkeybase_t* pFontSizeSection = fontSec->FindKeyBase("FontSize", KV_FLAG_SECTION);
			if(pFontSizeSection)
			{
				isAutogeneratedFont = true;

				m_spacing = fSpacing;

				int wide = KV_GetValueInt(pFontSizeSection->FindKeyBase("width"), 0, 8);

				int tall = KV_GetValueInt(pFontSizeSection->FindKeyBase("height"), 0, 8);

				float ratio = 1;

				int charsperline =  KV_GetValueInt(pFontSizeSection->FindKeyBase("charsperline"), 0, 16);

				float interval = KV_GetValueFloat(pFontSizeSection->FindKeyBase("interval"), 0, 0.75);

				m_fontTexture = g_pShaderAPI->LoadTexture(texname.GetData(),filter_font ? TEXFILTER_LINEAR : TEXFILTER_NEAREST,ADDRESSMODE_WRAP, TEXFLAG_NOQUALITYLOD);

				if(m_fontTexture == NULL)
				{
					MsgError("ERROR: no texture for font '%s'\n",finalFileName.GetData());
					return false;
				}

				m_fontTexture->Ref_Grab();

				int line = 0;
				int lChars = 0;
				for(int i = 0; i < 256;i++)
				{
					if(lChars == charsperline)
					{
						line++;
						lChars = 0;
					}

					eqfontchar_t& chr = m_FontChars[i];

					m_FontChars[i].ratio = ratio;

					float CurCharPos_x = lChars * tall;
					float CurCharPos_y = line * tall;

					chr.x0 = CurCharPos_x + interval; // LineCharCount * interval
					chr.x1 = CurCharPos_x + interval + wide;
					chr.y0 = CurCharPos_y + interval; // Line Count * height
					chr.y1 = CurCharPos_y + interval + tall;

					chr.x0 /= m_fontTexture->GetWidth();
					chr.x1 /= m_fontTexture->GetWidth();

					chr.y0 /= m_fontTexture->GetHeight();
					chr.y1 /= m_fontTexture->GetHeight();

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

// loads font
IEqFont* InternalLoadFont(const char* pszFontName)
{
	CFont* newFont = new CFont();
	if( !newFont->LoadFont(pszFontName) )
	{
		delete newFont;
		return NULL;
	}

	return newFont;
}
