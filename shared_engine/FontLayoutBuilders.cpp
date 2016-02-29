//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Font layout builders
//////////////////////////////////////////////////////////////////////////////////

#include "FontLayoutBuilders.h"

#ifdef PLAT_POSIX
#include <wctype.h> // iswspace
#endif

void CRectangleTextLayoutBuilder::OnNewLine(const eqFontStyleParam_t& params,
										void* strCurPos, bool isWideChar,
										int lineNumber,
										const Vector2D& textStart,
										Vector2D& curTextPos )
{
	if(lineNumber > 0)
		curTextPos.y += m_font->GetLineHeight();

	m_linesProduced++;

	float newlineStringWidth;

	if(isWideChar)
		newlineStringWidth = m_font->GetStringWidth( (wchar_t*)strCurPos, params.styleFlag, -1, '\n' );
	else
		newlineStringWidth = m_font->GetStringWidth( (char*)strCurPos, params.styleFlag, -1, '\n' );

	curTextPos.x = m_rectangle.vleftTop.x;

	// calc start position for first time
	if( params.align != TEXT_ALIGN_LEFT )
	{
		if(params.align & TEXT_ALIGN_HCENTER)
		{
			curTextPos.x = m_rectangle.GetCenter().x;
			curTextPos.x -= newlineStringWidth*0.5f;
		}
		else if(params.align & TEXT_ALIGN_RIGHT)
		{
			curTextPos.x = m_rectangle.vrightBottom.x-2.0f;
			curTextPos.x -= newlineStringWidth;
		}

		curTextPos.x = floor(curTextPos.x);
	}

	m_newWord = true;
}

bool CRectangleTextLayoutBuilder::LayoutChar(const eqFontStyleParam_t& params,
											void* strCurPos, bool isWideChar,
											const eqFontChar_t& chr,
											Vector2D& curTextPos,
											Vector2D& cPos, Vector2D& cSize )
{
	if(m_newWord) // new word always enables the word wrapping again
	{
		m_wrappedWord = false;
		m_wordWrapMode = true;
	}

	{
		float wordSize = cSize.x; // per-char wrapping

		bool wordWrap = m_wordWrapMode && m_newWord;

		if( wordWrap ) // per-word wrapping
		{
			if(isWideChar)
				wordSize = m_font->GetStringWidth( (wchar_t*)strCurPos, params.styleFlag, -1, ' ' );
			else
				wordSize = m_font->GetStringWidth( (char*)strCurPos, params.styleFlag, -1, ' ' );
		}

		// if word can't be wrapped, we switch to character wrapping
		if( m_wrappedWord && curTextPos.x+wordSize > m_rectangle.vrightBottom.x )
		{
			m_wordWrapMode = false;
			wordSize = cSize.x; // per-char wrapping
		}

		// check character/word right bound is outside the rectangle right bound
		if( curTextPos.x+wordSize > m_rectangle.vrightBottom.x )
		{
			float xPos = m_rectangle.vleftTop.x;

			// calc start position for first time
			if( params.align != TEXT_ALIGN_LEFT )
			{
				float newlineStringWidth;

				if(isWideChar) // TODO: must be calculated until next word wrap
					newlineStringWidth = m_font->GetStringWidth( (wchar_t*)strCurPos, params.styleFlag, -1, '\n' );
				else
					newlineStringWidth = m_font->GetStringWidth( (char*)strCurPos, params.styleFlag, -1, '\n' );

				if(params.align & TEXT_ALIGN_HCENTER)
				{
					xPos = m_rectangle.GetCenter().x;
					xPos -= floor(newlineStringWidth*0.5f);
				}
				else if(params.align & TEXT_ALIGN_RIGHT)
				{
					xPos = m_rectangle.vrightBottom.x;
					xPos -= newlineStringWidth;
				}

				xPos = floor(xPos);
			}

			curTextPos.x = cPos.x = xPos;

			curTextPos.y += m_font->GetLineHeight();
			cPos.y += m_font->GetLineHeight();

			m_linesProduced++;

			if( wordWrap )
				m_wrappedWord = true;

			m_newWord = false;
		}

		// check if character bottom bound is outside the rectangle bottom bound
		if( cPos.y + cSize.y > m_rectangle.vrightBottom.y)
		{
			m_hasNotdrawnLines = true;
			return false;
		}
	}

	if(params.styleFlag & TEXT_STYLE_MONOSPACE)
		curTextPos.x += cSize.x;
	else
		curTextPos.x += chr.advX;

	if( isWideChar )
		m_newWord = iswspace(*((wchar_t*)strCurPos));
	else
		m_newWord = isspace(*((char*)strCurPos));

	return true;
}
