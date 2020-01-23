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

class CUndoableObject;

struct undoableData_t
{
	undoableData_t();
	
	void					Clear();

	uint					Push();
	bool					Pop();

	bool					Redo();

	CMemoryStream			m_changesStream;
	DkList<histBlock_t>		m_histOffsets;
	int						m_curHist;

	CUndoableObject*		object;
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
	virtual bool			Undoable_WriteObjectData( IVirtualStream* stream ) = 0;	// writing object
	virtual void			Undoable_ReadObjectData( IVirtualStream* stream ) = 0;	// reading object
};

// Undoable factory. Recreates object
template <class T>
class CUndoableFactory
{
	typedef T* (FactoryFunc)();
public:

	virtual T* CreateFrom(IVirtualStream* stream) = 0;
};

#define UNDOABLE_FACTORY_BEGIN(classname)\
namespace N##classname##Factory {\
	class C##classname##Factory : public CUndoableFactory<classname> { \
		public:\
			classname* CreateFrom(IVirtualStream* stream);\
	};\
	typedef C##classname##Factory FactoryClass;\
	classname* C##classname##Factory::CreateFrom(IVirtualStream* stream)

#define UNDOABLE_FACTORY_END \
	 static FactoryClass s_factory; }

//-----------------------------------------------------------
// The observer itself
//-----------------------------------------------------------

struct histEvent_t
{
	EHistoryAction		type;
	int					context;
	undoableData_t*		subject;
	uint				streamStart;
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
	DkList<undoableData_t*>		m_tracking;

	int							m_curEvent;
	int							m_actionContextId;
};

extern CEditorActionObserver* g_pEditorActionObserver;

#endif // EDITORACTIONHISTORY_H