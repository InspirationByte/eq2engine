//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Save game file manager (load/save/read functions)
//////////////////////////////////////////////////////////////////////////////////

#include "SaveRestoreManager.h"

#include "BaseEngineHeader.h"
#include "IGameRules.h"
#include "SaveRestoreDefaultFunctions.h"
#include "EntityDataField.h"

#include "GameState.h"

// SAVE - RESTORE functions

struct physsaverestdata_t
{
	int			collisionFlags;
	int			collisionGroup;

	Vector3D	childTransforms[8]; // TODO: MAX_CHILDS 8
	int			nChilds;

	Matrix4x4	globalTransform;

	Vector3D	angularVelocity;
	Vector3D	linearVelocity;

	float		linearDamp;
	float		angularDamp;
};

int Save_PhysicsObject(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	IPhysicsObject *pObject = *(IPhysicsObject**)pObjectPtr;

	bool bHasPhysObject = (pObject != NULL);

	int nSize = pStream->Write(&bHasPhysObject,1,sizeof(bool));

	if(!bHasPhysObject)
		return nSize;

	physsaverestdata_t data;

	data.globalTransform = pObject->GetTransformMatrix();

	data.linearVelocity = pObject->GetVelocity();
	data.angularVelocity = pObject->GetAngularVelocity();
	data.nChilds = 0;
	data.collisionGroup = pObject->GetContents();
	data.collisionFlags = pObject->GetCollisionMask();
	data.linearDamp = pObject->GetDampingLinear();
	data.angularDamp = pObject->GetDampingAngular();

	nSize += pStream->Write(&data, 1, sizeof(data));

	return nSize;
}

int Restore_PhysicsObject(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	// WE ASSUMING THAT THE PHYSICS OBJECTS STORED IN ENTITIES
	BaseEntity* pEntity = (BaseEntity*)pStruct;

	IPhysicsObject *pObject = *(IPhysicsObject**)pObjectPtr;

	bool bHasPhysObject = false;

	int nSize = pStream->Read(&bHasPhysObject,1,sizeof(bool));

	if(bHasPhysObject)
	{
		physsaverestdata_t data;

		nSize += pStream->Read(&data, 1, sizeof(data));

		// precache
		pEntity->Precache();

		pEntity->SetModel(pEntity->GetModelName());

		pEntity->PhysicsCreateObjects();

		pObject = *(IPhysicsObject**)pObjectPtr;

		if(pObject)
		{
			//Msg("Physics object is initialized\n");
			pObject->SetTransformFromMatrix(transpose(data.globalTransform));
			pObject->SetVelocity(data.linearVelocity);
			pObject->SetAngularVelocity(data.angularVelocity, 1.0f);
			pObject->SetContents(data.collisionGroup);
			pObject->SetCollisionMask(data.collisionFlags);
			pObject->SetDamping(data.linearDamp, data.angularDamp);
		}
	}

	return nSize;
}

datavariant_t* FindDataVariantInMapByFunction_r(datamap_t *pMap, BASE_THINK_PTR function)
{
	if(function == NULL)
		return NULL;

	for(int i = 0; i < pMap->m_dataNumFields; i++)
	{
		if(!(pMap->m_fields[i].nFlags & FIELDFLAG_FUNCTION))
			continue;

		if(((BASE_THINK_PTR)pMap->m_fields[i].functionPtr) == function)
			return &pMap->m_fields[i];
	}

	if(pMap->baseMap)
	{
		return FindDataVariantInMapByFunction_r( pMap->baseMap, function );
	}

	return NULL;
}

datavariant_t* FindFunctionInMap_r(datamap_t *pMap, const char* fieldname)
{
	for(int i = 0; i < pMap->m_dataNumFields; i++)
	{
		if(!(pMap->m_fields[i].nFlags & FIELDFLAG_FUNCTION))
			continue;

		if(!strcmp(pMap->m_fields[i].name, fieldname))
			return &pMap->m_fields[i];
	}

	if(pMap->baseMap)
	{
		return FindFunctionInMap_r( pMap->baseMap, fieldname );
	}

	return NULL;
}

int Save_FunctionPtr(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	// we need datamap
	BaseEntity* pEntity = (BaseEntity*)pStruct;

	datavariant_t* pFunctionField = FindDataVariantInMapByFunction_r(pEntity->GetDataDescMap(), *(BASE_THINK_PTR*)pObjectPtr);
	if(!pFunctionField)
	{
		EqString nameString("NULL");
		return Save_String(pEntity, &nameString, pStream);
	}
	else
	{
		EqString nameString(pFunctionField->name);
		return Save_String(pEntity, &nameString, pStream);
	}
}

