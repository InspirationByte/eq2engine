//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine unicode font renderer and cacher
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "UFont.h"
#include "Platform.h"
#include "math/rectangle.h"
#include "utils/DkList.h"

struct FontArea
{
	FontArea() : rectangle(), underhang(0), overhang(0) {}
	Rectangle_t rectangle;
	int			underhang;
	int			overhang;
	wchar_t		symbol;
};

int CALLBACK FontEnumProc( const LOGFONT *lpelfe,    // logical-font data
  const TEXTMETRIC *lpntme,  // physical-font data
  DWORD FontType,           // type of font
  LPARAM lParam             // application-defined data
)
{
	return 0;
}

void GenerateBitmapFont(char* pszFontName, int nFontTall, int nFontWeight, bool bItalic, bool bUnderline, bool bStrikeOut)
{
	HDC dc = CreateCompatibleDC(NULL);

	LOGFONT logfont;
	logfont.lfCharSet = RUSSIAN_CHARSET;
	logfont.lfPitchAndFamily = 0;
	strcpy( logfont.lfFaceName, pszFontName );

	::EnumFontFamiliesEx(dc, &logfont, &FontEnumProc, 0, 0);

	int charset = RUSSIAN_CHARSET;

	HFONT font = ::CreateFontA(	nFontTall, 0, 0, 0, 
								nFontWeight, 
								bItalic, 
								bUnderline, 
								bStrikeOut, 
								charset, 
								OUT_DEFAULT_PRECIS, 
								CLIP_DEFAULT_PRECIS, 
								ANTIALIASED_QUALITY, 
								DEFAULT_PITCH | FF_DONTCARE, 
								pszFontName);

	// set as the active font
	SelectObject(dc, font);
	SetTextAlign(dc,TA_LEFT | TA_TOP | TA_NOUPDATECP);

	// get info about the font
	::TEXTMETRIC tm;
	memset( &tm, 0, sizeof( tm ) );

	if ( !GetTextMetrics(dc, &tm) )
	{
		return;
	}

	int nOutlineSize = 0;
	int nDropShadowOffset = 0;

	// draw characters

	int wide_tall = 1024;

	HBITMAP bmp = CreateCompatibleBitmap(dc, wide_tall, wide_tall);

	LOGBRUSH lbrush;
	lbrush.lbColor = RGB(0,0,0);
	lbrush.lbHatch = 0;
	lbrush.lbStyle = BS_SOLID;
	
	HBRUSH brush = CreateBrushIndirect(&lbrush);
	HPEN pen = CreatePen(PS_NULL, 0, 0);

	HGDIOBJ oldbmp = SelectObject(dc, bmp);
	HGDIOBJ oldbmppen = SelectObject(dc, pen);
	HGDIOBJ oldbmpbrush = SelectObject(dc, brush);
	HGDIOBJ oldbmpfont = SelectObject(dc, font);

	SetTextColor(dc, RGB(255,255,255));

	Rectangle(dc, 0,0,wide_tall,wide_tall);
	SetBkMode(dc, TRANSPARENT);

	// get information about this font's unicode ranges.
	int size = GetFontUnicodeRanges( dc, 0);
	ubyte *buf = new ubyte[size];
	LPGLYPHSET glyphs = (LPGLYPHSET)buf;

	GetFontUnicodeRanges( dc, glyphs);

	DkList<FontArea> chars;
	chars.resize(512);

	int currentx=0, currenty=0, maxy=0;

	for (int range=0; range < glyphs->cRanges; range++)
	{
		WCRANGE* current = &glyphs->ranges[range];

		//maxy=0;

		// loop through each glyph and write its size and position
		for (int ch=current->wcLow; ch< current->wcLow + current->cGlyphs; ch++)
		{
			wchar_t currentchar = ch;

			if ( IsDBCSLeadByte((BYTE) ch))
				continue;	// surragate pairs unsupported

			// get the dimensions
			SIZE size;
			ABC abc;
			GetTextExtentPoint32W(dc, &currentchar, 1, &size);

			FontArea fa;
			fa.underhang = 0;
			fa.overhang  = 0;

			if (GetCharABCWidthsW(dc, currentchar, currentchar, &abc)) // for unicode fonts, get overhang, underhang, width
			{
				size.cx = abc.abcB;
				fa.underhang  = abc.abcA;
				fa.overhang   = abc.abcC;

				if (abc.abcB-abc.abcA+abc.abcC<1)
					continue;	// nothing of width 0
			}

			size.cx += 2;

			if (size.cy < 1)
				continue;

			//GetGlyphOutline(dc, currentchar, GGO_METRICS, &gm, 0, 0, 0);

			//size.cx++; size.cy++;
			
			// wrap around?
			if (currentx + size.cx > (int) wide_tall)
			{
				currenty += maxy;
				currentx = 0;
				/*
				if ((int)(currenty + maxy) > textureHeight)
				{
					currentImage++; // increase Image count
					currenty=0;
				}*/
				maxy = 0;
			}
			// add this char dimension to the current map
			

			fa.rectangle = Rectangle_t(currentx, currenty, currentx + size.cx, currenty + size.cy);
			
			fa.symbol = currentchar;
			chars.append(fa);

			currentx += size.cx +1;

			if (size.cy+1 > maxy)
				maxy = size.cy+1;
		}
	}
	currenty += maxy;

	FontArea* d;

	for (int i = 0; i < chars.numElem(); i++)
	{
		d = &chars[i];

		int left_top = d->rectangle.vleftTop.x - d->underhang;

		TextOutW(dc, d->rectangle.vleftTop.x, d->rectangle.vleftTop.y, &d->symbol, 1);
	}

	// copy to clipboard
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData(CF_BITMAP, bmp);
	CloseClipboard();	

	DeleteDC(dc);
	DeleteObject(bmp);
}