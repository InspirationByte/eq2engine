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

template <class T, uint32 TMAX>
struct TRectangle
{
	TVec2D<T> vleftTop, vrightBottom;

	TRectangle()
	{
		Reset();
	}

	template <class T2, uint32 TMAX2>
	TRectangle( const TRectangle<T2,TMAX2>& rect)
	{
		vleftTop.x = (T)rect.vleftTop.x;
		vleftTop.y = (T)rect.vleftTop.y;
		vrightBottom.x = (T)rect.vrightBottom.x;
		vrightBottom.y = (T)rect.vrightBottom.y;
	}

	template <class T2>
	TRectangle( T2 fX1, T2 fY1, T2 fX2, T2 fY2)
	{
		vleftTop.x = (T)fX1;
		vleftTop.y = (T)fY1;
		vrightBottom.x = (T)fX2;
		vrightBottom.y = (T)fY2;
	}

	template <class T2>
	TRectangle( const TVec2D<T2>& leftTop, const TVec2D<T2>& rightBottom )
	{
		vleftTop.x = (T)leftTop.x;
		vleftTop.y = (T)leftTop.y;
		vrightBottom.x = (T)rightBottom.x;
		vrightBottom.y = (T)rightBottom.y;
	}

	void AddVertex(const TVec2D<T> &p)
	{
		if ( p.x < vleftTop.x )
			vleftTop.x = p.x;
		if ( p.x > vrightBottom.x )
			vrightBottom.x = p.x;

		if ( p.y < vleftTop.y )
			vleftTop.y = p.y;
		if ( p.y > vrightBottom.y )
			vrightBottom.y = p.y;
	}

	void Reset()
	{
		vleftTop		= TVec2D<T>((T)TMAX);
		vrightBottom	= TVec2D<T>(-(T)TMAX);
	}

	void Fix()
	{
		TVec2D<T> lt(vleftTop);
		TVec2D<T> rb(vrightBottom);

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

	TRectangle<T, TMAX> GetTopVertical(float sizePercent) const
	{
		return TRectangle<T, TMAX>(vleftTop, lerp(vleftTop, vrightBottom, TVec2D<T>(1.0f, sizePercent)));
	}

	TRectangle<T, TMAX> GetBottomVertical(float sizePercent) const
	{
		return TRectangle<T, TMAX>(lerp(vrightBottom, vleftTop, TVec2D<T>(1.0f, sizePercent), vrightBottom));
	}

	TRectangle<T, TMAX> GetLeftHorizontal(float sizePercent) const
	{
		return TRectangle<T, TMAX>(vleftTop, lerp(vleftTop, vrightBottom, TVec2D<T>(sizePercent, 1.0f)));
	}

	TRectangle<T, TMAX> GetRightHorizontal(float sizePercent) const
	{
		return TRectangle<T, TMAX>(lerp(vrightBottom, vleftTop, TVec2D<T>(sizePercent, 1.0f), vrightBottom));
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

	// modifiers
    void FlipX()
	{
        QuickSwap(vleftTop.x,vrightBottom.x);
	}

	void FlipY()
	{
        QuickSwap(vleftTop.y,vrightBottom.y);
	}
};

typedef TRectangle<float, (int)V_MAX_COORD>	Rectangle_t;
typedef TRectangle<int, (int)V_MAX_COORD>	IRectangle;
typedef TRectangle<int, 32767>			FRectangle;

#endif // RECTANGLE_H
