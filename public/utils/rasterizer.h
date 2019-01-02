//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Simplest triangle rasterizer
//				TODO:	include PixWriter to support various formats 
//						(instead of using template)
//////////////////////////////////////////////////////////////////////////////////

#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "DebugInterface.h"
#include "Math/math_util.h"
#include "math/Vector.h"

template< class T >
struct REdge_t
{
	T Color1, Color2;
	int X1, Y1, X2, Y2;

	REdge_t(const T &color1, int x1, int y1, const T &color2, int x2, int y2);
};

template< class T >
struct RSpan_t
{
	T Color1, Color2;
	int X1, X2;

	RSpan_t(const T &color1, int x1, const T &color2, int x2);
};

/*
enum RasterBlendFunc_e
{
	BLENDFUNC_ADD = 0,
	BLENDFUNC_SUBTRACT = 0,
}
*/

template< class T >
class CRasterizer
{
public:
	CRasterizer();

	void		SetImage(T *pImage, int width, int height);

	void		SetPixel(uint x, uint y, const T &color);
	void		SetPixel(int x, int y, const T &color);
	void		SetPixel(float x, float y, const T &color);

	void		Clear(const T &color);

	void		DrawTriangle(const T &color1, const Vector2D &p1, const T &color2, const Vector2D &p2, const T &color3, const Vector2D &p3);
	void		DrawTriangleZ(const Vector3D &p1, const Vector3D &p2, const Vector3D &p3);
	void		DrawLine(const T &color1, const Vector2D &p1, const T &color2, const Vector2D &p2);

	void		SetAdditiveColor(bool bAdditive);
	void		SetColorClampDiscardEnable(bool bEnable, const T &minColor, const T &maxColor);
	void		SetEnableDepthMode(bool bEnable);
	void		SetTestMode(bool bEnable);
	void		ResetOcclusionTest();
	int			GetDrawnPixels();

protected:

	void		DrawSpan(const RSpan_t<T> &span, int y);
	void		DrawSpansBetweenEdges(const REdge_t<T> &e1, const REdge_t<T> &e2);

	T*			m_pImage;
	uint		m_Width, m_Height;

	bool		m_bAdditive;
	bool		m_bColorClampDiscard;
	bool		m_bDepthMode;

	Vector3D	m_CurrTriangle[3];
	bool		m_bUseTriangleForZ;

	bool		m_bTestMode;

	int			m_nDrawnPixels;

	T			m_ColorMin, m_ColorMax;
};

template< class T >
inline REdge_t<T>::REdge_t(const T &color1, int x1, int y1, const T &color2, int x2, int y2)
{
	if(y1 < y2)
	{
		Color1 = color1;
		X1 = x1;
		Y1 = y1;
		Color2 = color2;
		X2 = x2;
		Y2 = y2;
	}
	else
	{
		Color1 = color2;
		X1 = x2;
		Y1 = y2;
		Color2 = color1;
		X2 = x1;
		Y2 = y1;
	}
}

template< class T >
inline RSpan_t<T>::RSpan_t(const T &color1, int x1, const T &color2, int x2)
{
	if(x1 < x2)
	{
		Color1 = color1;
		X1 = x1;
		Color2 = color2;
		X2 = x2;
	}
	else
	{
		Color1 = color2;
		X1 = x2;
		Color2 = color1;
		X2 = x1;
	}
}

template< class T >
inline CRasterizer<T>::CRasterizer()
{
	m_pImage = NULL;
	m_Width = 0;
	m_Height = 0;
	m_bAdditive = false;
	m_bColorClampDiscard = false;
	m_bDepthMode = false;
	m_bUseTriangleForZ = false;
	m_bTestMode = false;
	m_nDrawnPixels = 0;
}

template< class T >
inline void CRasterizer<T>::SetAdditiveColor(bool bAdditive)
{
	m_bAdditive = bAdditive;
}

template< class T >
inline void	CRasterizer<T>::SetColorClampDiscardEnable(bool bEnable, const T &minColor, const T &maxColor)
{
	m_bColorClampDiscard = bEnable;
	m_ColorMin = minColor;
	m_ColorMax = maxColor;
}

template< class T >
inline void CRasterizer<T>::SetEnableDepthMode(bool bEnable)
{
	m_bDepthMode = bEnable;
}

template< class T >
inline void CRasterizer<T>::SetTestMode(bool bEnable)
{
	m_bTestMode = bEnable;
}

