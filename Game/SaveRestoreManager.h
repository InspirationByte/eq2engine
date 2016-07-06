//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Save/Restore manager
//////////////////////////////////////////////////////////////////////////////////

#ifndef SAVERESTOREMANAGER_H
#define SAVERESTOREMANAGER_H

#include "VirtualStream.h"
#include "utils/DkList.h"

struct datavariant_t;
struct datamap_t;

struct saverestore_fieldinfo_t
{
	void*			pStruct;
	void*			pFieldObject;

	datavariant_t*	pVariant;
};

// saved field structure
struct save_field_t
{
	char	name[43];
	ubyte	type;		// only for checking the saved game for entity version

	// value after this structure
};

class BaseEntity;

// base save restore operators
// uses virtual streams for writing and reading information
class ISaveRestoreOperators
{
public:
	virtual void Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream ) {}
	virtual void Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream ) {}
	virtual void Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist) {}
};

datavariant_t*	FindDataVariantInMap_r(datamap_t *pMap, const char* fieldname);
void			SaveGame_ReadField(void* pStruct, datavariant_t* pField, IVirtualStream* pStream);
void			SaveGame_RestoreFromFile(const char* filename);
bool			SaveGame_Write(const char* filename);

#endif // SAVERESTOREMANAGER_H