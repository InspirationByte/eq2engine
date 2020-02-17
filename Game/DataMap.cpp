//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity data table for use in savegames and restoring from
//				savegames / parsing entity KVs
//////////////////////////////////////////////////////////////////////////////////

#include "DataMap.h"
#include "BaseEntity.h"

#include "utils/strtools.h"

bool variable_t::Convert( EVariableType newType )
{
	if ( newType == varType )
		return true;

	// there VOID is NULL
	if ( newType == VTYPE_VOID )
	{
		iVal = 0;
		newType = VTYPE_VOID;
		return true;
	}

	switch ( varType )
	{
		case VTYPE_INTEGER:
		{
			switch ( newType )
			{
				case VTYPE_FLOAT:
				{
					SetFloat( (float)iVal );
					return true;
				}
				case VTYPE_BOOLEAN:
				{
					SetBool( iVal > 0 );
					return true;
				}
			}
			break;
		}
		case VTYPE_FLOAT:
		{
			switch ( newType )
			{
				case VTYPE_INTEGER:
				{
					SetInt((int)flVal);
					return true;
				}
				case VTYPE_BOOLEAN:
				{
					SetBool( flVal > 0 );
					return true;
				}
			}
			break;
		}
		// the most types are converted from string
		case VTYPE_STRING:
		{
			switch ( newType )
			{
				case VTYPE_INTEGER:
				{
					if (pszVal[0])
						SetInt(atoi(pszVal));
					else
						SetInt(0);

					return true;
				}
				case VTYPE_FLOAT:
				{
					if (pszVal[0])
						SetFloat( atof(pszVal) );
					else
						SetFloat(0);

					return true;
				}
				case VTYPE_BOOLEAN:
				{
					if (pszVal[0])
						SetBool( atoi(pszVal) > 0 );
					else
						SetBool(false);

					return true;
				}
				case VTYPE_VECTOR3D:
				{
					Vector3D tmpVec = vec3_zero;

					if (sscanf(pszVal, "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
						sscanf(pszVal, "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);

					SetVector3D( tmpVec );
					return true;
				}

			}
		
			break;
		}
	}

	// invalid conversion
	return false;
}

// make queue global
CEventQueue	g_EventQueue;

//
// Event action
//
CEventAction::CEventAction( char* pszEditorActionData )
{
	// subdivide the string
	DkList<EqString> strings;
	strings.resize(5);

	xstrsplit( pszEditorActionData, ",", strings );

	m_szTargetInput = strings[0];
	m_szTargetName = strings[1];
	m_nTimesToFire = atoi(strings[2].GetData());
	m_fDelay = atof(strings[3].GetData());

	if(strings.numElem() > 4)
		m_szParameter = strings[4];
	else
		m_szParameter = "";
}

BEGIN_DATAMAP_NO_BASE( CEventAction )
	DEFINE_FIELD(m_szTargetName, VTYPE_STRING),
	DEFINE_FIELD(m_szTargetInput, VTYPE_STRING),
	DEFINE_FIELD(m_szParameter, VTYPE_STRING),
	DEFINE_FIELD(m_fDelay, VTYPE_FLOAT),
	DEFINE_FIELD(m_nTimesToFire, VTYPE_INTEGER),
END_DATAMAP();

static CEventSaveRestoreOperators event_ops;
ISaveRestoreOperators* g_pEventSaveRestore = &event_ops;

//
// Entity output
//

CBaseEntityOutput::~CBaseEntityOutput()
{
	Clear();
}

void CBaseEntityOutput::Clear()
{
	for(int i = 0; i < m_pActionList.numElem(); i++)
		delete m_pActionList[i];

	m_pActionList.clear();
}

void CBaseEntityOutput::AddAction(char* pszEditorActionData)
{
	CEventAction* pAction = new CEventAction(pszEditorActionData);
	m_pActionList.append(pAction);
}

void CBaseEntityOutput::FireOutput( variable_t& Value, BaseEntity* pActivator, BaseEntity* pCaller, float fDelay)
{
	for(int i = 0; i < m_pActionList.numElem(); i++)
	{
		CEventAction* pAction = m_pActionList[i];

		if(pAction->m_szParameter.Length() == 0)
		{
			g_EventQueue.AddEvent(pAction->m_szTargetName.GetData(), pAction->m_szTargetInput.GetData(), Value, fDelay + pAction->m_fDelay, pAction->m_nTimesToFire, pActivator, pCaller);
		}
		else
		{
			variable_t valueOverride;
			valueOverride.SetString(pAction->m_szParameter.GetData());

			int len = strlen(Value.GetString());

			if((len == 0) || (Value.GetVarType() == VTYPE_VOID) )
				g_EventQueue.AddEvent(pAction->m_szTargetName.GetData(), pAction->m_szTargetInput.GetData(), valueOverride, fDelay + pAction->m_fDelay, pAction->m_nTimesToFire, pActivator, pCaller);
			else
				g_EventQueue.AddEvent(pAction->m_szTargetName.GetData(), pAction->m_szTargetInput.GetData(), Value, fDelay + pAction->m_fDelay, pAction->m_nTimesToFire, pActivator, pCaller);
		}

		// check fire times, and decrement if there limit was set
		if(pAction->m_nTimesToFire == -1)
			continue;

		pAction->m_nTimesToFire--;

		if(pAction->m_nTimesToFire == 0)
		{
			delete m_pActionList[i];
			m_pActionList.removeIndex(i);
			i--;
		}
	}
}

class CVariableSaveRestoreOperators : public ISaveRestoreOperators
{
public:
	void Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
	{
		variable_t* pVariable = (variable_t*)info->pFieldObject;
		pStream->Write(pVariable, 1, sizeof(variable_t));
	}

	void Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
	{
		variable_t* pVariable = (variable_t*)info->pFieldObject;
		pStream->Read(pVariable, 1, sizeof(variable_t));
	}

	void Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist )
	{

	}
};

static CVariableSaveRestoreOperators g_variableSaveRestore;

struct EventQueueEvent_t
{
	EqString	szTargetName;
	EqString	szTargetInput;

	variable_t	parameter;

	float		fFireTime;

	BaseEntity*	pActivator;
	BaseEntity*	pCaller;

	DECLARE_DATAMAP();
};

BEGIN_DATAMAP_NO_BASE(EventQueueEvent_t)
	DEFINE_FIELD(szTargetName, VTYPE_STRING),
	DEFINE_FIELD(szTargetInput, VTYPE_STRING),

	DEFINE_CUSTOMFIELD(parameter, &g_variableSaveRestore),

	DEFINE_FIELD(fFireTime, VTYPE_TIME),
	DEFINE_FIELD(pActivator, VTYPE_ENTITYPTR),
	DEFINE_FIELD(pCaller, VTYPE_ENTITYPTR),
END_DATAMAP();

void CEventQueue::Init()
{
	Clear();
}

// clear for shutdown
void CEventQueue::Clear()
{
	for(int i = 0; i < m_pEventList.numElem(); i++)
		delete m_pEventList[i];

	m_pEventList.clear();
}

// fire events whose time has come
void CEventQueue::ServiceEvents()
{
	for(int i = 0; i < m_pEventList.numElem(); i++)
	{
		if (m_pEventList[i]->fFireTime > gpGlobals->curtime)
			continue;

		// FIRE THIS!
		EventQueueEvent_t* pEvent = m_pEventList[i];

		bool bTargetFound = false;
		BaseEntity* pEnt = nullptr;

		while(true)
		{
			pEnt = (BaseEntity*)UTIL_EntByName(pEvent->szTargetName.GetData(), pEnt);

			if(!pEnt)
				break;

			pEnt->AcceptInput((char*)pEvent->szTargetInput.GetData(), pEvent->parameter, pEvent->pActivator, pEvent->pCaller);

			//Msg("AcceptInput\n");

			bTargetFound = true;
		}

		if(!bTargetFound)
			DevMsg( 1, "Output ERROR: Entity with name '%s' not found\n", pEvent->szTargetName.GetData());

		// and remove
		m_pEventList.fastRemoveIndex(i);
		i--;
	}
}

// adds new event action
void CEventQueue::AddEvent(const char* pszTargetName,const char* pszTargetInput, variable_t& value, float fDelay, int nTimesToFire, BaseEntity *pActivator, BaseEntity *pCaller)
{
	EventQueueEvent_t* pEvent = new EventQueueEvent_t;

	pEvent->fFireTime = gpGlobals->curtime+fDelay;
	pEvent->pActivator = pActivator;
	pEvent->pCaller = pCaller;
	pEvent->szTargetName = pszTargetName;
	pEvent->szTargetInput = pszTargetInput;
	pEvent->parameter = value;

	m_pEventList.append( pEvent );
}

void SaveGame_RecursiveWriteDatamap(void* pStruct, datamap_t* pDataMap, IVirtualStream* pStream);
int SaveGame_DataMapFieldCount(datamap_t* pDataMap);

void CEventQueue::Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
{
	int event_count = m_pEventList.numElem();
	pStream->Write(&event_count, 1, sizeof(int));

	for(int i = 0; i < m_pEventList.numElem(); i++)
	{
		EventQueueEvent_t* pEvent = m_pEventList[i];

		int numFields = SaveGame_DataMapFieldCount(pEvent->GetDataDescMap());

		pStream->Write(&numFields, 1, sizeof(int));

		SaveGame_RecursiveWriteDatamap(pEvent, pEvent->GetDataDescMap(), pStream);
	}
}

void CEventQueue::Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
{
//	CBaseEntityOutput* pOutput = (CBaseEntityOutput*)info->pFieldObject;

	int event_count = 0;
	pStream->Read(&event_count, 1, sizeof(int));

	for(int i = 0; i < event_count; i++)
	{
		int field_count = 0;
		pStream->Read(&field_count, 1, sizeof(int));

		EventQueueEvent_t* pEvent = new EventQueueEvent_t();

		for(int j = 0; j < field_count; j++)
		{
			save_field_t dpField;
			pStream->Read(&dpField, 1, sizeof(dpField));

			datavariant_t* pField = FindDataVariantInMap_r( pEvent->GetDataDescMap(), dpField.name );

			if(!pField)
			{
				ErrorMsg("Fatal Error! Saved game is invalid due to some fields couldn't be found!\n");
				exit(-1);
			}

			SaveGame_ReadField( pEvent, pField, pStream );
		}

		m_pEventList.append(pEvent);
	}
}

void CEventQueue::Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist)
{
	for(int i = 0; i < m_pEventList.numElem(); i++)
	{
		RestoreEntityPointers_r(m_pEventList[i]->GetDataDescMap(), m_pEventList[i], entlist);
	}
}