int Restore_FunctionPtr(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	// we need datamap
	BaseEntity* pEntity = (BaseEntity*)pStruct;

	EqString nameString("NULL");

	int read = Restore_String(pEntity, &nameString, pStream);
	if(!nameString.Compare("NULL"))
	{
		(*(BASE_THINK_PTR*)pObjectPtr) = NULL;
	}
	else
	{
		datavariant_t* pFunctionField = FindFunctionInMap_r(pEntity->GetDataDescMap(), nameString.GetData());
		if(pFunctionField)
		{
			(*(BASE_THINK_PTR*)pObjectPtr) = (BASE_THINK_PTR)pFunctionField->functionPtr;
		}
		else
		{
			MsgError("*** FUNCTION '%s' NOT FOUND IN ENTITY '%s'! ***\n", nameString.GetData(), pEntity->GetClassname());
		}
	}

	return read;
}

// callback for loading/saving the some fields.
//  pObjectPtr is a object to read/write data.
//  saveGamePtr is a pointer to saved game data that read or will be written
//  callback must return written/read byte count

typedef int (*SAVE_FIELD_METHOD)(void* pStruct, void* pObjectPtr, IVirtualStream* pStream);
#define RESTORE_FIELD_METHOD SAVE_FIELD_METHOD

// the containers for save/restore functions
static SAVE_FIELD_METHOD		s_savedGameSaveFunctions[VTYPE_COUNT];
static RESTORE_FIELD_METHOD		s_savedGameRestoreFunctions[VTYPE_COUNT];

// registers save/restore functions for field type
void RegisterSaveRestore(VariableType_e field_type, SAVE_FIELD_METHOD saveMethod, RESTORE_FIELD_METHOD restoreMethod)
{
	// set save/restore functions
	s_savedGameSaveFunctions[field_type] = saveMethod;
	s_savedGameRestoreFunctions[field_type] = restoreMethod;
};

void SaveGame_WriteField(void* pStruct, datavariant_t* pField, IVirtualStream* pStream);
void RestoreEntityPointers_r(datamap_t* pDataMap, void* pStruct, DkList<BaseEntity*> &entlist);

// VTYPE_TIME

GlobalVarsBase gpGlobalsFile;

int Restore_FloatTime(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	int nSize = pStream->Read(pObjectPtr, 1, sizeof(float));
	float* ptr = (float*)pObjectPtr;

	*ptr = (*ptr) - gpGlobalsFile.curtime;

	return nSize;
}

int Restore_Modelname(void* pStruct, void* pObjectPtr, IVirtualStream* pStream)
{
	int sSize = Restore_String(pStruct, pObjectPtr, pStream);

	EqString* strPtr = (EqString*)pObjectPtr;

	PrecacheStudioModel(strPtr->GetData());

	// we assuming that the pStruct is baseentity object!!!
	BaseEntity* pEnt = (BaseEntity*)pStruct;

	pEnt->SetModel( pEnt->GetModelName() );

	return sSize;
}

