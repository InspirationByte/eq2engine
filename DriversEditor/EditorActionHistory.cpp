//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Undoable object for editor
//////////////////////////////////////////////////////////////////////////////////

#include "EditorActionHistory.h"
#include "EditorMain.h"

static CEditorActionObserver s_editActionObserver;
CEditorActionObserver* g_pEditorActionObserver = &s_editActionObserver;

undoableData_t::undoableData_t()
{
	changesStream.Open(NULL,VS_OPEN_WRITE | VS_OPEN_READ, 128);
}

void undoableData_t::Clear()
{
	changesStream.Close();
	changesStream.Open(NULL, VS_OPEN_WRITE | VS_OPEN_READ, 128);

	if (object)
		object->m_modifyMark = 0;
}

//--------------------------------------------------

CUndoableObject::CUndoableObject() : m_modifyMark(0)
{
}

//----------------------------------------------------------------------------------

CEditorActionObserver::CEditorActionObserver()
{
	m_curEvent = 0;
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

void CEditorActionObserver::ClearHistory()
{
	for (int i = 0; i < m_events.numElem(); i++)
		delete m_events[i];
	m_events.clear();

	for (int i = 0; i < m_tracking.numElem(); i++)
		delete m_tracking[i];
	m_tracking.clear();

	m_editing.clear();

	m_curEvent = 0;
	m_actionContextId = 0;
	m_activeEvent = nullptr;
}

void CEditorActionObserver::Undo()
{
	m_curEvent = max(m_curEvent, 0);

	// first execute HIST_ACT_CREATION
	if (m_events.inRange(m_curEvent))
	{
		actionEvent_t* evt = m_events[m_curEvent];

		for (int i = 0; i < evt->states.numElem(); i++)
		{
			histState_t& state = evt->states[i];

			undoableData_t* undoable = state.subject;

			// execute deletion on creation event
			if (state.type == HIST_ACT_CREATION)
			{
				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, HIST_ACT_DELETION);

				if (undoable->object)
					undoable->object->Undoable_Remove();
				undoable->object = nullptr;
			}
			else if (state.type == HIST_ACT_DELETION)
			{
				undoable->changesStream.Seek(state.streamStart, VS_SEEK_SET);

				if (!undoable->object)
					undoable->object = (undoable->undeleteFunc)(&undoable->changesStream);

				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, HIST_ACT_CREATION);
			}
		}
	}

	// step down
	m_curEvent--;

	// execute everything else than creation
	if (m_events.inRange(m_curEvent))
	{
		actionEvent_t* evt = m_events[m_curEvent];
		for (int i = 0; i < evt->states.numElem(); i++)
		{
			histState_t& state = evt->states[i];

			undoableData_t* undoable = state.subject;

			if (state.type == HIST_ACT_MODIFY || state.type == HIST_ACT_STOREINIT || state.type == HIST_ACT_CREATION)
			{
				undoable->changesStream.Seek(state.streamStart, VS_SEEK_SET);

				// create object if necessary
				if (!undoable->object)
					undoable->object = (undoable->undeleteFunc)(&undoable->changesStream);
				else
					undoable->object->Undoable_ReadObjectData(&undoable->changesStream);

				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, state.type == HIST_ACT_STOREINIT ? HIST_ACT_MODIFY : state.type);
			}
		}
	}

	g_pMainFrame->OnHistoryEvent(nullptr, HIST_ACT_COMPLETED);
}

