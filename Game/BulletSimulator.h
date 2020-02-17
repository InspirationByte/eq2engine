//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Realistic bullets manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef BULLETMANAGER_H
#define BULLETMANAGER_H

#include "math/Vector.h"
#include "utils/DkLinkedList.h"
#include "AmmoType.h"
#include "IDkPhysics.h"

class BaseEntity;
struct trace_t;

class CSimulatedBullet
{
public:
	CSimulatedBullet();
	CSimulatedBullet(Vector3D &startpos, Vector3D &dir, BaseEntity* pShooter, BaseEntity* pInflictor, int ammo_type);

	bool			Simulate(float deltaTime);	// simulates the bullet. Returns false if bullet speed less than epsilon
	bool			ProcessCollision(trace_t* collided);


protected:

	Vector3D		m_vOrigin;				// position
	Vector3D		m_vVelocity;			// velocity. position advances by it, mult. by fDT
	Vector3D		m_vGravity;				// gravity

	float			m_fInitialSpeed;		// initial speed to compute bullet speed loss

	BaseEntity*		m_pShooter;				// what fired this bullet
	BaseEntity*		m_pInflictor;			// who fired this bullet

	int				m_nAmmoType;			// fired ammo type
};

//-------------------------------------------------------------------
// Bullet simulator

class CBulletSimulator
{
public:
	void Cleanup();
	void FireBullet(Vector3D &origin, Vector3D &dir, BaseEntity* pShooter, BaseEntity* pInflictor, int ammo_type);
	void SimulateBullets();

private:
	DkList<CSimulatedBullet*> m_bullets;
};

extern CBulletSimulator* g_pBulletSimulator;

#endif