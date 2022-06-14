//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Collision object callback
//////////////////////////////////////////////////////////////////////////////////

#pragma once
class CEqCollisionObject;
struct ContactPair_t;
struct CollisionPairData_t;

class IEqPhysCallback
{
public:
						IEqPhysCallback(CEqCollisionObject* object);
	virtual				~IEqPhysCallback();

	virtual void		PreSimulate(float fDt) = 0;
	virtual void		PostSimulate(float fDt) = 0;

	// called before collision processed
	virtual void		OnPreCollide(ContactPair_t& pair) = 0;

	// called after collision processed and applied impulses
	virtual void		OnCollide(const CollisionPairData_t& pair) = 0;

	CEqCollisionObject* m_object;
};

