//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Font layout builders
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "font/IFont.h"

class CRectangleTextLayoutBuilder : public ITextLayoutBuilder
{
public:
	void	SetRectangle( const AARectangle& rectangle ) { m_rectangle = rectangle;}
	void	Reset( IEqFont* font ) { m_font = font; m_linesProduced = 0; m_newWord = true; m_hasNotdrawnLines = false; m_wrappedWord = false; m_wordWrapMode = true;}

	int		GetProducedLines() const {return m_linesProduced;}
	bool	HasNotDrawnLines() const {return m_hasNotdrawnLines;}

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

protected:
	AARectangle		m_rectangle;
	bool			m_newWord;
	int				m_linesProduced;
	bool			m_hasNotdrawnLines;

	bool			m_wrappedWord;
	bool			m_wordWrapMode;
};