// initializes save/restore functions
void SaveRestoreInitDefaults()
{
	memset(s_savedGameSaveFunctions, 0, sizeof(s_savedGameSaveFunctions));
	memset(s_savedGameRestoreFunctions, 0, sizeof(s_savedGameRestoreFunctions));

	// load all default save/restore functions
	RegisterSaveRestore(VTYPE_VOID,				Save_Empty, Restore_Empty);

	// simple types
	RegisterSaveRestore(VTYPE_FLOAT,			Save_Float, Restore_Float);
	RegisterSaveRestore(VTYPE_INTEGER,			Save_Int, Restore_Int);
	RegisterSaveRestore(VTYPE_BOOLEAN,			Save_Bool, Restore_Bool);
	RegisterSaveRestore(VTYPE_SHORT,			Save_Short, Restore_Short);
	RegisterSaveRestore(VTYPE_BYTE,				Save_Byte, Restore_Byte);
	RegisterSaveRestore(VTYPE_STRING,			Save_String, Restore_String);
	RegisterSaveRestore(VTYPE_MODELNAME,		Save_String, Restore_Modelname);
	RegisterSaveRestore(VTYPE_SOUNDNAME,		Save_String, Restore_String);

	// vector types
	RegisterSaveRestore(VTYPE_VECTOR2D,			Save_Vector2D, Restore_Vector2D);
	RegisterSaveRestore(VTYPE_VECTOR3D,			Save_Vector3D, Restore_Vector3D);
	RegisterSaveRestore(VTYPE_VECTOR4D,			Save_Vector4D, Restore_Vector4D);

	// optimized origin
	RegisterSaveRestore(VTYPE_ORIGIN,			Save_Vector3D, Restore_Vector3D);
	// optimized angles
	RegisterSaveRestore(VTYPE_ANGLES,			Save_Vector3D, Restore_Vector3D);

	// angle - fixed float
	RegisterSaveRestore(VTYPE_ANGLE,			Save_Float, Restore_Float);

	RegisterSaveRestore(VTYPE_TIME,				Save_Float, Restore_FloatTime);

	// matrix types
	RegisterSaveRestore(VTYPE_MATRIX2X2,		Save_Matrix2x2, Restore_Matrix2x2);
	RegisterSaveRestore(VTYPE_MATRIX3X3,		Save_Matrix3x3, Restore_Matrix3x3);
	RegisterSaveRestore(VTYPE_MATRIX4X4,		Save_Matrix4x4, Restore_Matrix4x4);

	RegisterSaveRestore(VTYPE_ENTITYPTR,		Save_EntityPtr, Restore_EntityPtr);

	RegisterSaveRestore(VTYPE_FUNCTION,			Save_FunctionPtr, Restore_FunctionPtr);
	RegisterSaveRestore(VTYPE_CUSTOM,			Save_Empty, Restore_Empty);

	// physics object now has a save/restore
	RegisterSaveRestore(VTYPE_PHYSICSOBJECT,	Save_PhysicsObject, Restore_PhysicsObject);
}

#define SAVEGAME_IDENT		(('G'<<24)+('V'<<16)+('S'<<8)+'E') // equilibrium savegame, ESVG
#define SAVEGAME_VERSION	5

// save header
struct save_header_t
{
	int		ident;		// "ESVG"
	int		version;	// version of saved game

	int		numEntities; // entities on map
};

// global variables base, skipping structure

int SaveGame_DataMapFieldCount(datamap_t* pDataMap)
{
	int numFields = 0;

	for(int i = 0; i < pDataMap->m_dataNumFields; i++)
	{
		if(pDataMap->m_fields[i].nFlags & FIELDFLAG_SAVE)
			numFields++;
	}

	if(pDataMap->baseMap)
		numFields += SaveGame_DataMapFieldCount(pDataMap->baseMap);

	return numFields;
}

void SaveGame_RecursiveWriteDatamap(void* pStruct, datamap_t* pDataMap, IVirtualStream* pStream);

void SaveGame_WriteField(void* pStruct, datavariant_t* pField, IVirtualStream* pStream)
{
	if(!(pField->nFlags & FIELDFLAG_SAVE))
		return;

	save_field_t dpField;

	strcpy(dpField.name, pField->name);
	dpField.type = pField->type;

	pStream->Write(&dpField, 1, sizeof(dpField));

	if(pField->type == VTYPE_EMBEDDED)
	{
		datamap_t* pDataMap = pField->customDatamap;

		int numFields = SaveGame_DataMapFieldCount(pDataMap);

		pStream->Write(&numFields, 1, sizeof(int));

		for(int i = 0; i < pField->fieldSize; i++)
		{
			SaveGame_RecursiveWriteDatamap((char *)pStruct + pField->offset + pField->embeddedStride*i, pDataMap, pStream);
		}

		return;
	}
	else if(pField->ops)
	{
		saverestore_fieldinfo_t info;
		info.pFieldObject = (char *)pStruct + pField->offset;
		info.pStruct = pStruct;
		info.pVariant = pField;

		pField->ops->Save(&info, pStream);
	}
	else
	{
		// typically it's arrays
		for(int i = 0; i < pField->fieldSize; i++)
			s_savedGameSaveFunctions[pField->type](pStruct, (char *)pStruct + pField->offset + typeSize[pField->type]*i, pStream);
	}
}

void SaveGame_RecursiveWriteDatamap(void* pStruct, datamap_t* pDataMap, IVirtualStream* pStream)
{
	// first we writing base data map
	if(pDataMap->baseMap)
		SaveGame_RecursiveWriteDatamap(pStruct, pDataMap->baseMap, pStream);

	for(int i = 0; i < pDataMap->m_dataNumFields; i++)
	{
		SaveGame_WriteField(pStruct, &pDataMap->m_fields[i], pStream);
	}
}

