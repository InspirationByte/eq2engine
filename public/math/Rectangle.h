//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Font container
//				Uses engine to load, draw fonts//////////////////////////////////////////////////////////////////////////////////

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "Vector.h"
#include "math_util.h"
#include "coord.h"

template <class T, uint32 TMAX>
struct TRectangle
{
	TVec2D<T> vleftTop, vrightBottom;

	TRectangle()
	{
		Reset();
	}

	TRectangle( T fX1, T fY1, T fX2, T fY2)
	{
		vleftTop.x = fX1;
		vleftTop.y = fY1;
		vrightBottom.x = fX2;
		vrightBottom.y = fY2;
	}

	TRectangle( const TVec2D<T>& leftTop, const TVec2D<T>& rightBottom )
	{
		vleftTop.x = leftTop.x;
		vleftTop.y = leftTop.y;
		vrightBottom.x = rightBottom.x;
		vrightBottom.y = rightBottom.y;
	}

	void AddVertex(const TVec2D<T> &vert)
	{
		POINT_TO_RECT(vert, vleftTop, vrightBottom);
	}

	void Reset()
	{
		vleftTop		= TVec2D<T>((T)TMAX);
		vrightBottom	= TVec2D<T>(-(T)TMAX);
	}

	void Fix()
	{
		Vector2D lt = vleftTop;
		Vector2D rb = vrightBottom;

		Reset();

		AddVertex(lt);
		AddVertex(rb);
	}

    const TVec2D<T>& GetLeftTop() const
	{
		return vleftTop;
	}

	TVec2D<T> GetLeftBottom() const
	{
		return Vector2D(vleftTop.x, vrightBottom.y);
	}

	TVec2D<T> GetRightTop() const
	{
		return Vector2D(vrightBottom.x, vleftTop.y);
	}

    const TVec2D<T>& GetRightBottom() const
	{
		return vrightBottom;
	}

    TVec2D<T> GetCenter() const
	{
		return (vleftTop + vrightBottom)*0.5f;
	}

	bool IsInRectangle(const TVec2D<T> &point) const
	{
		return ((point.x >= vleftTop.x) && (point.x <= vrightBottom.x) && (point.y >= vleftTop.y) && (point.y <= vrightBottom.y));
	}

	TVec2D<T> ClampPointInRectangle(const Vector2D &point) const
	{
		return clamp(point, vleftTop, vrightBottom);
	}

	TRectangle<T, TMAX> GetRectangleIntersectionDiff(const TRectangle<T, TMAX> &anotherRect) const
	{
		Vector2D tempLT = ClampPointInRectangle(Vector2D(anotherRect.vleftTop.x,anotherRect.vleftTop.y));
		Vector2D tempRB = ClampPointInRectangle(Vector2D(anotherRect.vrightBottom.x,anotherRect.vrightBottom.y));

		TRectangle pRect(vleftTop.x - tempLT.x,vleftTop.y - tempLT.y,vrightBottom.x - tempRB.x,vrightBottom.y - tempRB.y);

		return pRect;
	}

	bool IsIntersectsRectangle(const TRectangle<T, TMAX> &anotherRect) const
	{
		if(( vrightBottom.x < anotherRect.vleftTop.x) || (vleftTop.x > anotherRect.vrightBottom.x ) )
			return false;

		if(( vrightBottom.y < anotherRect.vleftTop.y) || (vleftTop.y > anotherRect.vrightBottom.y ) )
			return false;

		return true;
	}

	bool IsFullyInside(const TRectangle<T, TMAX> &anotherRect) const
	{
		if(	anotherRect.vleftTop >= vleftTop && anotherRect.vleftTop <= vrightBottom &&
			anotherRect.vrightBottom <= vrightBottom && anotherRect.vrightBottom >= vleftTop)
			return true;

		return false;
	}

	TVec2D<T> GetVertex(int index) const
	{
		return TVec2D<T>(	index & 1 ? vrightBottom.x : vleftTop.x,
							index & 2 ? vrightBottom.y : vleftTop.y);
	}

	TVec2D<T> GetSize() const
	{
		return vrightBottom - vleftTop;
	}
};

typedef TRectangle<float, (int)V_MAX_COORD>	Rectangle_t;
typedef TRectangle<int, (int)V_MAX_COORD>	IRectangle;
typedef TRectangle<int, 32767>			FRectangle;

#endif // RECTANGLE_H
