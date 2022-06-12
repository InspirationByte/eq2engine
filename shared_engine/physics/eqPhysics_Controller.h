//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics controllers
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqPhysics;

class IEqPhysicsController
{
	friend class CEqPhysics;
public:
					IEqPhysicsController() : m_enabled(false) {}
	virtual			~IEqPhysicsController() {}

	virtual void	Update(float dt) = 0;

	virtual void	SetEnabled(bool enable) { m_enabled = enable; }
	bool			IsEnabled()	const		{ return m_enabled; }

protected:
	virtual void	AddedToWorld( CEqPhysics* physics ) = 0;
	virtual void	RemovedFromWorld( CEqPhysics* physics ) = 0;

	bool			m_enabled;
};
