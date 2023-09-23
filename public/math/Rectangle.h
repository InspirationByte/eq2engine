//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Axis-aligned rectangle (2D bounding box)
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template <class T>
struct TAARectangle
{
	TVec2D<T> leftTop;
	TVec2D<T> rightBottom;

	TAARectangle();

	template <class T2>
	TAARectangle(const TAARectangle<T2>& rect);

	template <class T2>
	TAARectangle(T2 fX1, T2 fY1, T2 fX2, T2 fY2);

	template <class T2>
	TAARectangle(const TVec2D<T2>& leftTop, const TVec2D<T2>& rightBottom);

	void				AddVertex(const TVec2D<T>& p);
	void				AddVertices(const TVec2D<T>* v, int numVertices);

	void				Reset();
	void				Fix();
	void				Expand(T value);
	void				Expand(const TVec2D<T>& value);

	const TVec2D<T>&	GetLeftTop() const;
	const TVec2D<T>&	GetRightBottom() const;
	TVec2D<T>			GetLeftBottom() const;
	TVec2D<T>			GetRightTop() const;

    TVec2D<T>			GetCenter() const { return (leftTop + rightBottom) / static_cast<T>(2); }
	TVec2D<T>			GetSize() const { return rightBottom - leftTop; }

	TVec2D<T>			GetVertex(int index) const;

	TAARectangle<T>		GetTopVertical(float sizePercent) const;
	TAARectangle<T>		GetBottomVertical(float sizePercent) const;
	TAARectangle<T>		GetLeftHorizontal(float sizePercent) const;
	TAARectangle<T>		GetRightHorizontal(float sizePercent) const;

	TVec2D<T>			ClampPointInRectangle(const TVec2D<T>& point) const;
	TAARectangle<T>		GetRectangleIntersectionDiff(const TAARectangle<T>& anotherRect) const;

	bool				Contains(const TVec2D<T>& point) const;

	// warning, this is a size-dependent!
	bool				FullyInside(const TAARectangle<T>& box, T tolerance = 0) const;

	bool				Intersects(const TAARectangle<T>& anotherRect) const;
	bool				IntersectsRay(const TVec2D<T>& rayStart, const TVec2D<T>& rayDir, T& tnear, T& tfar) const;
	bool				IntersectsSphere(const TVec2D<T>& center, T radius) const;

	// modifiers
	void				FlipX();
	void				FlipY();
};

template <class T>
TAARectangle<T>::TAARectangle()
{
	Reset();
}

template <class T>
template <class T2>
TAARectangle<T>::TAARectangle(const TAARectangle<T2>& rect)
{
	leftTop.x = (T)rect.leftTop.x;
	leftTop.y = (T)rect.leftTop.y;
	rightBottom.x = (T)rect.rightBottom.x;
	rightBottom.y = (T)rect.rightBottom.y;
}

template <class T>
template <class T2>
TAARectangle<T>::TAARectangle(T2 fX1, T2 fY1, T2 fX2, T2 fY2)
{
	leftTop.x = (T)fX1;
	leftTop.y = (T)fY1;
	rightBottom.x = (T)fX2;
	rightBottom.y = (T)fY2;
}

template <class T>
template <class T2>
TAARectangle<T>::TAARectangle(const TVec2D<T2>& leftTop, const TVec2D<T2>& rightBottom)
	: leftTop(TVec2D<T>((T)leftTop.x, (T)leftTop.y))
	, rightBottom(TVec2D<T>((T)rightBottom.x, (T)rightBottom.y))
{
}

template <class T>
void TAARectangle<T>::AddVertex(const TVec2D<T>& p)
{
	if (p.x < leftTop.x)
		leftTop.x = p.x;
	if (p.x > rightBottom.x)
		rightBottom.x = p.x;

	if (p.y < leftTop.y)
		leftTop.y = p.y;
	if (p.y > rightBottom.y)
		rightBottom.y = p.y;
}

template <class T>
void TAARectangle<T>::AddVertices(const TVec2D<T>* v, int numVertices)
{
	for (int i = 0; i < numVertices; i++)
		AddVertex(v[i]);
}

template <class T>
void TAARectangle<T>::Reset()
{
	leftTop = TVec2D<T>((T)F_INFINITY);
	rightBottom = TVec2D<T>(-(T)F_INFINITY);
}

template <class T>
void TAARectangle<T>::Fix()
{
	TVec2D<T> lt(leftTop);
	TVec2D<T> rb(rightBottom);

	Reset();

	AddVertex(lt);
	AddVertex(rb);
}

template <class T>
void TAARectangle<T>::Expand(T value)
{
	rightBottom += value;
	leftTop -= value;
}

template <class T>
void TAARectangle<T>::Expand(const TVec2D<T>& value)
{
	rightBottom += value;
	leftTop -= value;
}

template <class T>
const TVec2D<T>& TAARectangle<T>::GetLeftTop() const
{
	return leftTop;
}

template <class T>
const TVec2D<T>& TAARectangle<T>::GetRightBottom() const
{
	return rightBottom;
}

