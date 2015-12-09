//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Event saver
//////////////////////////////////////////////////////////////////////////////////

#include "SaveGame_Events.h"

void SaveGame_RecursiveWriteDatamap(void* pStruct, datamap_t* pDataMap, IVirtualStream* pStream);
int SaveGame_DataMapFieldCount(datamap_t* pDataMap);

void CEventSaveRestoreOperators::Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
{
	CBaseEntityOutput* pOutput = (CBaseEntityOutput*)info->pFieldObject;
	DkList<CEventAction*>* actions = pOutput->GetActions();

	int event_count = actions->numElem();
	pStream->Write(&event_count, 1, sizeof(int));

	for(int i = 0; i < actions->numElem(); i++)
	{
		CEventAction* pAction = actions->ptr()[i];

		int numFields = SaveGame_DataMapFieldCount(pAction->GetDataDescMap());

		pStream->Write(&numFields, 1, sizeof(int));

		SaveGame_RecursiveWriteDatamap(pAction, pAction->GetDataDescMap(), pStream);
	}
}

void CEventSaveRestoreOperators::Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
{
	CBaseEntityOutput* pOutput = (CBaseEntityOutput*)info->pFieldObject;
	DkList<CEventAction*>* actions = pOutput->GetActions();

	int event_count = 0;
	pStream->Read(&event_count, 1, sizeof(int));

	for(int i = 0; i < event_count; i++)
	{
		int field_count = 0;
		pStream->Read(&field_count, 1, sizeof(int));

		CEventAction* pAction = new CEventAction();

		for(int j = 0; j < field_count; j++)
		{
			save_field_t dpField;
			pStream->Read(&dpField, 1, sizeof(dpField));

			datavariant_t* pField = FindDataVariantInMap_r( pAction->GetDataDescMap(), dpField.name );

			if(!pField)
			{
				ErrorMsg("Fatal Error! Saved game is invalid due to some fields couldn't be found!\n");
				exit(-1);
			}

			SaveGame_ReadField( pAction, pField, pStream );
		}

		actions->append(pAction);
	}
}

void CEventSaveRestoreOperators::Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist )
{

}