//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Undoable object for editor
//////////////////////////////////////////////////////////////////////////////////

#include "EditorActionHistory.h"

static CEditorActionObserver s_editActionObserver;
CEditorActionObserver* g_pEditorActionObserver = &s_editActionObserver;

#define NO_HISTORY -1

CUndoableObject::CUndoableObject()
{
	m_curHist = NO_HISTORY;
	m_changesStream.Open(NULL,VS_OPEN_WRITE | VS_OPEN_READ, 128);
	m_modifyMark = 0;
}

uint CUndoableObject::Undoable_PushCurrent()
{
	uint oldOfs = m_changesStream.Tell();

	int newOfs = 0;

	// if we has something on stack
	if(m_histOffsets.numElem() > 0 )
	{
		if(m_curHist == NO_HISTORY)
			m_curHist = 0;

		// if we somewhere in a middle of history
		if(m_curHist < m_histOffsets.numElem()-1)
		{
			// choose the start of the object to be overwritten
			newOfs = m_histOffsets[m_curHist].start;

			// remove 'parallel' future lines of history
			m_histOffsets.setNum(m_curHist+1);
		}
		else // just begin from the end
			newOfs = m_histOffsets[m_curHist].end;

		m_changesStream.Seek( newOfs, VS_SEEK_SET );
	}

	// now write our object state to stream
	if(!Undoable_WriteObjectData( &m_changesStream ))
		return -1;

	// add the history block
	histBlock_t hist;
	hist.start = newOfs;
	hist.end = m_changesStream.Tell();

	m_histOffsets.append(hist);
	m_curHist++;

	return oldOfs;
}

bool CUndoableObject::Undoable_PopBack(bool undoCase)
{
	if(m_curHist == NO_HISTORY)
		return false;	// nothing to do

	// seek to the beginning of data
	m_changesStream.Seek( m_histOffsets[m_curHist].start, VS_SEEK_SET );
	
	Undoable_ReadObjectData(&m_changesStream);

	if(!undoCase)
		m_histOffsets.setNum(m_curHist+1);

	m_curHist--;

	return true;
}

bool CUndoableObject::Undoable_Redo()
{
	if(m_histOffsets.numElem() == 0)
		return false;	// nothing to do

	if(m_curHist == NO_HISTORY)
		m_curHist = 0;

	WarningMsg("Cur hist: %d, total: %d\n", m_curHist, m_histOffsets.numElem());

	// no redo
	if(m_curHist > m_histOffsets.numElem()-1)
		return false;

	m_curHist++;

	m_modifyMark = 0;

	m_changesStream.Seek( m_histOffsets[m_curHist].start, VS_SEEK_SET );
	Undoable_ReadObjectData(&m_changesStream);
	
	return true;
}

int	CUndoableObject::Undoable_GetChangeCount() const
{
	return m_histOffsets.numElem();
}

void CUndoableObject::Undoable_ClearHistory()
{
	m_changesStream.Close();
	m_changesStream.Open(NULL,VS_OPEN_WRITE | VS_OPEN_READ, 128);
	m_histOffsets.clear();
	m_curHist = NO_HISTORY;
	m_modifyMark = 0;
}

//----------------------------------------------------------------------------------

CEditorActionObserver::CEditorActionObserver()
{
	m_curHist = NO_HISTORY;
	m_actionContextId = 0;
}

CEditorActionObserver::~CEditorActionObserver()
{

}

void CEditorActionObserver::OnLevelLoad()
{

}

void CEditorActionObserver::OnLevelUnload()
{
	ClearHistory();
}

void CEditorActionObserver::SaveHistory()
{

}

void CEditorActionObserver::ClearHistory()
{
	for(int i = 0; i < m_events.numElem(); i++)
		m_events[i].object->Undoable_ClearHistory();

	m_events.clear();

	m_curHist = NO_HISTORY;
}

void CEditorActionObserver::Undo()
{
	if(m_events.numElem() == 0)
		return;	// nothing to do

	int contextId = -1;

	while(m_curHist != NO_HISTORY)
	{
		histEvent_t& evt = m_events[m_curHist];

		if(contextId == -1)
		{
			// give the context
			contextId = evt.context;
		}
		else
		{
			// stop if there is different context
			if(evt.context != contextId)
				return;
		}

		evt.object->Undoable_PopBack(true);
		m_curHist--;
	}
}

void CEditorActionObserver::Redo()
{
	if(m_events.numElem() == 0)
		return;	// nothing to do

	if(m_curHist == NO_HISTORY)
		m_curHist = 0;

	// no redo
	if(m_curHist+1 > m_events.numElem()-1)
		return;

	m_curHist++;
	CUndoableObject* undoable = m_events[m_curHist].object;
	undoable->Undoable_Redo();
}

//---------------------------------------------------
// actions
//---------------------------------------------------

void CEditorActionObserver::OnCreate( CUndoableObject* object )
{
	ASSERTMSG(false, "CEditorActionObserver::OnCreate not implemented");
}

void CEditorActionObserver::OnDelete( CUndoableObject* object )
{
	ASSERTMSG(false, "CEditorActionObserver::OnDelete not implemented");
}

void CEditorActionObserver::BeginModify( CUndoableObject* object )
{
	if (object->m_modifyMark > 0)
	{
		object->m_modifyMark++;
		return;
	}

	object->m_modifyMark++;

	// if we has something on stack
	if(m_events.numElem() > 0)
	{
		if(m_curHist == NO_HISTORY)
			m_curHist = 0;

		// if we somewhere in a middle of history
		if(m_curHist < m_events.numElem()-1)
		{
			// remove 'parallel' future lines of history
			m_events.setNum(m_curHist+1);
		}
	}

	// add the history block
	histEvent_t ev;
	ev.object = object;
	ev.type = HIST_ACT_MODIFY;
	ev.context = m_actionContextId;
	ev.streamStart = object->Undoable_PushCurrent();

	m_events.append(ev);
	m_curHist++;

	m_tracking.append(object);
}

void CEditorActionObserver::EndModify()
{
	if (!m_tracking.numElem())
		return;

	// OK
	for(int i = 0; i < m_tracking.numElem(); i++)
		m_tracking[i]->m_modifyMark = false;

	m_actionContextId++;

	m_tracking.clear();
}

void CEditorActionObserver::CancelModify()
{
	for (int i = 0; i < m_tracking.numElem(); i++)
	{
		CUndoableObject* obj = m_tracking[i];
		obj->m_modifyMark = false;
	}

	m_tracking.clear();

	Redo();
}

int CEditorActionObserver::GetUndoSteps() const
{
	return m_curHist;
}

int CEditorActionObserver::GetRedoSteps() const
{
	return m_events.numElem() - m_curHist;
}