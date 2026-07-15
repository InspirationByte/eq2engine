#include "core/core_common.h"
#include "utils/KeyValues.h"

#include "math/Utility.h"
#include "math/Spline.h"

#include "scripting/esl.h"
#include "scripting/esl_luaref.h"
#include "scripting/esl_bind.h"

#include "sys_esl.h"
#include "sys_esl_math_utils.h"

EQSCRIPT_TYPE_BEGIN(Spline3dPoint)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_VAR(position)
	EQSCRIPT_BIND_VAR_EX_GET_SET(tangentBefore, GetTangentBefore, SetTangentBefore)
	EQSCRIPT_BIND_VAR_EX_GET_SET(tangentAfter, GetTangentAfter, SetTangentAfter)
	EQSCRIPT_BIND_VAR(time)
EQSCRIPT_TYPE_END

static int CSpline3d_AddPoint(CSpline3d& spline, const Spline3dPoint& point)
{
	return spline.m_points.append(point);
}

static int CSpline3d_RemovePoint(CSpline3d& spline, int idx)
{
	return spline.m_points.removeIndex(idx);
}

// stores object in keyvalues
static void	CSpline3d_ToKeyValues(const CSpline3d& spline, KVSection& section)
{
	KVSection& points = section.CreateSection("points");
	for (const Spline3dPoint& splinePt : spline.m_points)
	{
		KVSection& pointSec = points.CreateSection("point");
		pointSec.SetKey("position", Vector4D(splinePt.position, splinePt.time));
		pointSec.SetKey("tangentBefore", splinePt.tangents[TANGENT_BEFORE]);
		pointSec.SetKey("tangentAfter", splinePt.tangents[TANGENT_AFTER]);
	}
}

static bool	CSpline3d_FromKeyValues(CSpline3d& spline, const KVSection& section)
{
	for (const KVSection& pointSec : section.Get("points").Keys())
	{
		Spline3dPoint& splinePt = spline.m_points.append();

		const KVSection& posSec = pointSec.Get("position");
		posSec.GetValues(splinePt.position, splinePt.time);
		pointSec.Get("tangentBefore").GetValues(splinePt.tangents[TANGENT_BEFORE]);
		pointSec.Get("tangentAfter").GetValues(splinePt.tangents[TANGENT_AFTER]);
	}
	return true;
}

EQSCRIPT_TYPE_BEGIN(CSpline3d)
	EQSCRIPT_BIND_CONSTRUCTOR()

	EQSCRIPT_BIND_STATIC_FUNC("AddPoint", CSpline3d_AddPoint)
	EQSCRIPT_BIND_STATIC_FUNC("RemovePoint", CSpline3d_RemovePoint)

	EQSCRIPT_BIND_STATIC_FUNC("ToKeyValues", CSpline3d_ToKeyValues)
	EQSCRIPT_BIND_STATIC_FUNC("FromKeyValues", CSpline3d_FromKeyValues)

	EQSCRIPT_BIND_FUNC(Clear)

	EQSCRIPT_BIND_FUNC(SetLooped)
	EQSCRIPT_BIND_FUNC(IsLooped)

	EQSCRIPT_BIND_FUNC(GetDuration)
	EQSCRIPT_BIND_FUNC(GetLength)

	EQSCRIPT_BIND_FUNC(GetPointsCount)
	EQSCRIPT_BIND_FUNC(GetPoint)
	EQSCRIPT_BIND_FUNC(GetPointTime)
	EQSCRIPT_BIND_FUNC(GetPointDistance)
	EQSCRIPT_BIND_FUNC(GetTangent)

	EQSCRIPT_BIND_FUNC(PositionAtTime)
	EQSCRIPT_BIND_FUNC(TangentAtTime)
	EQSCRIPT_BIND_FUNC(DistanceAtTime)
	EQSCRIPT_BIND_FUNC(TimeAtDistance)

	EQSCRIPT_BIND_FUNC(UpdateDistances)
	EQSCRIPT_BIND_FUNC(PositionAtDistance)
	EQSCRIPT_BIND_FUNC(TangentAtDistance)

	EQSCRIPT_BIND_FUNC(GetSegmentLength)
	EQSCRIPT_BIND_FUNC(SegmentIndexByLocalTime)
	EQSCRIPT_BIND_FUNC(SegmentIndexByDistance)
EQSCRIPT_TYPE_END


static esl::Any<2> L_LineIntersectsLine2D(const esl::ScriptState& state, const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE)
{
	Vector2D isectPoint;
	const bool result = LineIntersectsLine2D(lAB, lAE, lBB, lBE, isectPoint);
	state.PushValue(result);
	state.PushValue(isectPoint);
	return {};
}

static esl::Any<2> L_LineSegIntersectsLineSeg2D(const esl::ScriptState& state, const Vector2D& lAB, const Vector2D& lAE, const Vector2D& lBB, const Vector2D& lBE)
{
	Vector2D isectPoint;
	const bool result = LineSegIntersectsLineSeg2D(lAB, lAE, lBB, lBE, isectPoint);
	state.PushValue(result);
	state.PushValue(isectPoint);
	return {};
}

static esl::Any<2> L_LineSegIntersectsCircle2D(const esl::ScriptState& state, const Vector2D& lB, const Vector2D& lE, const Vector2D& center, float radius)
{
	FixedArray<Vector2D, 2> isectPoints;
	const bool result = LineSegIntersectsCircle2D(lB, lE, center, radius, isectPoints);
	state.PushValue(result);
	if (result)
	{
		esl::LuaTable isectPointsTbl = state.CreateTable();
		for (int i = 0; i < isectPoints.numElem(); ++i)
			isectPointsTbl.Set(i + 1, isectPoints[i]);

		state.PushValue(isectPointsTbl);
	}
	else
		state.PushValue(nullptr);
	return {};
}

bool eslSysMathUtilsInit(const esl::ScriptState& state)
{
	state.RegisterClass<Spline3dPoint>();
	state.RegisterClass<CSpline3d>();

	state.SetGlobal("LineIntersectsLine2D", EQSCRIPT_CFUNC(L_LineIntersectsLine2D));
	state.SetGlobal("LineSegIntersectsLineSeg2D", EQSCRIPT_CFUNC(L_LineSegIntersectsLineSeg2D));
	state.SetGlobal("LineSegIntersectsCircle2D", EQSCRIPT_CFUNC(L_LineSegIntersectsCircle2D));

	state.SetGlobal("AngleDiff", EQSCRIPT_CFUNC(AngleDiff));
	state.SetGlobal("AnglesDiff", EQSCRIPT_CFUNC(AnglesDiff));

	return true;
}