template <class T>
TVec2D<T> TAARectangle<T>::GetLeftBottom() const
{
	return TVec2D<T>(leftTop.x, rightBottom.y);
}

template <class T>
TVec2D<T> TAARectangle<T>::GetRightTop() const
{
	return TVec2D<T>(rightBottom.x, leftTop.y);
}

template <class T>
TVec2D<T> TAARectangle<T>::GetVertex(int index) const
{
	return TVec2D<T>(index & 1 ? rightBottom.x : leftTop.x,
		index & 2 ? rightBottom.y : leftTop.y);
}

template <class T>
TAARectangle<T> TAARectangle<T>::GetTopVertical(float sizePercent) const
{
	return TAARectangle<T>(leftTop, lerp(leftTop, rightBottom, TVec2D<T>(1.0f, sizePercent)));
}

template <class T>
TAARectangle<T> TAARectangle<T>::GetBottomVertical(float sizePercent) const
{
	return GetTopVertical(1.0 - sizePercent);
}

template <class T>
TAARectangle<T> TAARectangle<T>::GetLeftHorizontal(float sizePercent) const
{
	return TAARectangle<T>(leftTop, lerp(leftTop, rightBottom, TVec2D<T>(sizePercent, 1.0f)));
}

template <class T>
TAARectangle<T> TAARectangle<T>::GetRightHorizontal(float sizePercent) const
{
	return GetLeftHorizontal(1.0 - sizePercent);
}

template <class T>
TVec2D<T> TAARectangle<T>::ClampPointInRectangle(const TVec2D<T>& point) const
{
	return clamp(point, leftTop, rightBottom);
}

template <class T>
TAARectangle<T> TAARectangle<T>::GetRectangleIntersectionDiff(const TAARectangle<T>& anotherRect) const
{
	TVec2D<T> tempLT = ClampPointInRectangle(TVec2D<T>(anotherRect.leftTop.x, anotherRect.leftTop.y));
	TVec2D<T> tempRB = ClampPointInRectangle(TVec2D<T>(anotherRect.rightBottom.x, anotherRect.rightBottom.y));

	TAARectangle pRect(leftTop.x - tempLT.x, leftTop.y - tempLT.y, rightBottom.x - tempRB.x, rightBottom.y - tempRB.y);

	return pRect;
}

template <class T>
bool TAARectangle<T>::Contains(const TVec2D<T>& point) const
{
	return point.x >= leftTop.x && point.x <= rightBottom.x
		&& point.y >= leftTop.y && point.y <= rightBottom.y;
}

template <class T>
bool TAARectangle<T>::FullyInside(const TAARectangle<T>& box, T tolerance) const
{
	if (box.leftTop >= leftTop - tolerance && box.leftTop <= rightBottom + tolerance &&
		box.rightBottom <= rightBottom + tolerance && box.rightBottom >= leftTop - tolerance)
		return true;

	return false;
}

template <class T>
bool TAARectangle<T>::Intersects(const TAARectangle<T>& anotherRect) const
{
	if ((rightBottom.x < anotherRect.leftTop.x) || (leftTop.x > anotherRect.rightBottom.x))
		return false;

	if ((rightBottom.y < anotherRect.leftTop.y) || (leftTop.y > anotherRect.rightBottom.y))
		return false;

	return true;
}

template <class T>
bool TAARectangle<T>::IntersectsRay(const TVec2D<T>& rayStart, const TVec2D<T>& rayDir, T& tnear, T& tfar) const
{
	TVec2D<T> T_1, T_2; // vectors to hold the T-values for every direction
	T t_near = -static_cast<T>(F_INFINITY);
	T t_far = static_cast<T>(F_INFINITY);

	for (int i = 0; i < 2; i++)
	{
		if (rayDir[i] == static_cast<T>(0))
		{
			// ray parallel to planes in this direction
			if ((rayStart[i] < leftTop[i]) || (rayStart[i] > rightBottom[i]))
				return false; // parallel AND outside box : no intersection possible
		}
		else
		{
			const float oneByRayDir = 1.0f / rayDir[i];

			// ray not parallel to planes in this direction
			T_1[i] = (leftTop[i] - rayStart[i]) * oneByRayDir;
			T_2[i] = (rightBottom[i] - rayStart[i]) * oneByRayDir;

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

template <class T>
bool TAARectangle<T>::IntersectsSphere(const TVec2D<T>& center, T radius) const
{
	T dmin = static_cast<T>(0);

	for (int i = 0; i < 2; ++i)
	{
		if (center[i] < leftTop[i])
		{
			const T cmin = center[i] - leftTop[i];
			dmin += M_SQR(cmin);
		}
		else if (center[i] > rightBottom[i])
		{
			const T cmax = center[i] - rightBottom[i];
			dmin += M_SQR(cmax);
		}
	}

	return dmin <= M_SQR(radius);
}

template <class T>
void TAARectangle<T>::FlipX()
{
	QuickSwap(leftTop.x, rightBottom.x);
}

template <class T>
void TAARectangle<T>::FlipY()
{
	QuickSwap(leftTop.y, rightBottom.y);
}

typedef TAARectangle<float>		AARectangle;
typedef TAARectangle<int>		IAARectangle;