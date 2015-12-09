//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: External only entity queue
//////////////////////////////////////////////////////////////////////////////////

#ifndef ENTQUEUE_H
#define ENTQUEUE_H

class BaseEntity;

// TODO: CWorldSpace, CWorldLayer and rename this file to WorldObjectSpace.h/cpp

// External only entity queue
extern DkList<BaseEntity*>* ents;

typedef void (*DOFOREACH_ENT_FN)(BaseEntity* ent, void* args);

void				UniqueRegisterEntityForUpdate(BaseEntity* pEntity);

int					UniqueRemoveEntity(BaseEntity* pEntity);
int					UniqueRemoveEntityByClassname(const char* pszEntityClassname);
int					UniqueRemoveEntityByName(const char* pszEntityName);

BaseEntity*			UTIL_EntByIndex(int nIndex);
BaseEntity*			UTIL_EntByName(const char* name, BaseEntity* pStartEntity = NULL);
BaseEntity*			UTIL_EntByClassname(const char* name);
void				UTIL_DoForEachEntity(DOFOREACH_ENT_FN func, void* pUserData);

void				UTIL_EntitiesInSphere(Vector3D &position, float radius, DkList<BaseEntity*>* arrayPtr);

int					UTIL_EntCount();

bool				UTIL_CheckEntityExist(BaseEntity* pEntity);

#endif // ENTQUEUE_H