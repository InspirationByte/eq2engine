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
};

struct histBlock_t
{
	int start;
	int end;
};

class CUndoableObject
{
	friend class CEditorActionObserver;

public:
						CUndoableObject();
	virtual				~CUndoableObject() {}

	void				Undoable_PushCurrent();				// pushes the current object

	bool				Undoable_PopBack(bool undoCase);	// pops back. undoCase = true if data must be kept
	bool				Undoable_Redo();					// redo the changes

	int					Undoable_GetChangeCount() const;	// get change count before creation

	void				Undoable_ClearHistory();

	bool				m_modifyMark;

protected:
	virtual bool		Undoable_WriteObjectData( IVirtualStream* stream ) = 0;	// writing object
	virtual void		Undoable_ReadObjectData( IVirtualStream* stream ) = 0;	// reading object

private:
	CMemoryStream		m_changesStream;
	DkList<histBlock_t>	m_histOffsets;
	int					m_curHist;
};

//-----------------------------------------------------------
// The observer itself
//-----------------------------------------------------------

struct histEvent_t
{
	EHistoryAction		type;
	int					context;
	CUndoableObject*	object;
};

class CEditorActionObserver
{
public:
			CEditorActionObserver();
			~CEditorActionObserver();

	void	OnLevelLoad();
	void	OnLevelUnload();

	void	SaveHistory();
	void	ClearHistory();

	void	Undo();
	void	Redo();

	// actions
	void	OnCreate( CUndoableObject* object );
	void	OnDelete( CUndoableObject* object );

	void	BeginModify( CUndoableObject* object );
	void	EndModify();
	void	CancelModify();

	int		GetUndoSteps() const;
	int		GetRedoSteps() const;

protected:
	DkList<histEvent_t>			m_events;
	DkList<CUndoableObject*>	m_tracking;

	int							m_curHist;
	int							m_actionContextId;
};

extern CEditorActionObserver* g_pEditorActionObserver;

#endif // EDITORACTIONHISTORY_H