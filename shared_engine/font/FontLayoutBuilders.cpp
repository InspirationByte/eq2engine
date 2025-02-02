//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Font layout builders
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "FontLayoutBuilders.h"
#include "materialsystem1/MeshBuilder.h"

void CRectangleTextLayoutBuilder::OnNewLine(const FontStyleParam& params,
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

void CRectangleTextLayoutBuilder::DebugDraw(CMeshBuilder& meshBuilder)
{
	meshBuilder.Color4f(1.0f, 1.0f, 0.0f, 0.5f);
	meshBuilder.Line2fv(m_rectangle.GetLeftTop(), m_rectangle.GetRightTop());
	meshBuilder.Line2fv(m_rectangle.GetRightTop(), m_rectangle.GetRightBottom());
	meshBuilder.Line2fv(m_rectangle.GetRightBottom(), m_rectangle.GetLeftBottom());
	meshBuilder.Line2fv(m_rectangle.GetLeftBottom(), m_rectangle.GetLeftTop());
}

bool CRectangleTextLayoutBuilder::LayoutChar(
	const FontStyleParam& params,
	void* strCurPos, bool isWideChar,
	const FontChar& chr,
	Vector2D& curTextPos,
	Vector2D& cPos, Vector2D& cSize )
{
	IEqFont* font = m_font;

	if(m_newWord) // new word always enables the word wrapping again
	{
		m_wordWrapMode = true;
		m_wordStartPtr = strCurPos;
		if (isWideChar)
			m_wordWidth = font->GetStringWidth((wchar_t*)strCurPos, params, -1, ' ');
		else
			m_wordWidth = font->GetStringWidth((char*)strCurPos, params, -1, ' ');
	}

	float charWidth;
	if (isWideChar)
		charWidth = font->GetStringWidth((wchar_t*)strCurPos, params, 1);
	else
		charWidth = font->GetStringWidth((char*)strCurPos, params, 1);

	{
		const bool wrapWord = m_wordWrapMode && m_newWord && curTextPos.x + m_wordWidth > m_rectangle.rightBottom.x;
		bool wrapChar = curTextPos.x > m_rectangle.rightBottom.x;

		// if word can't be wrapped, we switch to character wrapping
		if( !wrapWord && wrapChar)
		{
			m_wordWrapMode = false;
			m_wordWidth = cSize.x; // per-char wrapping
		}

		// check character/word right bound is outside the rectangle right bound
		if (wrapChar || wrapWord)
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
			m_newWord = false;
		}

		// check if character bottom bound is outside the rectangle bottom bound
		if(m_linesProduced > 1 && cPos.y + cSize.y > m_rectangle.rightBottom.y)
		{
			m_hasNotdrawnLines = true;
			return false;
		}
	}

	curTextPos.x += charWidth;

	if( isWideChar )
		m_newWord = CType::IsSpace(*((wchar_t*)strCurPos));
	else
		m_newWord = CType::IsSpace(*((char*)strCurPos));

	return true;
}
