//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics controllers
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqPhysicsWorld;

class IEqPhysController
{
	friend class CEqPhysicsWorld;
public:
					IEqPhysController() : m_enabled(false) {}
	virtual			~IEqPhysController() {}

	virtual void	Update(float dt) = 0;

	virtual void	SetEnabled(bool enable) { m_enabled = enable; }
	bool			IsEnabled()	const		{ return m_enabled; }

protected:
	virtual void	AddedToWorld( CEqPhysicsWorld* physics ) = 0;
	virtual void	RemovedFromWorld( CEqPhysicsWorld* physics ) = 0;

	bool			m_enabled;
};
