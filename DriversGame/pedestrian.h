//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#ifndef PEDESTRIAN_H
#define PEDESTRIAN_H

#include "ControllableObject.h"
#include "Animating.h"
#include "EventFSM.h"
#include "GameDefs.h"

class CPedestrian;

class CPedestrianAI : public CFSM_Base
{
	friend class CPedestrian;
public:
	CPedestrianAI(CPedestrian* host);

	void				InitAI();
	void				DestroyAI();

	int					DoWalk(float fDt, EStateTransition transition);
	int					DoEscape(float fDt, EStateTransition transition);

	int					SearchDaWay(float fDt, EStateTransition transition);

	bool				GetNextPath(int dir);
	void				DetectEscape();

	static void			DetectEscapeJob(void* data, int i);
	static void			Evt_CarHornHandler(const eventArgs_t& args);

	Vector3D			m_escapeDir;
	Vector3D			m_escapeFromPos;

	Vector2D			m_cellOffset;

	IVector2D			m_nextRoadTile;

	float				m_nextEscapeCheckTime;

	int					m_curDir;
	int					m_prevDir;

	levroadcell_t*		m_nextRoadCell;
	levroadcell_t*		m_prevRoadCell;

	CPedestrian*		m_host;
	CGameObject*		m_escapeObject;

	eventSub_t			m_signalEvent;
};

//------------------------------------------------------

struct pedestrianConfig_t
{
	EqString name;
	EqString model;
	bool hasAI;
	int spawnInterval;
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

	Vector3D			m_pedSteerAngle;

	int					m_pedState;
	bool				m_hasAI;

	float				m_thinkTime;
	CPedestrianAI		m_thinker;

	bool				m_jack;
	int					m_skin;
};

#endif // PEDESTRIAN_H