//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Edtior render window
//////////////////////////////////////////////////////////////////////////////////

#ifndef RENDERWINDOW_H
#define RENDERWINDOW_H

#include "EditorHeader.h"
#include "wx_inc.h"
#include "Font.h"
#include "EditorCamera.h"

extern CEditorViewRender g_views[];

class CViewWindow: public wxWindow
{
public:
    CViewWindow(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size);

	void		OnSize(wxSizeEvent &event);
	void		OnIdle(wxIdleEvent &event);
	void		OnEraseBackground(wxEraseEvent& event);
	void		OnPaint(wxPaintEvent& event);

	void		ProcessMouseEnter(wxMouseEvent& event);
	void		ProcessMouseLeave(wxMouseEvent& event);
	void		ProcessMouseEvents(wxMouseEvent& event);
	void		ProcessKeyboardDownEvents(wxKeyEvent& event);
	void		ProcessKeyboardUpEvents(wxKeyEvent& event);
	void		OnContextMenu(wxContextMenuEvent& event);
	void		OnFocus(wxFocusEvent& event);

	void		ProcessObjectMovement(Vector3D &delta3D, IVector2D &delta2D, wxMouseEvent& event);

	void		Redraw();

	void		ChangeView(int nNewView);

	void		NotifyUpdate();

	CEditorViewRender*	GetActiveView();

	DECLARE_EVENT_TABLE()

protected:

//	wxComboBox*				m_projection_selector;
//	wxComboBox*				m_rendermode_selector;

	int						m_viewid;
	bool					m_mustupdate;

	bool					m_bIsMoving;

	wxPoint					m_vLastCursorPos;
	wxPoint					m_vLastClientCursorPos;

	IEqSwapChain*			m_swapChain;
};

#endif // RENDERWINDOW_H