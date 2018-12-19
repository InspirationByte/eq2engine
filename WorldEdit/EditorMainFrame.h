//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Editor main cycle
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORMAINFRAME_H
#define EDITORMAINFRAME_H

#include "wx_inc.h"
#include "ISoundSystem.h"

class CViewWindow;
class CTextureListWindow;
class CSurfaceDialog;
class CLoadLevelDialog;
class CEditableSurface;
class CEntityPropertiesPanel;
class CBuildOptionsDialog;
class CTransformSelectionDialog;
class CEntityList;
class CGroupList;
class CSoundList;
class CWorldLayerPanel;
class wxFourWaySplitter;

enum ToolType_e;

enum VisObjType_e
{
	VisObj_Entities = 0,
	VisObj_Geometry,
	VisObj_Rooms,
	VisObj_AreaPortals,
	VisObj_Clip,

	VisObj_Count
};

class CEditorFrame: public wxFrame
{
public:
    CEditorFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	~CEditorFrame()
	{
		m_mgr.UnInit();
		//m_views_aui.UnInit();
	}

	void						InitializeWindows();
	void						ConstructMenus();
	void						ConstructToolbars();

	bool						IsSnapToGridEnabled();
	bool						IsSnapToVisibleGridEnabled();
	bool						IsPreviewEnabled();

	CViewWindow*				GetViewWindow(int id);

	CEntityPropertiesPanel*		GetEntityPropertiesPanel();

	CBuildOptionsDialog*		GetBuildOptionsDialog();

	CEntityList*				GetEntityListDialog();

	CGroupList*					GetGroupListDialog();

	CSoundList*					GetSoundListDialog();

	CWorldLayerPanel*			GetWorldLayerPanel();

	void						GetMaxRenderWindowSize(int &wide, int &tall);

	void						UpdateAllWindows();

	void						Update3DView();

	void						SnapVector3D(Vector3D &val);
	void						SnapFloatValue(float &val);

	ToolType_e					GetToolType();

	CTextureListWindow*			GetMaterials();

	CSurfaceDialog*				GetSurfaceDialog();

	void						OpenLevelPrompt();
	void						NewLevelPrompt();
	bool						SavePrompt(bool showQuestion = true, bool changeLevelName = false, bool bForceSave = false);

	void						OnConsoleMessage(SpewType_t type, const char* text);

	bool						IsPhysmodelDrawEnabled();
	bool						IsClipModelsEnabled();

	wxMenu*						GetContextMenu();

	void						AddRecentWorld(char* pszName);

	bool						IsObjectVisibiltyEnabled(VisObjType_e type);

    DECLARE_EVENT_TABLE()

protected:
	void						ProcessAllMenuCommands(wxCommandEvent& event);
	void						ViewPanelResize(wxSizeEvent& event);
	void						OnClose(wxCloseEvent& event);

	CViewWindow*				m_view_windows[4];

	CTextureListWindow*			m_texturelist;
	CSurfaceDialog*				m_surfacedialog;
	CLoadLevelDialog*			m_loadleveldialog;
	wxTextEntryDialog*			m_levelsavedialog;
	CEntityPropertiesPanel*		m_entprops;
	CBuildOptionsDialog*		m_buildoptions;
	CTransformSelectionDialog*	m_transformseldialog;
	CEntityList*				m_entlistdialog;

	wxTextEntryDialog*			m_groupnamedialog;
	CGroupList*					m_grplistdialog;
	CSoundList*					m_sndlistdialog;
	CWorldLayerPanel*			m_worldlayers;

	wxAuiManager				m_mgr;

	wxFourWaySplitter*			m_fourway_splitter;
	//wxAuiManager				m_views_aui;
	wxPanel*					m_viewspanel;

	wxPanel*					m_toolsettings_multipanel;

	wxAuiNotebook*				m_output_notebook;
	wxTextCtrl*					m_console_out;
	wxTextCtrl*					m_errors_out;
	wxTextCtrl*					m_warnings_out;
	//wxAuiNotebook*			m_notebook;
	wxMenu*						m_contextMenu;

	wxMenuItem*					m_snaptogrid;
	wxMenuItem*					m_snaptovisiblegrid;

	wxMenuItem*					m_preview;
	wxMenuItem*					m_drawphysmodels;
	wxMenuItem*					m_clipmodels;

	wxMenuItem*					m_gridsizes[9];
	wxMenuItem*					m_objectvis[VisObj_Count];
	

	wxMenu*						m_pRecent;

	ToolType_e					m_nTool;

};

extern CEditorFrame *g_editormainframe;

#endif // EDITORMAINFRAME_H