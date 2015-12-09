//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap UV chart polygon as a convex compound polygon set
//////////////////////////////////////////////////////////////////////////////////

#ifndef CHART_UV_POLY_H
#define CHART_UV_POLY_H

#include "utils/DkList.h"
#include "math/DkMath.h"

struct lmchart_convexpoly_t
{
	DkList<Vector2D>	verts;
	DkList<Vector2D>	verts_oriented;

	Matrix2x2			orient;
	Vector2D			center;
};

void SetChartOrientation(lmchart_convexpoly_t* poly, float fAngle);

bool ChartCollide(	const lmchart_convexpoly_t& Convex0, const Vector2D& PA,
 					const lmchart_convexpoly_t& Convex1, const Vector2D& PB,
 					Vector2D& N, float& t);

#endif // CHART_UV_POLY_H