template< class T >
inline void CRasterizer<T>::ResetOcclusionTest()
{
	m_nDrawnPixels = 0;
}

template< class T >
inline int CRasterizer<T>::GetDrawnPixels()
{
	return m_nDrawnPixels;
}

template< class T >
inline void CRasterizer<T>::SetImage(T *pImage, int width, int height)
{
	m_pImage	= pImage;
	m_Width		= width;
	m_Height	= height;
}

template< class T >
inline void CRasterizer<T>::SetPixel(uint x, uint y, const T &color)
{
	if(x >= m_Width || y >= m_Height)
		return;

	if(m_bColorClampDiscard)
	{
		if(color < m_ColorMin)
			return;

		if(color > m_ColorMax)
			return;
	}

	uint pixel = (y * m_Width + x);

	if(m_bDepthMode)
	{
		// compare it with current pixel
		if(color > m_pImage[pixel])
			return;
	}

	m_nDrawnPixels++;

	if(m_bTestMode)
		return;

	if(m_bAdditive)
	{
		float result = m_pImage[pixel] + color;

		if(result > 255)
			result = 255;

		m_pImage[pixel] = (T)result;
	}
	else
		m_pImage[pixel] = color;
}

template< class T >
inline void CRasterizer<T>::SetPixel(int x, int y, const T &color)
{
	SetPixel((uint)x, (uint)y, color);
}

template< class T >
inline void CRasterizer<T>::SetPixel(float x, float y, const T &color)
{
	if(x < 0.0f || y < 0.0f)
		return;

	SetPixel((uint)x, (uint)y, color);
}

template< class T >
inline void CRasterizer<T>::Clear(const T &Color)
{
	int nCycles = m_Width*m_Height;

	for(int i = 0; i < nCycles; i++)
		m_pImage[i] = Color;
}

#define CLIP_EPS 0.0001
inline float GetZValue(const Vector3D& pt1, const Vector3D& pt2, const Vector3D& pt3, const Vector3D& linept, const Vector3D& vect)
{
	Plane plane((Vector3D)pt1,(Vector3D)pt2,(Vector3D)pt3);

	// get triangle edges
	Vector3D edge1 = pt2-pt1;
	Vector3D edge2 = pt3-pt1;

	// find normal
	Vector3D pvec = cross(vect, edge2);

	float det = dot(edge1, pvec);

	// No culling
	if(det > -CLIP_EPS && det < CLIP_EPS)
		return false;

//	float inv_det = 1.0f / det;

	Vector3D tvec = linept-pt1;
	Vector3D qvec = cross(tvec, edge1);

	return dot(edge2, qvec);
}

template< class T >
inline void CRasterizer<T>::DrawSpan(const RSpan_t<T> &span, int y)
{
	int xdiff = span.X2 - span.X1;
	if(xdiff == 0)
		return;

	T colordiff = span.Color2 - span.Color1;

	float factor = 0.0f;
	float factorStep = 1.0f / (float)xdiff;

	int min,max;
	min = clamp(span.X1, 0, (int)m_Width);
	max = clamp(span.X2, 0, (int)m_Width);

	// draw each pixel in the span
	for(int x = min; x < max; x++)
	{
		SetPixel(x, y, span.Color1 + (colordiff * factor));
		factor += factorStep;
	}
}

template< class T >
inline void CRasterizer<T>::DrawSpansBetweenEdges(const REdge_t<T> &e1, const REdge_t<T> &e2)
{
	// calculate difference between the y coordinates
	// of the first edge and return if 0
	float e1ydiff = (float)(e1.Y2 - e1.Y1);
	if(e1ydiff == 0.0f)
		return;

	// calculate difference between the y coordinates
	// of the second edge and return if 0
	float e2ydiff = (float)(e2.Y2 - e2.Y1);
	if(e2ydiff == 0.0f)
		return;

	// calculate differences between the x coordinates
	// and colors of the points of the edges
	float e1xdiff = (float)(e1.X2 - e1.X1);
	float e2xdiff = (float)(e2.X2 - e2.X1);
	T e1colordiff = (e1.Color2 - e1.Color1);
	T e2colordiff = (e2.Color2 - e2.Color1);

	// calculate factors to use for interpolation
	// with the edges and the step values to increase
	// them by after drawing each span
	float factor1 = (float)(e2.Y1 - e1.Y1) / e1ydiff;
	float factorStep1 = 1.0f / e1ydiff;
	float factor2 = 0.0f;
	float factorStep2 = 1.0f / e2ydiff;

	int min, max;
	min = clamp(e2.Y1, 0, (int)m_Height);
	max = clamp(e2.Y2, 0, (int)m_Height);

	// loop through the lines between the edges and draw spans
	for(int y = min; y < max; y++)
	{
		// create and draw span
		RSpan_t<T> span(	(T)e1.Color1 + (e1colordiff * factor1),
						e1.X1 + (int)(e1xdiff * factor1),
						(T)e2.Color1 + (e2colordiff * factor2),
						e2.X1 + (int)(e2xdiff * factor2));
		DrawSpan(span, y);

		// increase factors
		factor1 += factorStep1;
		factor2 += factorStep2;
	}
}