void SaveGame_WriteEntity(BaseEntity* pEntity, IVirtualStream* pStream)
{
	datamap_t* pEntityDataMap = pEntity->GetDataDescMap();

	int numFields = SaveGame_DataMapFieldCount(pEntityDataMap);

	pStream->Write(&numFields, 1, sizeof(int));

	SaveGame_RecursiveWriteDatamap(pEntity, pEntityDataMap, pStream);
}

#define MIN_SAVE_SIZE (8*1024*1024)

bool SaveGame_Write(const char* filename)
{
	EqString dest_file(varargs("SavedGames/%s.esv", filename));

	MsgInfo("Saving game to '%s'...\n", dest_file.GetData());

	IFile* pStream = g_fileSystem->Open(dest_file.GetData(), "wb", SP_MOD);

	if(!pStream)
		return false;

	save_header_t		save_hdr;

	pStream->Write(&save_hdr, 1, sizeof(save_hdr));
	
	save_hdr.ident		= SAVEGAME_IDENT;
	save_hdr.version		= SAVEGAME_VERSION;
	save_hdr.numEntities	= ents->numElem();

	pStream->Write(gpGlobals, 1, sizeof(GlobalVarsBase));

	// write entity class names for pre-creation
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = (BaseEntity*)ents->ptr()[i];

		if( !stricmp(pEnt->GetClassname(),"info_player_start") ||
			!stricmp(pEnt->GetClassname(),"tokamak_gamerules"))
		{
			continue;
		}

		char szClassName[128];
		strcpy(szClassName, pEnt->GetClassname());

		pStream->Write(szClassName, 1, 128);
	}

	// write all entity data
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = (BaseEntity*)ents->ptr()[i];

		if( !stricmp(pEnt->GetClassname(),"info_player_start") ||
			!stricmp(pEnt->GetClassname(),"tokamak_gamerules"))
		{
			save_hdr.numEntities--;
			continue;
		}

		SaveGame_WriteEntity(pEnt, pStream);
	}

	// save active events
	g_EventQueue.Save(NULL, pStream);

	int save_size = pStream->Tell();

	Msg("Save file size: %d\n", save_size);

	pStream->Seek(0, VS_SEEK_SET);
	pStream->Write(&save_hdr, 1, sizeof(save_hdr));
	pStream->Seek(save_size, VS_SEEK_SET);

	g_fileSystem->Close(pStream);

	MsgInfo("Successfully saved to '%s'...\n", dest_file.GetData());

	return true;
}

datavariant_t* FindDataVariantInMap_r(datamap_t *pMap, const char* fieldname)
{
	for(int i = 0; i < pMap->m_dataNumFields; i++)
	{
		if(pMap->m_fields[i].type == VTYPE_VOID)
			continue;

		if(!strcmp(pMap->m_fields[i].name, fieldname))
			return &pMap->m_fields[i];
	}

	if(pMap->baseMap)
	{
		return FindDataVariantInMap_r( pMap->baseMap, fieldname );
	}

	return NULL;
}

extern void SetWorldSpawn(BaseEntity* pEntity);

BaseEntity* GetEntByIndexRestore(int index, DkList<BaseEntity*> &list)
{
	if(index == -1)
		return NULL;

	for(int i = 0; i < list.numElem(); i++)
	{
		if(list[i]->EntIndex() == index)
			return list[i];
	}

	// invalid/not found
	return NULL;
}

void RestoreEntityPointers_r(datamap_t* pDataMap, void* pStruct, DkList<BaseEntity*> &entlist)
{
	for(int i = 0; i < pDataMap->m_dataNumFields; i++)
	{
		datavariant_t* pField = &pDataMap->m_fields[i];

		if(pField->type == VTYPE_EMBEDDED)
		{
			for(int j = 0; j < pField->fieldSize; j++)
			{
				RestoreEntityPointers_r(pField->customDatamap, (char *)pStruct + pField->offset + pField->embeddedStride*j, entlist);
			}
		}
		else if(pField->ops)
		{
			saverestore_fieldinfo_t info;
			info.pFieldObject = (char *)pStruct + pField->offset;
			info.pStruct = pStruct;
			info.pVariant = pField;

			pField->ops->Restore(&info, entlist);
		}
		else if(pField->type == VTYPE_ENTITYPTR)
		{
			for(int j = 0; j < pField->fieldSize; j++)
			{
				int ent_index = *(int*)((char *)pStruct + pField->offset + typeSize[pField->type]*j);

				BaseEntity** entity_ptr = (BaseEntity**)((char *)pStruct + pField->offset + typeSize[pField->type]*j);

				*entity_ptr = GetEntByIndexRestore(ent_index, entlist);
			}
		}
	}

	if(pDataMap->baseMap)
		RestoreEntityPointers_r(pDataMap->baseMap, pStruct, entlist);
}

