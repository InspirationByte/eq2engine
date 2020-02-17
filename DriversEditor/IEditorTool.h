//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Edtior interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITINTERFACE_H
#define EDITINTERFACE_H

class CUndoableObject;

class IEditorTool
{
public:
	virtual				~IEditorTool() {}

	virtual void		InitTool() = 0;
	virtual void		ShutdownTool() = 0;

	virtual void		Update_Refresh() = 0;

	virtual void		ProcessMouseEvents( wxMouseEvent& event ) = 0;
	virtual void		OnKey(wxKeyEvent& event, bool bDown) = 0;
	virtual void		OnRender() = 0;

	virtual void		OnLevelUnload() {};
	virtual void		OnLevelLoad() {};
	virtual void		OnLevelSave() {};

	virtual void		OnHistoryEvent(CUndoableObject* undoable, int eventType) {}

	virtual void		OnSwitchedTo() { Update_Refresh(); };
	virtual void		OnSwitchedFrom() {};
};

#endif // EDITINTERFACE_H