template < class T >
inline void CRasterizer<T>::DrawTriangle( const T &color1, const Vector2D &p1, const T &color2, const Vector2D &p2, const T &color3, const Vector2D &p3 )
{
	// create edges for the triangle
	REdge_t<T> edges[3] = 
	{
		REdge_t<T>(color1, (int)p1.x, (int)p1.y, color2, (int)p2.x, (int)p2.y),
		REdge_t<T>(color2, (int)p2.x, (int)p2.y, color3, (int)p3.x, (int)p3.y),
		REdge_t<T>(color3, (int)p3.x, (int)p3.y, color1, (int)p1.x, (int)p1.y)
	};

	int maxLength = 0;
	int longEdge = 0;

	// find edge with the greatest length in the y axis
	for(int i = 0; i < 3; i++)
	{
		int length = edges[i].Y2 - edges[i].Y1;
		if(length > maxLength)
		{
			maxLength = length;
			longEdge = i;
		}
	}

	int shortEdge1 = (longEdge + 1) % 3;
	int shortEdge2 = (longEdge + 2) % 3;

	// draw spans between edges; the long edge can be drawn
	// with the shorter edges to draw the full triangle
	DrawSpansBetweenEdges(edges[longEdge], edges[shortEdge1]);
	DrawSpansBetweenEdges(edges[longEdge], edges[shortEdge2]);
}

template < class T >
inline void CRasterizer<T>::DrawTriangleZ(const Vector3D &p1, const Vector3D &p2, const Vector3D &p3)
{
	m_bUseTriangleForZ = true;

	m_CurrTriangle[0] = p1;
	m_CurrTriangle[1] = p2;
	m_CurrTriangle[2] = p3;

	DrawTriangle(p1.z,p1.xy(),p2.z,p2.xy(),p3.z,p3.xy());
	m_bUseTriangleForZ = false;
}

template < class T >
inline void CRasterizer<T>::DrawLine(const T &color1, const Vector2D &p1, const T &color2, const Vector2D &p2)
{
	float xdiff = (p2.x - p1.x);
	float ydiff = (p2.y - p1.y);

	if(xdiff == 0.0f && ydiff == 0.0f)
	{
		SetPixel(p1.x, p1.y, color1);
		return;
	}

	if(fabs(xdiff) > fabs(ydiff))
	{
		float xmin, xmax;

		// set xmin to the lower x value given
		// and xmax to the higher value
		if(p1.x < p2.x)
		{
			xmin = p1.x;
			xmax = p2.x;
		}
		else
		{
			xmin = p2.x;
			xmax = p1.x;
		}

		xmin = clamp(xmin, 0, m_Height);
		xmax = clamp(xmax, 0, m_Height);

		// draw line in terms of y slope
		float slope = ydiff / xdiff;
		for(float x = xmin; x <= xmax; x += 1.0f)
		{
			float y = p1.y + ((x - p1.x) * slope);
			T color = color1 + ((color2 - color1) * ((x - p1.x) / xdiff));
			SetPixel(x, y, color);
		}
	}
	else 
	{
		float ymin, ymax;

		// set ymin to the lower y value given
		// and ymax to the higher value
		if(p1.y < p2.y)
		{
			ymin = p1.y;
			ymax = p2.y;
		}
		else
		{
			ymin = p2.y;
			ymax = p1.y;
		}

		ymin = clamp(ymin, 0, m_Height);
		ymax = clamp(ymax, 0, m_Height);

		// draw line in terms of x slope
		float slope = xdiff / ydiff;
		for(float y = ymin; y <= ymax; y += 1.0f)
		{
			float x = p1.x + ((y - p1.y) * slope);
			T color = color1 + ((color2 - color1) * ((y - p1.y) / ydiff));
			SetPixel(x, y, color);
		}
	}
}


#endif // RASTERIZER_H