void SaveGame_ReadField(void* pStruct, datavariant_t* pField, IVirtualStream* pStream)
{
	if(pField->type == VTYPE_EMBEDDED)
	{
		int field_count = 0;
		pStream->Read(&field_count, 1, sizeof(int));

		for(int i = 0; i < pField->fieldSize; i++)
		{
			for(int j = 0; j < field_count; j++)
			{
				save_field_t dpField;
				pStream->Read(&dpField, 1, sizeof(dpField));

				datavariant_t* pEmbField = FindDataVariantInMap_r( pField->customDatamap, dpField.name );

				if(!pField)
				{
					ErrorMsg("Fatal Error! Saved game is invalid due to some fields couldn't be found!\n");
					exit(-1);
				}

				SaveGame_ReadField((char *)pStruct + pField->offset + pField->embeddedStride*i, pEmbField, pStream);
			}
		}
	}
	else if(pField->ops)
	{
		saverestore_fieldinfo_t info;
		info.pFieldObject = (char *)pStruct + pField->offset;
		info.pStruct = pStruct;
		info.pVariant = pField;

		pField->ops->Read(&info, pStream);
	}
	else
	{
		// typically it's arrays
		for(int i = 0; i < pField->fieldSize; i++)
		{
			s_savedGameRestoreFunctions[pField->type](pStruct, (char *)pStruct + pField->offset + typeSize[pField->type]*i, pStream);
		}
	}
}

void SaveGame_LoadEntities(IVirtualStream* pStream, int numEntities)
{
	DkList<BaseEntity*> pEntityList;

	// pre-create entities first
	for(int i = 0; i < numEntities; i++)
	{
		BaseEntity *pCurrEnt = NULL;

		char szClassName[128];
		pStream->Read(szClassName, 1, 128);

		//Msg("Spawning: %s\n", szClassName);

		pCurrEnt = (BaseEntity*)entityfactory->CreateEntityByName(szClassName);
		if(!pCurrEnt)
		{
			MsgError("Can't create entity %s!\n", szClassName);
			return;
		}

		pEntityList.append(pCurrEnt);
	}

	// load entity datamaps
	for(int i = 0; i < numEntities; i++)
	{
		int field_count = 0;
		pStream->Read(&field_count, 1, sizeof(int));

		BaseEntity *pCurrEnt = pEntityList[i];

		for(int j = 0; j < field_count; j++)
		{
			save_field_t dpField;
			pStream->Read(&dpField, 1, sizeof(dpField));

			datavariant_t* pField = FindDataVariantInMap_r( pCurrEnt->GetDataDescMap(), dpField.name );

			if(!pField)
			{
				ErrorMsg("Fatal Error! Saved game is invalid due to some fields couldn't be found!\n");
				exit(-1);
			}

			SaveGame_ReadField( pCurrEnt, pField, pStream );
		}
	}

	// load event queues
	g_EventQueue.Read(NULL, pStream);
	g_EventQueue.Restore(NULL, pEntityList);

	// gamerules
	CreateGameRules( GAME_SINGLEPLAYER );

	for(int i = 0; i < pEntityList.numElem(); i++)
	{
		// restore pointer links
		datamap_t* pDataMap = pEntityList[i]->GetDataDescMap();
		RestoreEntityPointers_r(pDataMap, pEntityList[i], pEntityList);
	}

	for(int i = 0; i < pEntityList.numElem(); i++)
	{
		pEntityList[i]->Precache();

		pEntityList[i]->Activate();

		// set local player for this game
		if(!stricmp(pEntityList[i]->GetClassname(), "player"))
		{
			g_pGameRules->SetLocalPlayer( pEntityList[i] );
			//pEntityList[i]->SetIndex( 1 );
		}
	}

	for(int i = 0; i < pEntityList.numElem(); i++)
	{
		pEntityList[i]->SetIndex(pEntityList[i]->EntIndex());
	}

	Msg("Successfully restored game\n");
}

extern int first_game_update_frames;

struct saved_game_params_t
{
	IFile*		pStream;
	int			numEntities;
};

bool g_bIsRestoringSavedGame = false;

