//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#ifndef PEDESTRIAN_H
#define PEDESTRIAN_H

#include "ControllableObject.h"
#include "Animating.h"
#include "EventFSM.h"

class CPedestrianAI : public CFSM_Base
{
	friend class CPedestrian;
public:
	CPedestrianAI(CPedestrian* host)
	{
		m_host = host;
		m_nextRoadTile = 0;

		m_prevRoadCell = nullptr;
		m_nextRoadCell = nullptr;

		m_prevDir = m_curDir = -1;
	}

	int					DoWalk(float fDt, EStateTransition transition);
	int					DoEscape(float fDt, EStateTransition transition);

	int					SearchDaWay(float fDt, EStateTransition transition);

	bool				GetNextPath(int dir);
	void				DetectEscape();

	int					m_curDir;
	int					m_prevDir;

	Vector3D			m_escapeDir;
	Vector3D			m_escapeFromPos;

	IVector2D			m_nextRoadTile;

	levroadcell_t*		m_nextRoadCell;
	levroadcell_t*		m_prevRoadCell;

	CPedestrian*		m_host;
};

//------------------------------------------------------

struct pedestrianConfig_t
{
	bool hasAI;
	EqString name;
	EqString model;
};

struct spritePedSegment_t
{
	int fromJoint;
	int toJoint;
	float width;
	float addLength;

	int atlasIdx;
};

class CPedestrian : 
	public CControllableGameObject,
	public CAnimatingEGF
{
	friend class CCar;
public:
	DECLARE_CLASS(CPedestrian, CControllableGameObject)

	CPedestrian();
	CPedestrian(pedestrianConfig_t* config);
	~CPedestrian();

	void				SetModelPtr(IEqModel* modelPtr);

	void				Precache();

	void				OnRemove();
	void				Spawn();

	void				ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const;

	void				Draw(int nRenderFlags);

	void				Simulate(float fDt);

	int					ObjType() const { return GO_PEDESTRIAN; }

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	const Vector3D&		GetVelocity() const;
	const Vector3D&		GetAngles() const;

	void				UpdateTransform();

protected:

	virtual void		OnPhysicsFrame(float fDt);

	void				HandleAnimatingEvent(AnimationEvent nEvent, char* options);

	void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

	CPhysicsHFObject*	m_physObj;
	bool				m_onGround;

	Vector3D			m_pedSteerAngle;

	int					m_pedState;
	bool				m_hasAI;

	float				m_thinkTime;
	CPedestrianAI		m_thinker;

	bool				m_jack;
};

#endif // PEDESTRIAN_H