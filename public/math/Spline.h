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

enum ESplineTangent : int
{
	TANGENT_BEFORE,
	TANGENT_AFTER,
	TANGENT_COUNT,
};

struct Spline3dPoint
{
	//Quaternion	rotation;
	Vector3D	position;
	Vector3D	tangents[TANGENT_COUNT];
	float		time{ -1.0f };
};

class CSpline3d
{
public:
	void					SetLooped(bool loop) { m_loop = loop; }
	bool					IsLooped() const { return m_loop; }

	float					GetDuration() const { return m_duration; }
	float					GetLength() const { return m_distances.numElem() ? m_distances.back().y : 0.0f; }

	// raw points
	ArrayCRef<Spline3dPoint>	GetPoints() const { return m_points; }

	int						GetPointsCount() const { return m_points.numElem(); }
	const Spline3dPoint&	GetPoint(int idx) const { return m_points[idx]; }
	const float				GetPointTime(int idx) const { return m_points[idx].time; }
	const float				GetPointDistance(int idx) const { return m_distances[idx * m_stepsPerSegment].y; }
	const Vector3D&			GetTangent(int idx, ESplineTangent tangentId) const { return m_points[idx].tangents[tangentId]; }

	// spline samplers
	Vector3D				PositionAtTime(float time) const;
	Vector3D				TangentAtTime(float time) const;
	float					DistanceAtTime(float time) const;
	float					TimeAtDistance(float time) const;

	// distance utils
	void					UpdateDistances();
	Vector3D				PositionAtDistance(float dist) const;
	Vector3D				TangentAtDistance(float dist) const;

	// segments
	float					GetSegmentLength(int segIdx) const;
	int						SegmentIndexByLocalTime(float time) const;
	int						SegmentIndexByDistance(float dist) const;


	Array<Spline3dPoint>	m_points{ PP_SL };
	bool					m_loop{ false };

protected:
	int						GetSegmentIndexAndLocalTime(float time, float& localTime) const;
protected:
	Array<Vector2D>			m_distances{ PP_SL };
	float					m_duration{ 0.0f };
	int						m_stepsPerSegment{ 5 };
};

Vector3D Spline3DPositionAtLocalTime(ArrayCRef<Spline3dPoint> points, int startPtIdx, float t);
Vector3D Spline3DTangentAtLocalTime(ArrayCRef<Spline3dPoint> points, int startPtIdx, float t);