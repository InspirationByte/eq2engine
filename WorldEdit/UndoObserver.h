//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Undoable stuff
//
//				CObjectAction - object modification undo table, writes as level//////////////////////////////////////////////////////////////////////////////////

#ifndef UNDOOBSERVER_H
#define UNDOOBSERVER_H

#include "platform/Platform.h"
#include "Utils/EqString.h"
#include "Utils/DkList.h"
#include "IVirtualStream.h"

class CBaseEditableObject;

enum ActionType_e
{
	ACTION_MODIFY = 0,
	ACTION_CREATE,
	ACTION_REMOVE,
};

class CMemoryStream;

//---------------------------------
// Object action. For undo.
//---------------------------------
class CObjectAction
{
public:
	PPMEM_MANAGED_OBJECT();

									CObjectAction();
									~CObjectAction();

	// returns stream for this undo action
	IVirtualStream*					GetStream();

	// returns name of this undo action
	const char*						GetName();

	// sets new name for this undo action
	void							SetName(const char* pszName);

	// file dump, unsupported until now
	void							DumpToFile(const char* pszFileName);

	// begins object writing
	void							Begin();

	// adds object to the undo table (use after begin)
	void							AddObject(CBaseEditableObject* pObject);

	// closes stream
	void							End();

	void							SetObjectIsCreated() {m_bObjectIsCreated = true;}
	bool							IsObjectCreated() {return m_bObjectIsCreated;}

	DkList<int>						m_objectIDS;

protected:
	bool							m_bReady;

	CMemoryStream*					m_pStream;
	EqString						m_szActionName;

	bool							m_bObjectIsCreated;

	DkList<CBaseEditableObject*>	m_pTempWriteObjects;
};

#endif // UNDOOBSERVER_H