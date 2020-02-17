//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: 2D rendering device context
//////////////////////////////////////////////////////////////////////////////////

#ifndef DCRENDER2D_H
#define DCRENDER2D_H

#include "EditorHeader.h"
#include "math/BoundingBox.h"
#include "wx_inc.h"

class CViewWindow;

#define LINE_THIN	1
#define LINE_THICK	2

// implementation of 2D rendering through DC
class CDCRender2D
{
public:
	CDCRender2D(CViewWindow *pWindow, wxDC& dc);

	void		SetFillColor(const wxColour& Color);
	void		SetLineColor(const wxColour& Color);
	void		SetLineType(int nStyle, int Width, const wxColour& Color);

	void		DrawPoint(const wxPoint& Point, int Radius);
	void		DrawPoint(const Vector3D& Point, int Radius);

	void		DrawLine(const wxPoint& A, const wxPoint& B);
	void		DrawLine(const Vector3D& A, const Vector3D& B);
	void		DrawLine(int x1, int y1, int x2, int y2);
	//void		DrawLineLoop(const Vector3D& Points, int Radius);

	void		DrawEllipse(const wxPoint& Center, int RadiusX, int RadiusY, bool Fill);
	void		Rectangle(const wxRect& Rect, bool Fill);
	void		DrawBitmap(int x, int y, const wxBitmap& Bitmap);

	void		SetTextColor(const wxColour& FgColor, const wxColour& BkColor);
	void		DrawText(const wxString& Text, const wxPoint& Pos);

	// Renders the dimensions of the given bounding-box next to the box.
	void		DrawBoxDims(BoundingBox& box, int Pos);

protected:

	CViewWindow*		m_pViewWindow;
	wxDC&				m_DC;
	wxPen				m_Pen;
	wxBrush				m_Brush;
};

#endif // DCRENDER2D_H