bool DoLoadSavedGame( void* pData )
{
	saved_game_params_t* params = (saved_game_params_t*)pData;

	// Initialize sound emitter system
	ses->Init();

	PrecachePhysicsSounds();

	gpGlobals->curtime = 0;
	gpGlobals->frametime = 0;

	SaveGame_LoadEntities( params->pStream, params->numEntities );

	engine->Reset();

	GAME_STATE_GameStartSession( true );

	engine->SetGameState(IEngineGame::GAME_IDLE);

	// close stream here
	g_fileSystem->Close( params->pStream );

	PPFree(params);

	g_bIsRestoringSavedGame = false;

	return true;
}

void SaveGame_RestoreFromFile(const char* filename)
{
	if(g_bIsRestoringSavedGame)	// this is for DoLoadSavedGame on thread issues...
		return;

	EqString dest_file(varargs("SavedGames/%s.esv", filename));

	Msg("Loading game from '%s'...\n", dest_file.GetData());

	IFile* pStream = g_fileSystem->Open(dest_file.GetData(), "rb", SP_MOD);

	if(!pStream)
	{
		Msg("Cannot load game from '%s'...\n", dest_file.GetData());
		return;
	}

	save_header_t header;
	pStream->Read(&header, 1, sizeof(header));

	if(header.ident != SAVEGAME_IDENT)
	{
		MsgError("Invalid savegame!\n");
		g_fileSystem->Close(pStream);
		return;
	}

	if(header.version != SAVEGAME_VERSION)
	{
		MsgError("Invalid savegame version!\n");
		g_fileSystem->Close(pStream);
		return;
	}

	pStream->Read(&gpGlobalsFile, 1, sizeof(gpGlobalsFile));

	DevMsg(2, "Version: %d\n", header.version);
	DevMsg(2, "World name: %s\n", gpGlobalsFile.worldname);
	DevMsg(2, "Game time: %f\n", gpGlobalsFile.curtime);
	DevMsg(2, "Engine time: %f\n", gpGlobalsFile.realtime);
	DevMsg(2, "Time scale: %f\n", gpGlobalsFile.timescale);
	DevMsg(2, "---------------\n");
	DevMsg(2, "Entity count: %d\n", header.numEntities);
	DevMsg(2, "---------------\n");

	// Try to remove all game entities
	engine->SetLevelName( gpGlobalsFile.worldname );

	g_pEngineHost->EnterResourceLoading();

	g_bIsRestoringSavedGame = true;

	// load in place if level is not changed
	if(!engine->IsLevelChanged())
	{
		GAME_STATE_GameEndSession();
		GAME_STATE_UnloadGameObjects();

		first_game_update_frames = 0;

		gpGlobals->curtime = 0;
		gpGlobals->frametime = 0;

		SaveGame_LoadEntities(pStream, header.numEntities);

		// close stream
		g_fileSystem->Close(pStream);

		GAME_STATE_GameStartSession( true );

		engine->Reset();

		g_bIsRestoringSavedGame = false;
	}
	else
	{
		engine->SetGameState(IEngineGame::GAME_NOTRUNNING);

		engine->UnloadGame( engine->IsLevelChanged() );

		saved_game_params_t* pParams = (saved_game_params_t*)PPAlloc(sizeof(saved_game_params_t));

		pParams->numEntities = header.numEntities;
		pParams->pStream = pStream;



		// send to engine
		engine->LoadSaveGame( DoLoadSavedGame, pParams );
	}

	g_pEngineHost->EndResourceLoading();
}

DECLARE_CMD(quicksave,"Makes quick save",0)
{
	SaveGame_Write("quick");
}

DECLARE_CMD(quickload,"Makes quick load from quick save",0)
{
	SaveGame_RestoreFromFile("quick");
}

DECLARE_CMD(save,"Saves game to specified file",0)
{
	if(args)
	{
		if(args->numElem() == 0)
		{
			goto usage;
		}

		if(!SaveGame_Write(args->ptr()[0].GetData()))
		{
			MsgError("Can't save to %s\n", args->ptr()[0].GetData());
		}
		else
		{
			MsgInfo("Game saved to %s\n", args->ptr()[0].GetData());
		}

		return;
	}
usage:
	MsgInfo("Usage: save <save name>\n");
}

DECLARE_CMD(load,"Loads game from specified file",0)
{
	if(args)
	{
		if(args->numElem() == 0)
		{
			goto usage;
		}

		SaveGame_RestoreFromFile(args->ptr()[0].GetData());

		return;
	}
usage:
	MsgInfo("Usage: load <save name>\n");
}