//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Undoable stuff
//
//				CObjectAction - object modification undo table, writes as level//////////////////////////////////////////////////////////////////////////////////

#include "UndoObserver.h"
#include "DebugInterface.h"
#include "EditorLevel.h"
#include "BaseEditableObject.h"
#include "utils/VirtualStream.h"

//---------------------------------
// Object action. For undo.
//---------------------------------
CObjectAction::CObjectAction()
{
	m_pStream = (CMemoryStream*)OpenMemoryStream(VS_OPEN_READ | VS_OPEN_WRITE, 0, NULL);
	m_bObjectIsCreated = false;
}

CObjectAction::~CObjectAction()
{
	delete m_pStream;
}

// returns stream for this undo action
IVirtualStream* CObjectAction::GetStream()
{
	return m_pStream;
}

// returns name of this undo action
const char* CObjectAction::GetName()
{
	return m_szActionName.GetData();
}

// sets new name for this undo action
void CObjectAction::SetName(const char* pszName)
{
	m_szActionName = pszName;
}

// file dump, unsupported until now
void CObjectAction::DumpToFile(const char* pszFileName)
{
	MsgError("CObjectAction::DumpToFile doesnt work for now\n");
}

// begins object writing
void CObjectAction::Begin()
{
	m_pTempWriteObjects.clear();
	m_objectIDS.clear();
	m_bReady = false;
}

// adds object to the undo table (use after begin)
void CObjectAction::AddObject(CBaseEditableObject* pObject)
{
	if(m_bReady)
		return;

	m_pTempWriteObjects.append(pObject);
	m_objectIDS.append(pObject->GetObjectID());
}

// closes stream
void CObjectAction::End()
{
	g_pLevel->SaveObjects(m_pStream, m_pTempWriteObjects.ptr(), m_pTempWriteObjects.numElem());

	m_pStream->Seek(0,VS_SEEK_SET);

	m_bReady = true;
	m_pTempWriteObjects.clear();
}