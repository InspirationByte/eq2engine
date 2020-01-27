//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Undoable object for editor
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORACTIONHISTORY_H
#define EDITORACTIONHISTORY_H

#include "utils/VirtualStream.h"
#include "utils/DkList.h"

enum EHistoryAction
{
	HIST_ACT_CREATION = 0,	// object created by user
	HIST_ACT_DELETION,		// object deleted by user

	HIST_ACT_MODIFY,		// object modified
	HIST_ACT_STOREINIT,		// object intially stored before modifying

	HIST_ACT_COMPLETED,		// only used in OnHistoryEvent
};

class CUndoableObject;
typedef CUndoableObject* (*UndoableFactoryFunc)(IVirtualStream* stream);

struct undoableData_t
{
	undoableData_t();
	
	void					Clear();

	CMemoryStream			changesStream;
	CUndoableObject*		object;
	UndoableFactoryFunc		undeleteFunc;
};

// Undoable object itself. Contains history
class CUndoableObject
{
	friend class CEditorActionObserver;
	friend struct undoableData_t;

public:
							CUndoableObject();
	virtual					~CUndoableObject() {}

	int						m_modifyMark;

protected:
	virtual UndoableFactoryFunc	Undoable_GetFactoryFunc() = 0;
	virtual void				Undoable_Remove() = 0;
	virtual bool				Undoable_WriteObjectData( IVirtualStream* stream ) = 0;	// writing object
	virtual void				Undoable_ReadObjectData( IVirtualStream* stream ) = 0;	// reading object
};

//-----------------------------------------------------------
// The observer itself
//-----------------------------------------------------------

struct histState_t
{
	EHistoryAction			type;
	undoableData_t*			subject;
	uint					streamStart;
};

struct actionEvent_t
{
	int						id;
	DkList<histState_t>		states;
};

class CEditorActionObserver
{
public:
			CEditorActionObserver();
			~CEditorActionObserver();

	void	DebugDisplay();

	void	OnLevelLoad();
	void	OnLevelUnload();

	void	ClearHistory();

	// moves history context into past
	void			Undo();

	// moves history context into future
	void			Redo();

	// actions

	// called after object is created
	void			OnCreate( CUndoableObject* object );

	// called before object is deleted
	void			OnDelete( CUndoableObject* object );

	// called before object is modified
	void			BeginModify( CUndoableObject* object );

	// completes all actions (OnCreate, OnDelete, BeginModify)
	void			EndAction();

	int				GetUndoSteps() const;
	int				GetRedoSteps() const;

protected:

	undoableData_t*		TrackUndoable(CUndoableObject* object);
	undoableData_t*		RecordState(actionEvent_t* storeTo, CUndoableObject* object, EHistoryAction type);

	histState_t*		GetLastHistoryOn(CUndoableObject* object, int& stepsAway, int skipEvents = 0);
	bool				HasHistoryToCurrentEvent(CUndoableObject* object);

	bool				IsTrackedUndoable(CUndoableObject* object);

	void				EnsureActiveEvent();
	void				RewindEvents();

	actionEvent_t*				m_activeEvent;
	DkList<actionEvent_t*>		m_events;

	DkList<undoableData_t*>		m_editing;

	DkList<undoableData_t*>		m_tracking;

	int							m_curEvent;
	int							m_actionContextId;
};

extern CEditorActionObserver* g_pEditorActionObserver;

#endif // EDITORACTIONHISTORY_H