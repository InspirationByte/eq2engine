//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Script library registrator
//////////////////////////////////////////////////////////////////////////////////

#include "EntityBind.h"
#include "BaseEntity.h"

// also bind AI objects
#include "AI/AITaskActionBase.h"

// Init statics and constants
gmType GMTYPE_EQENTITY	= -1;

void GM_CDECL Entity_GetDot(gmThread * a_thread, gmVariable * a_operands)
{
	//O_GETDOT = 0,       // object, "member"          (tos is a_operands + 2)
	GM_ASSERT(a_operands[0].m_type == GMTYPE_EQENTITY);

	gmUserObject* userObj = (gmUserObject*) GM_OBJECT(a_operands[0].m_value.m_ref);
	BaseEntity* pEntity = (BaseEntity*)userObj->m_user;

	if(!pEntity)
	{
		a_operands[0].Nullify();

		return;
	}

	a_operands[0] = pEntity->GetScriptTableObject()->Get(a_operands[1]);
}

void GM_CDECL Entity_SetDot(gmThread * a_thread, gmVariable * a_operands)
{
	//O_SETDOT,           // object, value, "member"   (tos is a_operands + 3)
	GM_ASSERT(a_operands[0].m_type == GMTYPE_EQENTITY);

	gmUserObject* userObj = (gmUserObject*) GM_OBJECT(a_operands[0].m_value.m_ref);
	BaseEntity* pEntity = (BaseEntity*)userObj->m_user;

	if(pEntity)
	{
		pEntity->GetScriptTableObject()->Set(a_thread->GetMachine(), a_operands[2], a_operands[1]);
	}
}


//------------------------------------------------------------------------------------------------

// type library

