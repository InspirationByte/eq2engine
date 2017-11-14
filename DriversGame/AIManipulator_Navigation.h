//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_NAVIGATOR_H
#define AIMANIPULATOR_NAVIGATOR_H

#include "AIHandling.h"
#include "level.h"

class CCar;

class CAINavigationManipulator
{
public:
	CAINavigationManipulator();

	void UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);

	void SetPath(pathFindResult_t& newPath, const Vector3D& searchPos);

	Vector3D GetAdvancedPointByDist(int& startSeg, float distFromSegment);
	int FindSegmentByPosition(const Vector3D& pos, float distToSegment);

	// the position driver needs reach to
	Vector3D			m_driveTarget;
	Vector3D			m_driveTargetVelocity;
	CEqCollisionObject*	m_excludeColl;

protected:
	pathFindResult_t	m_path;
	int					m_pathPointIdx;

	bool				m_init;

	float				m_timeToUpdatePath;

	Vector3D			m_lastSuccessfulSearchPos;
	int					m_searchFails;
};

#endif // AIMANIPULATOR_NAVIGATOR_H