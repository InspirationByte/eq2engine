//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Event saver
//////////////////////////////////////////////////////////////////////////////////

#ifndef SAVEGAME_EVENTS_H
#define SAVEGAME_EVENTS_H

#include "DataMap.h"

class CEventSaveRestoreOperators : public ISaveRestoreOperators
{
public:
	void Save( saverestore_fieldinfo_t* info, IVirtualStream* pStream );

	void Read( saverestore_fieldinfo_t* info, IVirtualStream* pStream );

	void Restore( saverestore_fieldinfo_t* info, DkList<BaseEntity*> &entlist );
};

#endif // SAVEGAME_EVENTS_H