void CEditorActionObserver::Redo()
{
	m_curEvent = min(m_curEvent, m_events.numElem()-1);

	// first execute HIST_ACT_DELETION
	if (m_events.inRange(m_curEvent))
	{
		actionEvent_t* evt = m_events[m_curEvent];
		for (int i = 0; i < evt->states.numElem(); i++)
		{
			histState_t& state = evt->states[i];

			undoableData_t* undoable = state.subject;

			// execute deletion on deletion event
			if (state.type == HIST_ACT_DELETION)
			{
				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, HIST_ACT_DELETION);

				if (undoable->object)
					undoable->object->Undoable_Remove();
				undoable->object = nullptr;
			}
		}
	}

	// step up
	m_curEvent++;

	// execute everything else than deletion
	if (m_events.inRange(m_curEvent))
	{
		actionEvent_t* evt = m_events[m_curEvent];
		for (int i = 0; i < evt->states.numElem(); i++)
		{
			histState_t& state = evt->states[i];

			undoableData_t* undoable = state.subject;

			if (state.type == HIST_ACT_DELETION)
			{
				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, HIST_ACT_DELETION);

				if (undoable->object)
					undoable->object->Undoable_Remove();
				undoable->object = nullptr;
			}
			else if (state.type == HIST_ACT_CREATION)
			{
				undoable->changesStream.Seek(state.streamStart, VS_SEEK_SET);

				if (!undoable->object)
					undoable->object = (undoable->undeleteFunc)(&undoable->changesStream);

				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, HIST_ACT_CREATION);
			}
			else if (state.type == HIST_ACT_MODIFY || state.type == HIST_ACT_STOREINIT)
			{
				undoable->changesStream.Seek(state.streamStart, VS_SEEK_SET);

				// create object if necessary
				if (!undoable->object)
					undoable->object = (undoable->undeleteFunc)(&undoable->changesStream);
				else
					undoable->object->Undoable_ReadObjectData(&undoable->changesStream);

				// do tool events
				g_pMainFrame->OnHistoryEvent(undoable->object, HIST_ACT_MODIFY);
			}
		}
	}

	g_pMainFrame->OnHistoryEvent(nullptr, HIST_ACT_COMPLETED);
}

static const char* eventTypeStr[] = {
	"CREATION",
	"DELETION",
	"MODIFY",
	"STOREINIT",
};

void CEditorActionObserver::DebugDisplay()
{
	debugoverlay->Text(color4_white, "actions: %d\n", m_events.numElem() );
	debugoverlay->Text(color4_white, "currentAct: %d\n", m_curEvent);
	for (int i = 0; i < m_events.numElem(); i++)
	{
		actionEvent_t* ev = m_events[i];

		debugoverlay->Text(color4_white, "event %2d %s %s\n", ev->id, eventTypeStr[ev->states[0].type], m_curEvent==i? "X" : "-");
	}
}

//---------------------------------------------------
// actions
//---------------------------------------------------

void CEditorActionObserver::OnCreate( CUndoableObject* object )
{
	// rewind if user did Undo
	RewindEvents();

	EnsureActiveEvent();
	RecordState(m_activeEvent, object, HIST_ACT_CREATION);
}

void CEditorActionObserver::OnDelete( CUndoableObject* object )
{
	// rewind if user did Undo
	RewindEvents();

	EnsureActiveEvent();
    undoableData_t* data = RecordState(m_activeEvent, object, HIST_ACT_DELETION);
	data->object = nullptr;
}

void CEditorActionObserver::RewindEvents()
{
	// don't perform rewind while has active event
	if (m_activeEvent)
		return;

	// before we drop future events -
	// tracked objects must be removed
	for (int i = 0; i < m_tracking.numElem(); i++)
	{
		if (!HasHistoryToCurrentEvent(m_tracking[i]->object))
		{
			delete m_tracking[i];
			m_tracking.fastRemoveIndex(i);
			i--;
		}
	}

	for (int i = m_curEvent+1; i < m_events.numElem(); i++)
		delete m_events[i];

	if (m_curEvent == 0)
		m_events.clear();
	else
		m_events.setNum(m_curEvent+1);
}

undoableData_t*	CEditorActionObserver::TrackUndoable(CUndoableObject* object)
{
	undoableData_t* data = nullptr;
	for (int i = 0; i < m_tracking.numElem(); i++)
	{
		if (m_tracking[i]->object == object)
		{
			data = m_tracking[i];
			break;
		}
	}

	// add undoable tracking data
	if (!data)
	{
		data = new undoableData_t();
		data->object = object;
		data->undeleteFunc = object->Undoable_GetFactoryFunc();

		m_tracking.append(data);
	}

	return data;
}

