//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DkList saver
//////////////////////////////////////////////////////////////////////////////////

#ifndef SAVEGAME_DKLIST_H
#define SAVEGAME_DKLIST_H

#include "DataMap.h"

void SaveGame_WriteField(void* pStruct, datavariant_t* pField, IVirtualStream* pStream);
void SaveGame_ReadField(void* pStruct, datavariant_t* pField, IVirtualStream* pStream);
void RestoreEntityPointers_r(datamap_t* pDataMap, void* pStruct, DkList<BaseEntity*> &entlist);
BaseEntity* GetEntByIndexRestore(int index, DkList<BaseEntity*> &list);

// base save restore operators
template <class DKLISTTYPE, int FIELD_TYPE>
class CDkListOperators : public ISaveRestoreOperators
{
public:
	void Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
	{
		DKLISTTYPE* pList = (DKLISTTYPE*)info->pFieldObject;

		datavariant_t field_objects = 
		{
			(EVariableType)FIELD_TYPE,
			"elems",
			0,
			pList->numElem(),
			0,
			FIELDFLAG_SAVE,
			NULL,
			NULL,
			NULL
		};

		// write array element count
		int array_count = pList->numElem();
		pStream->Write(&array_count, 1, sizeof(int));

		SaveGame_WriteField( (char*)pList->ptr(), &field_objects, pStream );
	}

	void Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream )
	{
		DKLISTTYPE* pList = (DKLISTTYPE*)info->pFieldObject;

		int array_count = 0;
		pStream->Read(&array_count, 1, sizeof(int));

		datavariant_t field_objects = 
		{
			(EVariableType)FIELD_TYPE,
			"elems",
			0,
			array_count,
			0,
			FIELDFLAG_SAVE,
			NULL,
			NULL,
			NULL
		};

		//pList->clear();
		pList->setNum(array_count);

		// read unused field name
		save_field_t dpField;
		pStream->Read(&dpField, 1, sizeof(dpField));

		SaveGame_ReadField( (char*)pList->ptr(), &field_objects, pStream );
	}

	void Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist )
	{
		DKLISTTYPE* pList = (DKLISTTYPE*)info->pFieldObject;

		datavariant_t field_objects = 
		{
			(EVariableType)FIELD_TYPE,
			"elems",
			0,
			1,
			0,
			FIELDFLAG_SAVE,
			NULL,
			NULL,
			NULL
		};

		datavariant_t* pField = &field_objects;

		for(int i = 0; i < pList->numElem(); i++)
		{
			if(pField->type == VTYPE_EMBEDDED)
			{
				RestoreEntityPointers_r(pField->customDatamap, (char *)&pList->ptr()[i] + pField->offset, entlist);
			}
			else if(pField->ops)
			{
				saverestore_fieldinfo_t nestedInfo;
				nestedInfo.pFieldObject = (char *)&pList->ptr()[i] + pField->offset;
				nestedInfo.pStruct = &pList->ptr()[i];
				nestedInfo.pVariant = pField;

				pField->ops->Restore(&nestedInfo, entlist);
			}
			else if((pField->type == VTYPE_ENTITYPTR) && (pField->nFlags & FIELDFLAG_SAVE))
			{
				for(int j = 0; j < pField->fieldSize; j++)
				{
					int ent_index = *(int*)((char *)&pList->ptr()[i] + pField->offset);

					BaseEntity** entity_ptr = (BaseEntity**)((char *)&pList->ptr()[i] + pField->offset);

					*entity_ptr = GetEntByIndexRestore(ent_index, entlist);
				}
			}
		}
	}
};

template <int FIELD_TYPE>
class CDataopsInstantiator
{
public:
	template <class DKLISTTYPE>
	static ISaveRestoreOperators *GetDataOperators(DKLISTTYPE *)
	{
		static CDkListOperators<DKLISTTYPE, FIELD_TYPE> ops;
		return &ops;
	}
};

#endif // SAVEGAME_DKLIST_H