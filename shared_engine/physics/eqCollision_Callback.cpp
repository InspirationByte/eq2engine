//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Collision object callback
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqCollision_Callback.h"
#include "eqCollision_Object.h"

IEqPhysCallback::IEqPhysCallback(CEqCollisionObject* object) : m_object(object)
{
	ASSERT(m_object);
	m_object->m_callbacks = this;
}

IEqPhysCallback::~IEqPhysCallback()
{
	if (m_object)
		m_object->m_callbacks = nullptr;
}