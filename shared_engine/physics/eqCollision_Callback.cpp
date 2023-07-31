//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Collision object callback
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqCollision_Callback.h"
#include "eqCollision_Object.h"

void IEqPhysCallback::Attach(CEqCollisionObject* object)
{
	Detach();
	m_object = object;
	if(object)
		object->m_callbacks = this;
}

void IEqPhysCallback::Detach()
{
	if (m_object)
		m_object->m_callbacks = nullptr;
	m_object = nullptr;
}