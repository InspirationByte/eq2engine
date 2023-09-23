//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Font layout builders
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "FontLayoutBuilders.h"

void CRectangleTextLayoutBuilder::OnNewLine(const eqFontStyleParam_t& params,
										void* strCurPos, bool isWideChar,
										int lineNumber,
										const Vector2D& textStart,
										Vector2D& curTextPos )
{
	float xPos = m_rectangle.leftTop.x;

	// calc start position for first time
	if (params.align != TEXT_ALIGN_LEFT)
	{
		float newlineStringWidth;

		if(isWideChar)
			newlineStringWidth = m_font->GetStringWidth( (wchar_t*)strCurPos, params, -1, '\n' );
		else
			newlineStringWidth = m_font->GetStringWidth( (char*)strCurPos, params, -1, '\n' );

		if (xPos + newlineStringWidth < m_rectangle.rightBottom.x)
		{
			if (params.align & TEXT_ALIGN_HCENTER)
				xPos = m_rectangle.GetCenter().x - newlineStringWidth * 0.5f;
			else if (params.align & TEXT_ALIGN_RIGHT)
				xPos = m_rectangle.rightBottom.x - newlineStringWidth;
		}
	}

	curTextPos.x = xPos;

	if (lineNumber > 0)
		curTextPos.y += m_font->GetLineHeight(params);

	m_linesProduced++;

	m_newWord = true;
}

bool CRectangleTextLayoutBuilder::LayoutChar(const eqFontStyleParam_t& params,
											void* strCurPos, bool isWideChar,
											const eqFontChar_t& chr,
											Vector2D& curTextPos,
											Vector2D& cPos, Vector2D& cSize )
{
	IEqFont* font = m_font;

	if(m_newWord) // new word always enables the word wrapping again
	{
		m_wrappedWord = false;
		m_wordWrapMode = true;
	}

	{
		const bool wordWrap = m_wordWrapMode && m_newWord;
		float wordSize;
		if( wordWrap ) // per-word wrapping
		{
			if(isWideChar)
				wordSize = font->GetStringWidth( (wchar_t*)strCurPos, params, -1, ' ' );
			else
				wordSize = font->GetStringWidth( (char*)strCurPos, params, -1, ' ' );
		}
		else
		{
			wordSize = font->GetStringWidth((char*)strCurPos, params, 1);
		}

		// if word can't be wrapped, we switch to character wrapping
		if( m_wrappedWord && curTextPos.x+wordSize > m_rectangle.rightBottom.x )
		{
			m_wordWrapMode = false;
			wordSize = cSize.x; // per-char wrapping
		}

		// check character/word right bound is outside the rectangle right bound
		if( curTextPos.x + wordSize > m_rectangle.rightBottom.x && !m_newWord)
		{
			float xPos = m_rectangle.leftTop.x;

			// calc start position for first time
			if( params.align != TEXT_ALIGN_LEFT )
			{
				float newlineStringWidth;

				if(isWideChar) // TODO: must be calculated until next word wrap
					newlineStringWidth = font->GetStringWidth( (wchar_t*)strCurPos, params, -1, '\n' );
				else
					newlineStringWidth = font->GetStringWidth( (char*)strCurPos, params, -1, '\n' );

				if (xPos + newlineStringWidth <= m_rectangle.rightBottom.x)
				{
					if (params.align & TEXT_ALIGN_HCENTER)
						xPos = m_rectangle.GetCenter().x - newlineStringWidth * 0.5f;
					else if (params.align & TEXT_ALIGN_RIGHT)
						xPos = m_rectangle.rightBottom.x - newlineStringWidth;
				}
			}

			curTextPos.x = cPos.x = xPos;

			curTextPos.y += font->GetLineHeight(params);
			cPos.y += font->GetLineHeight(params);

			m_linesProduced++;

			if( wordWrap )
				m_wrappedWord = true;

			m_newWord = false;
		}

		// check if character bottom bound is outside the rectangle bottom bound
		if(m_linesProduced > 1 && cPos.y + cSize.y > m_rectangle.rightBottom.y)
		{
			m_hasNotdrawnLines = true;
			return false;
		}
	}

	if (isWideChar)
		curTextPos.x += font->GetStringWidth((wchar_t*)strCurPos, params, 1);
	else
		curTextPos.x += font->GetStringWidth((char*)strCurPos, params, 1);

	if( isWideChar )
		m_newWord = iswspace(*((wchar_t*)strCurPos));
	else
		m_newWord = isspace(*((char*)strCurPos));

	return true;
}
