//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: 'Drivers' level editor
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORMAIN_H
#define EDITORMAIN_H

#include "core/platform/Platform.h"

#include "wxui_header.h"

class CMainWindow : public wxFrame 
{
public:

	CMainWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("EGFman"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 915,697 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
	
	void			OnSashSize( wxSplitterEvent& event );

	void			ReDraw();
	void			OnPaint(wxPaintEvent& event) {}
	void			OnEraseBackground(wxEraseEvent& event) {}
	void			OnSize(wxSizeEvent& event);

	void			OnIdle(wxIdleEvent &event);

	void			OnCloseCmd(wxCloseEvent& event);

	void			ProcessMouseEnter(wxMouseEvent& event);
	void			ProcessMouseLeave(wxMouseEvent& event);
	void			ProcessMouseEvents(wxMouseEvent& event);

	void			ProcessAllMenuCommands(wxCommandEvent& event);

	void			OnComboboxChanged(wxCommandEvent& event);
	void			OnButtons(wxCommandEvent& event);

	void			RefreshGUI();

	void			GetMouseScreenVectors(int x, int y, Vector3D& origin, Vector3D& dir);

	void			ProcessKeyboardDownEvents(wxKeyEvent& event);
	void			ProcessKeyboardUpEvents(wxKeyEvent& event);
	//void			OnContextMenu(wxContextMenuEvent& event);
	//void			OnFocus(wxFocusEvent& event);
	
protected:

	wxMenuBar*		m_pMenu;

	wxMenu*			m_menu_file;
	wxMenu*			m_menu_sound;
	wxMenu*			m_menu_view;

	bool			m_bDoRefresh;
	bool			m_bIsMoving;

	wxPoint			m_vLastCursorPos;
	wxPoint			m_vLastClientCursorPos;

	wxPanel*		m_renderPanel;

	DECLARE_EVENT_TABLE();
};

extern CMainWindow *g_pMainFrame;

#endif // EDITORMAIN_H