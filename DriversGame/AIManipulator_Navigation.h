//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI navigation manipulator
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATOR_NAVIGATOR_H
#define AIMANIPULATOR_NAVIGATOR_H

#include "AIHandling.h"
#include "road_defs.h"

class CCar;
class CEqCollisionObject;

class CAINavigationManipulator
{
public:
	CAINavigationManipulator();

	void				Setup(const Vector3D&driveTargetPos, const Vector3D& targetVelocity, CEqCollisionObject* excludeCollObj);
	void				UpdateAffector(ai_handling_t& handling, CCar* car, float fDt);
	void				ForceUpdatePath();

	Vector3D			m_outSteeringTargetPos;

protected:
	void		SetPath(pathFindResult_t& newPath, const Vector3D& searchPos, CCar* ignoreObj);

	Vector3D	GetAdvancedPointByDist(int& startSeg, float distFromSegment);
	Vector3D	GetPoint(int startSeg, float distFromSegment, int& outSeg, Vector3D& outDir, float& outSegLen, float& outDistFromSegmen);
	float		GetPointVelocityFactor(int startSeg, const Vector3D& position, const Vector3D& velocity);

	int			FindSegmentByPosition(const Vector3D& pos, float distToSegment);
	float		GetPathPercentage(const Vector3D& pos);
	float		GetNarrownessAt(const Vector3D& pos, float distToSegment);

	pathFindResult3D_t	m_path;

	Vector3D			m_lastSuccessfulSearchPos;
	Vector3D			m_targetSuccessfulSearchPos;

	// the position driver needs reach to
	Vector3D			m_driveTarget;
	Vector3D			m_driveTargetVelocity;
	CEqCollisionObject*	m_excludeColl;

	int					m_pathPointIdx;
	int					m_searchFails;
	float				m_timeToUpdatePath;
	bool				m_init;
};

#endif // AIMANIPULATOR_NAVIGATOR_H
