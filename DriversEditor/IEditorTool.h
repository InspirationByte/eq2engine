//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Edtior interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITINTERFACE_H
#define EDITINTERFACE_H

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
};

#endif // EDITINTERFACE_H