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

class CPedestrian : 
	public CControllableGameObject,
	public CAnimatingEGF
{
	friend class CCar;
public:
	DECLARE_CLASS(CPedestrian, CControllableGameObject)

	CPedestrian();
	CPedestrian(kvkeybase_t* kvdata);
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

	const Vector3D&		GetOrigin();
	const Vector3D&		GetAngles();
	const Vector3D&		GetVelocity() const;

	void				UpdateTransform();

protected:

	void				HandleAnimatingEvent(AnimationEvent nEvent, char* options);

	void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

	CEqRigidBody*		m_physBody;
	bool				m_onGround;

	int					m_pedState;

	float				m_thinkTime;
	CPedestrianAI		m_thinker;
};

#endif // PEDESTRIAN_H