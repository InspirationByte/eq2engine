//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Axis-aligned rectangle (2D bounding box)
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <class T>
struct TAARectangle
{
	TVec2D<T> vleftTop;
	TVec2D<T> vrightBottom;

	TAARectangle()
	{
		Reset();
	}

	template <class T2>
	TAARectangle( const TAARectangle<T2>& rect)
	{
		vleftTop.x = (T)rect.vleftTop.x;
		vleftTop.y = (T)rect.vleftTop.y;
		vrightBottom.x = (T)rect.vrightBottom.x;
		vrightBottom.y = (T)rect.vrightBottom.y;
	}

	template <class T2>
	TAARectangle( T2 fX1, T2 fY1, T2 fX2, T2 fY2)
	{
		vleftTop.x = (T)fX1;
		vleftTop.y = (T)fY1;
		vrightBottom.x = (T)fX2;
		vrightBottom.y = (T)fY2;
	}

	template <class T2>
	TAARectangle( const TVec2D<T2>& leftTop, const TVec2D<T2>& rightBottom )
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

	void AddVertices(const TVec2D<T>* v, int numVertices)
	{
		for (int i = 0; i < numVertices; i++)
			AddVertex(v[i]);
	}

	void Reset()
	{
		vleftTop		= TVec2D<T>((T)F_INFINITY);
		vrightBottom	= TVec2D<T>(-(T)F_INFINITY);
	}

	// FIXME: this needs to be somehow removed
	void Fix()
	{
		TVec2D<T> lt(vleftTop);
		TVec2D<T> rb(vrightBottom);

		Reset();

		AddVertex(lt);
		AddVertex(rb);
	}

	void Expand(T value)
	{
		vrightBottom += value;
		vleftTop -= value;
	}

	void Expand(const TVec2D<T>& value)
	{
		vrightBottom += value;
		vleftTop -= value;
	}

    const TVec2D<T>& GetLeftTop() const
	{
		return vleftTop;
	}

	const TVec2D<T>& GetRightBottom() const
	{
		return vrightBottom;
	}

	TVec2D<T> GetLeftBottom() const
	{
		return Vector2D(vleftTop.x, vrightBottom.y);
	}

	TVec2D<T> GetRightTop() const
	{
		return Vector2D(vrightBottom.x, vleftTop.y);
	}

    TVec2D<T> GetCenter() const
	{
		return (vleftTop + vrightBottom) / static_cast<T>(2);
	}

	TVec2D<T> GetVertex(int index) const
	{
		return TVec2D<T>(index & 1 ? vrightBottom.x : vleftTop.x,
			index & 2 ? vrightBottom.y : vleftTop.y);
	}

	TVec2D<T> GetSize() const
	{
		return vrightBottom - vleftTop;
	}

	TAARectangle<T> GetTopVertical(float sizePercent) const
	{
		return TAARectangle<T>(vleftTop, lerp(vleftTop, vrightBottom, TVec2D<T>(1.0f, sizePercent)));
	}

	TAARectangle<T> GetBottomVertical(float sizePercent) const
	{
		return TAARectangle<T>(lerp(vrightBottom, vleftTop, TVec2D<T>(1.0f, sizePercent), vrightBottom));
	}

	TAARectangle<T> GetLeftHorizontal(float sizePercent) const
	{
		return TAARectangle<T>(vleftTop, lerp(vleftTop, vrightBottom, TVec2D<T>(sizePercent, 1.0f)));
	}

	TAARectangle<T> GetRightHorizontal(float sizePercent) const
	{
		return TAARectangle<T>(lerp(vrightBottom, vleftTop, TVec2D<T>(sizePercent, 1.0f), vrightBottom));
	}

	TVec2D<T> ClampPointInRectangle(const Vector2D &point) const
	{
		return clamp(point, vleftTop, vrightBottom);
	}

	TAARectangle<T> GetRectangleIntersectionDiff(const TAARectangle<T> &anotherRect) const
	{
		Vector2D tempLT = ClampPointInRectangle(Vector2D(anotherRect.vleftTop.x,anotherRect.vleftTop.y));
		Vector2D tempRB = ClampPointInRectangle(Vector2D(anotherRect.vrightBottom.x,anotherRect.vrightBottom.y));

		TAARectangle pRect(vleftTop.x - tempLT.x,vleftTop.y - tempLT.y,vrightBottom.x - tempRB.x,vrightBottom.y - tempRB.y);

		return pRect;
	}

	bool Contains(const TVec2D<T>& point) const
	{
		return point.x >= vleftTop.x && point.x <= vrightBottom.x 
			&& point.y >= vleftTop.y && point.y <= vrightBottom.y;
	}

	// warning, this is a size-dependent!
	bool FullyInside(const TAARectangle<T>& box, T tolerance = 0) const
	{
		if (box.vleftTop >= vleftTop - tolerance && box.vleftTop <= vrightBottom + tolerance &&
			box.vrightBottom <= vrightBottom + tolerance && box.vrightBottom >= vleftTop - tolerance)
			return true;

		return false;
	}

	bool Intersects(const TAARectangle<T> &anotherRect) const
	{
		if(( vrightBottom.x < anotherRect.vleftTop.x) || (vleftTop.x > anotherRect.vrightBottom.x ) )
			return false;

		if(( vrightBottom.y < anotherRect.vleftTop.y) || (vleftTop.y > anotherRect.vrightBottom.y ) )
			return false;

		return true;
	}

	bool IntersectsRay(const TVec2D<T>& rayStart, const TVec2D<T>& rayDir, T& tnear, T& tfar) const
	{
		TVec2D<T> T_1, T_2; // vectors to hold the T-values for every direction
		T t_near = -static_cast<T>(F_INFINITY);
		T t_far = static_cast<T>(F_INFINITY);

		for (int i = 0; i < 2; i++)
		{
			if (rayDir[i] == static_cast<T>(0))
			{
				// ray parallel to planes in this direction
				if ((rayStart[i] < vleftTop[i]) || (rayStart[i] > vrightBottom[i]))
					return false; // parallel AND outside box : no intersection possible
			}
			else
			{
				const float oneByRayDir = 1.0f / rayDir[i];

				// ray not parallel to planes in this direction
				T_1[i] = (vleftTop[i] - rayStart[i]) * oneByRayDir;
				T_2[i] = (vrightBottom[i] - rayStart[i]) * oneByRayDir;

				if (T_1[i] > T_2[i])
					QuickSwap(T_1, T_2);

				if (T_1[i] > t_near)
					t_near = T_1[i];

				if (T_2[i] < t_far)
					t_far = T_2[i];

				if ((t_near > t_far) || (t_far < static_cast<T>(0)))
					return false;
			}
		}

		tnear = t_near;
		tfar = t_far;

		return true;
	}

	bool IntersectsSphere(const TVec2D<T>& center, T radius) const
	{
		T dmin = static_cast<T>(0);

		for (int i = 0; i < 2; ++i)
		{
			if (center[i] < vleftTop[i])
			{
				const T cmin = center[i] - vleftTop[i];
				dmin += M_SQR(cmin);
			}
			else if (center[i] > vrightBottom[i])
			{
				const T cmax = center[i] - vrightBottom[i];
				dmin += M_SQR(cmax);
			}
		}

		return dmin <= M_SQR(radius);
	}

	// modifiers
    void FlipX()
	{
        QuickSwap(vleftTop.x, vrightBottom.x);
	}

	void FlipY()
	{
        QuickSwap(vleftTop.y, vrightBottom.y);
	}
};

typedef TAARectangle<float>		AARectangle;
typedef TAARectangle<int>		IAARectangle;