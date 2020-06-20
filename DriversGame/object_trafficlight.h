//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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
	DECLARE_CLASS(CObject_TrafficLight, CGameObject)

	CObject_TrafficLight( kvkeybase_t* kvdata );
	~CObject_TrafficLight();

	void				OnRemove();
	void				Spawn();

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	void				Draw( int nRenderFlags );

	void				Simulate(float fDt);

	int					ObjType() const	{return GO_LIGHT_TRAFFIC;}

	void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

protected:

	DkList<trafficlights_t>	m_lights;

	CEqCollisionObject*		m_physObj;

	int						m_trafficDir;	// the traffic light direction
	float					m_flickerTime;

	ushort					m_junctionId;

	bool					m_flicker;
	bool					m_killed;
};

#endif // OBJECT_TRAFFICLIGHT_H