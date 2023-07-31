//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	virtual				~IEqPhysCallback() = default;

	virtual void		OnStartMove() = 0;
	virtual void		OnStopMove() = 0;

	virtual void		PreSimulate(float fDt) = 0;
	virtual void		PostSimulate(float fDt) = 0;

	// called before collision processed
	virtual void		OnPreCollide(ContactPair_t& pair) = 0;

	// called after collision processed and applied impulses
	virtual void		OnCollide(const CollisionPairData_t& pair) = 0;

protected:
	void				Attach(CEqCollisionObject* object);
	void				Detach();

	CEqCollisionObject* m_object{ nullptr };
};