bool CEditorActionObserver::IsTrackedUndoable(CUndoableObject* object)
{
	for (int i = 0; i < m_tracking.numElem(); i++)
	{
		if (m_tracking[i]->object == object)
			return true;
	}
	return false;
}

histState_t* CEditorActionObserver::GetLastHistoryOn(CUndoableObject* object, int& stepsAway, int skipEvents /*= 0*/)
{
	stepsAway = 0;

	if (!m_events.numElem())
		return nullptr;

	for (int i = m_curEvent; i >= 0; i--)
	{
		actionEvent_t* evt = m_events[i];
		for (int j = 0; j < evt->states.numElem(); j++)
		{
			histState_t& state = evt->states[j];

			if (state.subject->object == object)
			{
				if (skipEvents-- <= 0)
					return &state;
				else
					break;
			}
			else
				stepsAway++;
		}
	}

	return nullptr;
}

bool CEditorActionObserver::HasHistoryToCurrentEvent(CUndoableObject* object)
{
	for (int i = 0; i < m_curEvent+1; i++)
	{
		actionEvent_t* evt = m_events[i];
		for (int j = 0; j < evt->states.numElem(); j++)
		{
			histState_t& state = evt->states[j];

			if (state.subject->object == object)
				return true;
		}
	}

	return false;
}

void CEditorActionObserver::EnsureActiveEvent()
{
	if (m_activeEvent)
		return;

	// make a new event
	m_activeEvent = new actionEvent_t;
	m_activeEvent->id = m_actionContextId;
}

undoableData_t* CEditorActionObserver::RecordState(actionEvent_t* storeTo, CUndoableObject* object, EHistoryAction type)
{
	undoableData_t* data = TrackUndoable(object);

	// add the state to the active event
	histState_t ev;
	ev.subject = data;
	ev.type = type;
	ev.streamStart = data->changesStream.Tell();

	storeTo->states.append(ev);

	// store object
	object->Undoable_WriteObjectData(&data->changesStream);

	return data;
}

void CEditorActionObserver::BeginModify( CUndoableObject* object )
{
	if (object->m_modifyMark > 0)
	{
		object->m_modifyMark++;
		return;
	}

	object->m_modifyMark++;

	// rewind if user did Undo
	RewindEvents();

	// Before first action object must be recorded.
	// It's required for backwards action
	int stateStepsAway = 0;
	histState_t* lastState = GetLastHistoryOn(object, stateStepsAway, 0);
	if (!lastState)
	{
		EnsureActiveEvent();

		if (!m_events.numElem())
		{
			m_curEvent = m_events.addUnique(m_activeEvent);
			m_activeEvent = nullptr;
		}

		// push into last action (or new one above)
		RecordState(m_events[m_curEvent], object, HIST_ACT_STOREINIT);
	}
	else if (lastState->type != HIST_ACT_DELETION && stateStepsAway > 0)
	{
		// copy last history state
		histState_t clonedState;
		clonedState.type = HIST_ACT_MODIFY;
		clonedState.subject = lastState->subject;
		clonedState.streamStart = lastState->streamStart;

		// add to previous event only
		m_events[m_curEvent]->states.append(clonedState);
	}

	EnsureActiveEvent();

	undoableData_t* data = TrackUndoable(object);
	m_editing.append(data);
}

void CEditorActionObserver::EndAction()
{
	if (!m_activeEvent)
		return;
		
	// unlock objects
	for (int i = 0; i < m_editing.numElem(); i++)
	{
		undoableData_t* data = RecordState(m_activeEvent, m_editing[i]->object, HIST_ACT_MODIFY);
		m_editing[i]->object->m_modifyMark = 0;
	}
	m_editing.clear();

	if (m_activeEvent->states.numElem())
		m_curEvent = m_events.append(m_activeEvent);
	else
		delete m_activeEvent;

	m_actionContextId++;
	m_activeEvent = nullptr;
}

int CEditorActionObserver::GetUndoSteps() const
{
	return m_curEvent;
}

int CEditorActionObserver::GetRedoSteps() const
{
	return m_events.numElem() - m_curEvent;
}