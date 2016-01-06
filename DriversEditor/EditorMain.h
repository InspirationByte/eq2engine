//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: 'Drivers' level editor
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORMAIN_H
#define EDITORMAIN_H

#include "Platform.h"

#include "EditorHeader.h"
#include "DebugOverlay.h"
#include "math/math_util.h"

#include "IEqModel.h"

#include "physics.h"
#include "level.h"

class CUI_HeightEdit;
class IEditorTool;
class CLoadLevelDialog;
class CNewLevelDialog;
class CRegionEditFrame;

class CMainWindow : public wxFrame 
{
public:

	CMainWindow( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("EGFman"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 915,697 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
	
	CLevelRegion*	GetRegionAtScreenPos(int x, int y, float height, Vector3D& pointPos);

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

	void			OnPageSelectMode( wxNotebookEvent& event );

	void			GetMouseScreenVectors(int x, int y, Vector3D& origin, Vector3D& dir);

	void			ProcessKeyboardDownEvents(wxKeyEvent& event);
	void			ProcessKeyboardUpEvents(wxKeyEvent& event);
	//void			OnContextMenu(wxContextMenuEvent& event);
	//void			OnFocus(wxFocusEvent& event);

	void			OpenLevelPrompt();
	void			NewLevelPrompt();
	bool			SavePrompt(bool showQuestion = true, bool changeLevelName = false, bool bForceSave = false);
	
	void			NotifyUpdate();
	bool			IsNeedsSave();
	bool			IsSavedOnDisk();

	void			MakeEnvironmentListMenu();

protected:

	bool			m_bNeedsSave;
	bool			m_bSavedOnDisk;

	wxPanel*		m_pRenderPanel;
	wxPanel*		m_nbPanel;
	wxNotebook*		m_notebook1;

	IEditorTool*	m_hmapedit;
	IEditorTool*	m_modelsedit;
	IEditorTool*	m_roadedit;
	IEditorTool*	m_missionedit;
	IEditorTool*	m_occluderEditor;

	CNewLevelDialog*	m_newLevelDialog;
	CLoadLevelDialog*	m_loadleveldialog;
	wxTextEntryDialog*	m_levelsavedialog;
	CRegionEditFrame*	m_regionEditorFrame;

	DkList<IEditorTool*>	m_tools;

	DkList<EqString>	m_environmentList;

	wxMenuBar*		m_pMenu;

	wxMenu*			m_menu_file;
	wxMenu*			m_menu_edit;
	wxMenu*			m_menu_view;
	wxMenu*			m_menu_build;
	wxMenu*			m_menu_environment;
	wxMenuItem*		m_menu_environment_itm;

	bool			m_bDoRefresh;
	bool			m_bIsMoving;

	wxPoint			m_vLastCursorPos;
	wxPoint			m_vLastClientCursorPos;

	IEditorTool*	m_selectedTool;

	DECLARE_EVENT_TABLE();
};

extern CMainWindow *g_pMainFrame;

#endif // EDITORMAIN_H