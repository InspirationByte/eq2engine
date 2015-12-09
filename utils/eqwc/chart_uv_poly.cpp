//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap UV chart polygon as a convex compound polygon set
//////////////////////////////////////////////////////////////////////////////////

#include "chart_uv_poly.h"

void SetChartOrientation(lmchart_convexpoly_t* poly, float fAngle)
{
	poly->orient = rotate2(fAngle);
	poly->verts_oriented.resize( poly->verts.numElem() );

	for (int i = 0; i < poly->verts.numElem(); i++)
	{
		poly->verts_oriented[i] = poly->orient* poly->verts[i];
	}
}

inline Vector2D Cross(float s, const Vector2D& a)
{
    return Vector2D(-s * a.y, s * a.x);
}

inline float CrossMag(const Vector2D& u, const Vector2D& v)
{
    return u.x * v.y - u.y * v.x;
}

Vector2D MatDot( const Vector2D& v, const Matrix2x2& M )
{
	Vector2D T(0,0);
	T.x = v.x * M.rows[0].x + v.y * M.rows[1].x;
	T.y = v.x * M.rows[0].y + v.y * M.rows[1].y;

	return T;
}

inline Matrix2x2 MatInvMul(const Matrix2x2& mat, const Matrix2x2& M)
{
    Matrix2x2 T;

    T.rows[0].x = mat.rows[0].x * M.rows[0].x + mat.rows[0].y * M.rows[0].y;
    T.rows[1].x = mat.rows[1].x * M.rows[0].x + mat.rows[1].y * M.rows[0].y;
    T.rows[0].y = mat.rows[0].x * M.rows[1].x + mat.rows[0].y * M.rows[1].y;
    T.rows[1].y = mat.rows[1].x * M.rows[1].x + mat.rows[1].y * M.rows[1].y;

    return T;
}

void ProjectConvex( const lmchart_convexpoly_t& Convex, const Vector2D& Axis, float& pmin, float& pmax )
{
	ASSERT( Convex.verts.numElem()>0 );
	pmin = pmax = dot(Convex.verts[0], Axis);

	for( size_t i = 1; i < Convex.verts.numElem(); ++i )
	{
		const float proj = dot(Convex.verts[i], Axis);

		pmin = min(pmin,proj);
		pmax = max(pmax,proj);
	}
}

bool IntervalIntersect(const lmchart_convexpoly_t& Convex0,
                       const lmchart_convexpoly_t& Convex1,
                       const Vector2D& xAxis,
                       const Vector2D& xOffset, const Matrix2x2& xOrient,
                       float& taxis)
{
	float min0 = 0, max0 = 0;
	float min1 = 0, max1 = 0;

	ProjectConvex(Convex0, MatDot(xAxis,xOrient), min0, max0);
	ProjectConvex(Convex1, xAxis, min1, max1);

	float h = dot(xOffset,xAxis);
	min0 += h;
	max0 += h;

	float d0 = min0 - max1;
	float d1 = min1 - max0;

	if (d0 > 0.0f || d1 > 0.0f)
	{
		return false;
	}
	else
	{
		taxis = (d0 > d1)? d0 : d1;
		return true;
	}
}

bool FindMTD(Vector2D* xAxis, float* taxis, int iNumAxes, Vector2D& N, float& t)
{
	int mini = -1;
	t = 0.0f;
	N = vec2_zero;

	for(int i = 0; i < iNumAxes; i ++)
	{
		float n = length(xAxis[i]);
		xAxis[i] = normalize(xAxis[i]);
		taxis[i] /= n;

		if (taxis[i] > t || mini == -1)
		{
			mini = i;
			t = taxis[i];
			N = xAxis[i];
		}
	}

	return (mini != -1);
}

bool ChartCollide(	const lmchart_convexpoly_t& Convex0, const Vector2D& PA,
 					const lmchart_convexpoly_t& Convex1, const Vector2D& PB,
 					Vector2D& N, float& t)
{
	if (!Convex0.verts.numElem() || !Convex1.verts.numElem())
		return false;

	Matrix2x2 xOrient = MatInvMul(Convex0.orient, Convex1.orient);
	Vector2D xOffset = MatDot((PA - PB), Convex1.orient);
	//Vector xVel    = (VA - VB) ^ OB;

	// All the separation axes
	Vector2D  xAxis[64];
	float   taxis[64];
	int     iNumAxes=0;

	for (int i = 0; i < (int)Convex0.verts.numElem(); i++)
	{
		Vector2D E0 = Convex0.verts[i];
		Vector2D E1 = Convex0.verts[(i + 1)%Convex0.verts.numElem()];
		xAxis[iNumAxes] = xOrient*Cross(1.0f , (E0 - E1));

		if(!IntervalIntersect(Convex0, Convex1, xAxis[iNumAxes], xOffset, xOrient, taxis[iNumAxes]))
		{
		    return false;
		}

		iNumAxes++;
	}

	for (int i = 0; i < (int)Convex1.verts.numElem(); i++)
	{
		Vector2D E0 = Convex1.verts[i];
		Vector2D E1 = Convex1.verts[(i + 1)%Convex1.verts.numElem()];
		xAxis[iNumAxes] = Cross(1.0f , (E0 - E1));

		if(!IntervalIntersect(Convex0, Convex1, xAxis[iNumAxes], xOffset, xOrient, taxis[iNumAxes]))
		{
			return false;
		}
		
		iNumAxes++;
	}

	if(!FindMTD(xAxis, taxis, iNumAxes, N, t))
		return false;

	if(N * xOffset < Vector2D(0.0f))
		N = -N;

	N = Convex1.orient*N;

	return true;
}