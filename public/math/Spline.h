//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: Spline functions
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// Catmull-Rom spline, based on http://iquilezles.org/www/articles/minispline/minispline.htm
// key format (for DIM == 1) is [t0,x0, t1,x1 ...]
// key format (for DIM == 2) is [t0,x0,y0, t1,x1,y1 ...]
// key format (for DIM == 3) is [t0,x0,y0,z0, t1,x1,y1,z1 ...]
template<int DIM>
void spline(const float* key, int num, float t, float* v)
{
	static float coefs[16] = {
		-1.0f, 2.0f,-1.0f, 0.0f,
		 3.0f,-5.0f, 0.0f, 2.0f,
		-3.0f, 4.0f, 1.0f, 0.0f,
		 1.0f,-1.0f, 0.0f, 0.0f
	};

	const int size = DIM + 1;

	const float tmin = key[0];
	const float tmax = key[(num - 1) * size];

	t = clamp(t, tmin, tmax);

	// find key
	int k = 0;
	while (key[k * size] < t)
		++k;

	if (!k)
		++k;

	const float key0 = key[(k - 1) * size];
	const float key1 = key[k * size];

	// interpolant
	const float h = (t - key0) / (key1 - key0);

	// init result
	for (int i = 0; i < DIM; i++)
		v[i] = 0.0f;

	// add basis functions
	for (int i = 0; i < 4; ++i)
	{
		const float* co = &coefs[4 * i];
		const float b = 0.5f * (((co[0] * h + co[1]) * h + co[2]) * h + co[3]);

		const int kn = clamp(k + i - 2, 0, num - 1);
		for (int j = 0; j < DIM; j++)
			v[j] += b * key[kn * size + j + 1];
	}
}


class BezierSpline
{
public:
	enum PointMode : ubyte
	{
		FREE,
		ALIGNED,
		MIRRORED
	};

	// Quadratic
	static Vector3D		GetPoint(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, float t);

	// Quadratic
	static Vector3D		GetFirstDerivative(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, float t);

	// Cubic
	static Vector3D		GetPoint(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, const Vector3D& p3, float t);

	// Cubic
	static Vector3D		GetFirstDerivative(const Vector3D& p0, const Vector3D& p1, const Vector3D& p2, const Vector3D& p3, float t);

	BezierSpline() = default;
	virtual ~BezierSpline() = default;

	void		Reset();

	int			GetControlPointCount() const { return points.numElem(); }

	Vector3D	GetControlPoint(int index) const;
	void		SetControlPoint(int index, Vector3D point);

	PointMode	GetControlPointMode(int index) const;
	void		SetControlPointMode(int index, PointMode mode);

	Vector3D	GetPoint(float t) const;
	Vector3D	GetVelocity(float t) const;

protected:
	int			GetCurveCount() const;
	void		TimeToCurveIndex(float& t, int& i) const;
	void		EnforceMode(int index);

	Array<Vector3D>		points{ PP_SL };
	Array<PointMode>	pointModes{ PP_SL };
};