int GM_CDECL GM_BaseEntity_Precache(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->Precache();

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_Spawn(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->Spawn();

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_Activate(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->Activate();

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetName(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(name, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetName(name);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_GetName(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushNewString(pEntity->GetName());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetClassname(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(name, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetClassName(name);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_GetClassname(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushNewString(pEntity->GetClassname());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_EntIndex(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushInt(pEntity->EntIndex());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetEntIndex(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(index, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetIndex(index);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetHealth(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(value, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetHealth(value);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_GetHealth(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushInt(pEntity->GetHealth());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_IsDead(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushInt(pEntity->IsDead());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_IsAlive(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushInt(pEntity->IsAlive());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetModel(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	if(GM_THREAD_ARG->ParamType(0) != GM_STRING)
	{
		GM_EXCEPTION_MSG("expecting param %d as string (type is %d)", 0, GM_THREAD_ARG->ParamType(0));
		return GM_EXCEPTION;
	}
	const char * path = (const char *) *((gmStringObject *) GM_OBJECT(GM_THREAD_ARG->ParamRef(0)));

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetModel(path);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_Classify(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushInt(pEntity->Classify());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_EntType(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	GM_THREAD_ARG->PushInt(pEntity->EntType());

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_GetKey(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(key, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	variable_t var = pEntity->GetKey(key);

	// not supported
	if(var.GetVarType() == VTYPE_VOID)
	{
		GM_THREAD_ARG->PushNull();
	}
	else if(var.GetVarType() == VTYPE_BOOLEAN)
	{
		GM_THREAD_ARG->PushInt(var.GetBool());
	}
	else if(var.GetVarType() == VTYPE_INTEGER)
	{
		GM_THREAD_ARG->PushInt(var.GetInt());
	}
	else if(var.GetVarType() == VTYPE_FLOAT)
	{
		GM_THREAD_ARG->PushFloat(var.GetFloat());
	}
	else if(var.GetVarType() == VTYPE_VECTOR2D)
	{
		GM_THREAD_ARG->PushNull();
	}
	else if(var.GetVarType() == VTYPE_VECTOR3D)
	{
		GM_THREAD_ARG->PushNull();
	}
	else if(var.GetVarType() == VTYPE_VECTOR4D)
	{
		GM_THREAD_ARG->PushNull();
	}

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetKey(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_STRING_PARAM(key, 0);

	variable_t var;
	var.SetInt(0);

	if(GM_THREAD_ARG->ParamType(1) == GM_INT)
	{
		GM_INT_PARAM(a_value, 1, 0);
		var.SetInt(a_value);
	}
	else if(GM_THREAD_ARG->ParamType(1) == GM_FLOAT)
	{
		GM_FLOAT_PARAM(a_value, 1, 0);
		var.SetFloat(a_value);
	}	
	else if(GM_THREAD_ARG->ParamType(1) == GM_STRING)
	{
		GM_STRING_PARAM(a_value, 1, 0);
		var.SetString(a_value);
	}	

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetKey(key, var);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetNextThink(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_FLOAT_OR_INT_PARAM(nexttime, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetNextThink(nexttime);

	return GM_OK;
}

// position and angles
int GM_CDECL GM_BaseEntity_GetAbsOrigin(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	Vector3D vec = pEntity->GetAbsOrigin();

	gmVector3_Push(GM_THREAD_ARG, vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetAbsOrigin(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_USER_PARAM(Vector3D*, GM_VECTOR3, vec, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetAbsOrigin(*vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_GetAbsAngles(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	Vector3D vec = pEntity->GetAbsAngles();

	gmVector3_Push(GM_THREAD_ARG, vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetAbsAngles(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_USER_PARAM(Vector3D*, GM_VECTOR3, vec, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetAbsAngles(*vec);

	return GM_OK;
}


int GM_CDECL GM_BaseEntity_GetLocalOrigin(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	Vector3D vec = pEntity->GetLocalOrigin();

	gmVector3_Push(GM_THREAD_ARG, vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetLocalOrigin(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_USER_PARAM(Vector3D*, GM_VECTOR3, vec, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetLocalOrigin(*vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_GetLocalAngles(gmThread* a_thread)
{
	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	Vector3D vec = pEntity->GetLocalAngles();

	gmVector3_Push(GM_THREAD_ARG, vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_SetLocalAngles(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_USER_PARAM(Vector3D*, GM_VECTOR3, vec, 0);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	pEntity->SetLocalAngles(*vec);

	return GM_OK;
}

int GM_CDECL GM_BaseEntity_EmitSound(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);

	BaseEntity* pEntity = GetThisEntity(a_thread);

	if(!pEntity)
		return GM_EXCEPTION;

	if(GM_THREAD_ARG->ParamType(0) != GM_TABLE)
	{
		GM_CHECK_STRING_PARAM(name, 0);

		pEntity->EmitSound(name);
	}
	else
	{
		GM_CHECK_TABLE_PARAM(emitParams, 0);

		EmitSound_t es;

		gmVariable nameVar = emitParams->Get(GM_THREAD_ARG->GetMachine(), "name");
		gmVariable pitchVar = emitParams->Get(GM_THREAD_ARG->GetMachine(), "pitch");
		gmVariable originVar = emitParams->Get(GM_THREAD_ARG->GetMachine(), "origin");
		gmVariable volumeVar = emitParams->Get(GM_THREAD_ARG->GetMachine(), "volume");

		es.name = (char*)nameVar.GetCStringSafe();

		if(!pitchVar.IsNull())
			es.fPitch = pitchVar.GetFloat();

		if(!volumeVar.IsNull())
			es.fVolume = volumeVar.GetFloat();

		if(!originVar.IsNull())
		{
			Vector3D* pVec = (Vector3D*)originVar.GetUserSafe(GM_VECTOR3);
			es.origin = *pVec;
		};

		pEntity->EmitSoundWithParams(&es);
	}

	return GM_OK;
}

//----------------------------------------------------------------------------------------

// also need a library of entity queue

int GM_CDECL GM_EntQueue_EntByIndex(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(index, 0);

	BaseEntity* pEntity = UTIL_EntByIndex(index);

	if(pEntity)
	{
		GM_THREAD_ARG->PushUser(pEntity->GetScriptUserObject());
	}
	else
	{
		gmVariable nullVar;
		nullVar.Nullify();

		GM_THREAD_ARG->Push(nullVar);
	}

	return GM_OK;
}

int GM_CDECL GM_EntQueue_EntByName(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(name, 0);

	BaseEntity* pFirstEnt = NULL;

	if(GM_THREAD_ARG->GetNumParams() == 2)
	{
		if(GM_THREAD_ARG->ParamType(1) != GMTYPE_EQENTITY)
		{
			GM_EXCEPTION_MSG("expecting param %d as Entity", 1);
			return GM_EXCEPTION;
		} 

		pFirstEnt = (BaseEntity*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(1);
	}

	BaseEntity* pEntity = UTIL_EntByName(name, pFirstEnt);

	if(pEntity)
	{
		GM_THREAD_ARG->PushUser(pEntity->GetScriptUserObject());
	}
	else
	{
		gmVariable nullVar;
		nullVar.Nullify();

		GM_THREAD_ARG->Push(nullVar);
	}

	return GM_OK;
}

int GM_CDECL GM_EntQueue_EntByClassname(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(name, 0);

	BaseEntity* pEntity = UTIL_EntByClassname(name);

	if(pEntity)
	{
		GM_THREAD_ARG->PushUser(pEntity->GetScriptUserObject());
	}
	else
	{
		gmVariable nullVar;
		nullVar.Nullify();

		GM_THREAD_ARG->Push(nullVar);
	}

	return GM_OK;
}

int GM_CDECL GM_EntQueue_EntitiesInSphere(gmThread* a_thread)
{
	ASSERT(!"entities.EntitiesInSphere not implemented");
	return GM_OK;
}

int GM_CDECL GM_EntQueue_EntCount(gmThread* a_thread)
{
	GM_THREAD_ARG->PushInt(UTIL_EntCount());

	return GM_OK;
}

int GM_CDECL GM_EntQueue_CheckEntityExist(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	if(GM_THREAD_ARG->ParamType(0) != GMTYPE_EQENTITY)
	{
		GM_EXCEPTION_MSG("expecting param %d as Entity", 0);
		return GM_EXCEPTION;
	} 

	BaseEntity* pEnt = (BaseEntity*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(0);

	GM_THREAD_ARG->PushInt(UTIL_CheckEntityExist(pEnt));

	return GM_OK;
}

int GM_CDECL GM_Ent_Create(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(classname, 0);

	BaseEntity *pEntity = (BaseEntity*)entityfactory->CreateEntityByName(classname);
	if(pEntity != NULL)
	{
		// sorry, disk, I just need force precaching
		pEntity->Precache();
	}

	if(pEntity)
	{
		GM_THREAD_ARG->PushUser(pEntity->GetScriptUserObject());
	}
	else
	{
		gmVariable nullVar;
		nullVar.Nullify();

		GM_THREAD_ARG->Push(nullVar);
	}

	return GM_OK;
}

int GM_CDECL GM_Ent_Remove(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	if(GM_THREAD_ARG->ParamType(0) != GMTYPE_EQENTITY)
	{
		GM_EXCEPTION_MSG("expecting param %d as Entity", 0);
		return GM_EXCEPTION;
	} 

	BaseEntity* pEnt = (BaseEntity*)GM_THREAD_ARG->ParamUser_NoCheckTypeOrParam(0);

	UniqueRemoveEntity(pEnt);

	return GM_OK;
}

int GM_CDECL GM_Ent_Precache(gmThread* a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(classname, 0);

	PrecacheEntity(classname);

	return GM_OK;
}

void gmBindEntityLib(gmMachine * a_machine)
{
	// register entity object
	GMTYPE_EQENTITY = a_machine->CreateUserType("Entity");

	// Bind Get dot operator for our type
	a_machine->RegisterTypeOperator(GMTYPE_EQENTITY, O_GETDOT, NULL, Entity_GetDot);

	// Bind Set dot operator for our type
	a_machine->RegisterTypeOperator(GMTYPE_EQENTITY, O_SETDOT, NULL, Entity_SetDot);

	// the standard baseentity functions...
	// for extensions use InitScriptHooks
	static gmFunctionEntry entityTypeLib[] = 
	{ 
		{"Precache", GM_BaseEntity_Precache},
		{"Spawn", GM_BaseEntity_Spawn},
		{"Activate", GM_BaseEntity_Activate},

		{"SetName", GM_BaseEntity_SetName},
		{"GetName", GM_BaseEntity_GetName},
		{"SetClassname", GM_BaseEntity_SetClassname},
		{"GetClassname", GM_BaseEntity_GetClassname},

		{"EntIndex", GM_BaseEntity_EntIndex},
		{"SetEntIndex", GM_BaseEntity_SetEntIndex},

		{"SetHealth", GM_BaseEntity_SetHealth},
		{"GetHealth", GM_BaseEntity_GetHealth},

		{"IsDead", GM_BaseEntity_IsDead},
		{"IsAlive", GM_BaseEntity_IsAlive},

		{"SetModel", GM_BaseEntity_SetModel},

		{"Classify", GM_BaseEntity_Classify},

		{"EntType", GM_BaseEntity_EntType},

		{"GetKey", GM_BaseEntity_GetKey},
		{"SetKey", GM_BaseEntity_SetKey},

		{"SetNextThink", GM_BaseEntity_SetNextThink},

		// position and angles
		{"GetAbsOrigin", GM_BaseEntity_GetAbsOrigin},
		{"SetAbsOrigin", GM_BaseEntity_SetAbsOrigin},

		{"GetAbsAngles", GM_BaseEntity_GetAbsAngles},
		{"SetAbsAngles", GM_BaseEntity_SetAbsAngles},

		{"GetLocalOrigin", GM_BaseEntity_GetLocalOrigin},
		{"SetLocalOrigin", GM_BaseEntity_SetLocalOrigin},

		{"GetLocalAngles", GM_BaseEntity_GetLocalAngles},
		{"SetLocalAngles", GM_BaseEntity_SetLocalAngles},

		{"EmitSound", GM_BaseEntity_EmitSound},
	};

	a_machine->RegisterTypeLibrary(GMTYPE_EQENTITY, entityTypeLib, elementsOf(entityTypeLib));

	static gmFunctionEntry entityUtilsLib[] = 
	{ 
		{"FindByIndex", GM_EntQueue_EntByIndex},
		{"FindByName", GM_EntQueue_EntByName},
		{"FindByClassname", GM_EntQueue_EntByClassname},
		{"EntitiesInSphere", GM_EntQueue_EntitiesInSphere},
		{"Count", GM_EntQueue_EntCount},
		{"IsExist", GM_EntQueue_CheckEntityExist},
		{"Create", GM_Ent_Create},
		{"Remove", GM_Ent_Remove},
		{"Precache", GM_Ent_Precache},
	};

	a_machine->RegisterLibrary(entityUtilsLib, elementsOf(entityUtilsLib), "entities", false);
}