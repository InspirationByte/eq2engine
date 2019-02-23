//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian AI
//////////////////////////////////////////////////////////////////////////////////

#include "AIPedestrian.h"

CPedestrian::CPedestrian(kvkeybase_t* kvdata)
{

}

CPedestrian::~CPedestrian()
{

}

void CPedestrian::OnRemove()
{
	BaseClass::Spawn();
}

void CPedestrian::Spawn()
{
	BaseClass::Spawn();
}

void CPedestrian::Draw(int nRenderFlags)
{

}

void CPedestrian::Simulate(float fDt)
{

}

void CPedestrian::SetOrigin(const Vector3D& origin)
{
	if (m_physBody)
		m_physBody->SetPosition(origin);

	m_vecOrigin = origin;
}

void CPedestrian::SetAngles(const Vector3D& angles)
{
	if (m_physBody)
		m_physBody->SetOrientation(Quaternion(DEG2RAD(angles.x), DEG2RAD(angles.y), DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CPedestrian::SetVelocity(const Vector3D& vel)
{
	if (m_physBody)
		m_physBody->SetLinearVelocity(vel);
}

const Vector3D& CPedestrian::GetOrigin() const
{
	return m_vecOrigin;
}

const Vector3D& CPedestrian::GetAngles() const
{
	return m_vecAngles;
}

const Vector3D& CPedestrian::GetVelocity() const
{
	return m_physBody->GetLinearVelocity();
}