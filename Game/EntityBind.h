//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Script library registrator
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENTITYBIND_H
#define ENTITYBIND_H

#include "EqGMS.h"

class BaseEntity;

extern gmType GMTYPE_EQENTITY;
extern gmType GMTYPE_AITASK;

inline BaseEntity* GetEntityByVar(gmVariable* var)
{
	GM_ASSERT(var->m_type == GMTYPE_EQENTITY); //Paranoid check for type function

	return (BaseEntity*)var->GetUserSafe(GMTYPE_EQENTITY);
}

inline BaseEntity* GetThisEntity(gmThread* a_thread)
{
	GM_ASSERT(a_thread->GetThis()->m_type == GMTYPE_EQENTITY); //Paranoid check for type function

	BaseEntity* pEnt = (BaseEntity*)a_thread->ThisUser();

	if(!pEnt)
		return NULL;

	return pEnt;
}

void gmBindEntityLib(gmMachine * a_machine);

#endif // ENTITYBIND_H