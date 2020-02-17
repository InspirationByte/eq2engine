//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: External only entity queue
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEntity.h"
#include "IEntityFactory.h"
#include "IDkPhysics.h"
#include "IViewRenderer.h"
#include "DebugInterface.h"
#include "IDebugOverlay.h"

static DkList<BaseEntity*> g_Ents;
DkList<BaseEntity*> *ents = &g_Ents;

DECLARE_CMD(g_ent_by_index,"Shows entity classname by index",CV_CHEAT)
{
	if(CMD_ARGC)
	{
		int idx = atoi(CMD_ARGV(0).c_str());

		BaseEntity* pEnt = UTIL_EntByIndex(idx);
		if(pEnt)
		{
			Msg("Entity %d: %s", idx, pEnt->GetClassname());
		}
	}
}

DECLARE_CMD(g_ent_spawn,"Spawns new entity",CV_CHEAT)
{
	if(CMD_ARGC)
	{
		BaseEntity *pEntity = (BaseEntity*)entityfactory->CreateEntityByName(CMD_ARGV(0).c_str());
		if(pEntity != NULL)
		{
			// sorry, disk, I just need force precaching
			pEntity->Precache();

			pEntity->Spawn();
			pEntity->Activate();
			pEntity->SetAbsOrigin(Vector3D(0));
		}
		return;
	}
	else
		MsgInfo("Usage: g_spawn <entityname>\n");
}

void UniqueRegisterEntityForUpdate(BaseEntity* pEntity)
{
	if(pEntity == NULL)
		return;

	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEntity == pEnt)
			return;
	}

	BaseEntity* pEnt = (BaseEntity*)pEntity;
	pEnt->SetIndex( ents->numElem() );
}


BaseEntity* UTIL_EntByIndex(int nIndex)
{
	if(nIndex < UTIL_EntCount())
		return ents->ptr()[nIndex];
	else
		return NULL;
}

BaseEntity* UTIL_EntByName(const char* name, BaseEntity* pStartEntity)
{
	bool bDoSearch = false;

	if(!pStartEntity)
		bDoSearch = true;

	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(bDoSearch && !stricmp(pEnt->GetName(), name))
			return pEnt;

		if(pEnt == pStartEntity)
			bDoSearch = true;
	}

	return NULL;
}

BaseEntity* UTIL_EntByClassname(const char* name)
{
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(!stricmp(pEnt->GetClassname(), name))
			return pEnt;
	}

	return NULL;
}

void UTIL_EntitiesInSphere(Vector3D &position, float radius, DkList<BaseEntity*>* arrayPtr)
{
	ASSERT(arrayPtr);

	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(length(position - pEnt->GetAbsOrigin()) < radius)
			arrayPtr->append(pEnt);
	}
}

int UTIL_EntCount()
{
	return ents->numElem();
}

int UniqueRemoveEntity(BaseEntity* pEntity)
{
	int cnt = 0;
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEnt == pEntity)
		{
			pEnt->SetState(BaseEntity::ENTITY_REMOVE);
			cnt++;
		}
	}

	return cnt;
}

int UniqueRemoveEntityByClassname(const char* pszEntityClassname)
{
	int cnt = 0;
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(!stricmp(pEnt->GetClassname(),pszEntityClassname))
		{
			pEnt->SetState(BaseEntity::ENTITY_REMOVE);
			cnt++;
		}
	}

	return cnt;
}

int UniqueRemoveEntityByName(const char* pszEntityName)
{
	int cnt = 0;
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(!stricmp(pEnt->GetName(),pszEntityName))
		{
			pEnt->SetState(BaseEntity::ENTITY_REMOVE);
			cnt++;
		}
	}

	return cnt;
}

bool UTIL_CheckEntityExist(BaseEntity* pEntity)
{
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEnt == pEntity)
			return true;
	}

	return false;
}

void UTIL_DoForEachEntity(DOFOREACH_ENT_FN func, void* pUserData)
{
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];
		(func)(pEnt, pUserData);
	}
}

DECLARE_CMD(g_ent_remove,"Removes a named entity",CV_CHEAT)
{
	if(CMD_ARGC)
	{
		BaseEntity* pEnt = UTIL_EntByClassname(CMD_ARGV(0).c_str());

		if(!pEnt)
			pEnt = UTIL_EntByName(CMD_ARGV(0).c_str());

		if(pEnt)
		{
			pEnt->SetState(BaseEntity::ENTITY_REMOVE);
			MsgInfo("Removed entity %s (%s)\n", pEnt->GetName(), pEnt->GetClassname());
		}
	}
}

DECLARE_CMD(g_ent_removeall,"Removes a named entity",CV_CHEAT)
{
	if(CMD_ARGC)
	{
		int counter = 0;
		counter += UniqueRemoveEntityByClassname(CMD_ARGV(0).c_str());
		counter += UniqueRemoveEntityByName(CMD_ARGV(0).c_str());

		if(counter)
			MsgInfo("Removed %d %s\n", counter, CMD_ARGV(0).c_str());
	}
}

void PrintEntityName(BaseEntity* ent, void* args)
{
	Msg("Entity #%d %s '%s'\n", ent->EntIndex(), ent->GetClassname(), ent->GetName());
}

DECLARE_CMD(g_ent_list,"Prints entity list to console",CV_CHEAT)
{
	Msg("-------------\nentity list dump:\n");
	UTIL_DoForEachEntity(PrintEntityName, NULL);
}