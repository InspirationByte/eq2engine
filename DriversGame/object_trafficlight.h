//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Traffic light
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_TRAFFICLIGHT_H
#define OBJECT_TRAFFICLIGHT_H

#include "GameObject.h"
#include "world.h"

struct trafficlights_t
{
	int			type;
	int			direction;
	Vector3D	position;
};

class CObject_TrafficLight : public CGameObject
{
public:
	CObject_TrafficLight( kvkeybase_t* kvdata );
	~CObject_TrafficLight();

	void				OnRemove();
	void				Spawn();

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	Vector3D			GetOrigin();
	Vector3D			GetAngles();
	Vector3D			GetVelocity();

	void				Draw( int nRenderFlags );

	void				Simulate(float fDt);

	int					ObjType() const		{return GO_LIGHT_TRAFFIC;}

	void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

protected:

	CEqCollisionObject*		m_pPhysicsObject;

	int						m_trafficDir;	// the traffic light direction

	DkList<trafficlights_t>	m_lights;
	

	bool					m_flicker;
	bool					m_killed;
	float					m_flickerTime;
};

#endif // OBJECT_TRAFFICLIGHT_H