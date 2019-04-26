//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_NAVIGATOR_H
#define AIMANIPULATOR_NAVIGATOR_H

#include "AIHandling.h"
#include "road_defs.h"

class CCar;
class CEqCollisionObject;

struct pathFindResult3D_t
{
	Vector3D			start;
	Vector3D			end;

	DkList<Vector4D>	points;	// w is narrowness factor

	void InitFrom(pathFindResult_t& path, CEqCollisionObject* ignore);
};

class CAINavigationManipulator
{
public:
	CAINavigationManipulator();

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	void SetPath(pathFindResult_t& newPath, const Vector3D& searchPos, CCar* ignoreObj);

	Vector3D	GetAdvancedPointByDist(int& startSeg, float distFromSegment);
	Vector3D	GetPoint(int startSeg, float distFromSegment, int& outSeg, Vector3D& outDir, float& outSegLen, float& outDistFromSegmen);
	float		GetPointVelocityFactor(int startSeg, const Vector3D& position, const Vector3D& velocity);


	int FindSegmentByPosition(const Vector3D& pos, float distToSegment);
	float GetPathPercentage(const Vector3D& pos);

	float GetNarrownessAt(const Vector3D& pos, float distToSegment);

	void ForceUpdatePath();

	// the position driver needs reach to
	Vector3D			m_driveTarget;
	Vector3D			m_driveTargetVelocity;
	CEqCollisionObject*	m_excludeColl;

protected:
	pathFindResult3D_t	m_path;
	int					m_pathPointIdx;

	bool				m_init;

	float				m_timeToUpdatePath;

	Vector3D			m_lastSuccessfulSearchPos;
	Vector3D			m_targetSuccessfulSearchPos;

	int					m_searchFails;
};

#endif // AIMANIPULATOR_NAVIGATOR_H
