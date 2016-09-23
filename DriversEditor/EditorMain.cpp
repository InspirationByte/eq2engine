//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: 'Drivers' level editor
//////////////////////////////////////////////////////////////////////////////////

#include "EditorMain.h"

#include "FontCache.h"

#include "UI_HeightEdit.h"
#include "UI_LevelModels.h"
#include "UI_RoadEditor.h"
#include "UI_OccluderEditor.h"
#include "UI_BuildingConstruct.h"

#include "NewLevelDialog.h"
#include "LoadLevDialog.h"
#include "RegionEditFrame.h"

#include "world.h"
#include "eqParallelJobs.h"

#include "EditorActionHistory.h"
#include "materialsystem/MaterialProxy.h"

static CDebugOverlay g_DebugOverlays;
IDebugOverlay *debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

IProxyFactory*		proxyfactory	= NULL;

DKMODULE*			g_matsysmodule = NULL;
IShaderAPI*			g_pShaderAPI = NULL;
IMaterialSystem*	materials = NULL;

CViewParams			g_pCameraParams(Vector3D(0,0,-100), vec3_zero, 70);
Matrix4x4			g_mProjMat, g_mViewMat;

sceneinfo_t			scinfo;

float				g_fRealtime = 0.0f;
float				g_fOldrealtime = 0.0f;
float				g_fFrametime = 0.0f;

Vector3D			g_camera_rotation(25,225,0);
Vector3D			g_camera_target(0);
float				g_fCamSpeed = 10.0;

void FlushCache()
{
	g_pModelCache->ReleaseCache();

	materials->FreeMaterials(true);

	g_pModelCache->PrecacheModel("models/error.egf");
}

class CEGFViewApp: public wxApp
{
    virtual bool OnInit();
	virtual int	 OnExit();

	// TODO: more things
};

//
// main frame
//

#define EDITOR_MAX_ENVIRONMENT_LIST 64

// GUI events
enum
{
	// FILE

	Event_File_New = 1001,
	Event_File_Load,
	Event_File_Save,
	Event_File_SaveAs,
	Event_File_Exit,

	Event_Edit_Copy,
	Event_Edit_Paste,
	Event_Edit_Undo,
	Event_Edit_Redo,

	Event_Edit_ReloadEnvironments,
	Event_Edit_ReloadObjects,

	Event_View_ResetView,
	Event_View_ShowRegionEditor,

	Event_View_Environments_Start,
	Event_View_Environments_End = Event_View_Environments_Start + EDITOR_MAX_ENVIRONMENT_LIST,	// max weather types. If you have issues with this - increase

	Event_Level_RebuildRoadTextures,

	Event_Max_Menu_Range,
};



BEGIN_EVENT_TABLE(CMainWindow, wxFrame)
	EVT_SIZE(OnSize)
	//EVT_ERASE_BACKGROUND(OnEraseBackground)
	//EVT_IDLE(OnIdle)
	//EVT_PAINT(OnPaint)
	EVT_COMBOBOX(-1, OnComboboxChanged)
	EVT_BUTTON(-1, OnButtons)
	EVT_SLIDER(-1, OnButtons)

	EVT_MENU_RANGE(Event_File_New, Event_Max_Menu_Range, ProcessAllMenuCommands)

	EVT_SIZE(OnSize)

	EVT_LEFT_DOWN(ProcessMouseEnter)
	EVT_LEFT_UP(ProcessMouseLeave)
	EVT_MIDDLE_DOWN(ProcessMouseEnter)
	EVT_MIDDLE_UP(ProcessMouseLeave)
	EVT_RIGHT_DOWN(ProcessMouseEnter)
	EVT_RIGHT_UP(ProcessMouseLeave)

	/*
	EVT_KEY_DOWN(ProcessKeyboardDownEvents)
	EVT_KEY_UP(ProcessKeyboardUpEvents)
	EVT_CONTEXT_MENU(OnContextMenu)
	EVT_SET_FOCUS(OnFocus)
	*/
END_EVENT_TABLE()

DECLARE_INTERNAL_SHADERS();

extern void DrvSyn_RegisterShaderOverrides();

void InitMatSystem(HWND window)
{
	materials = (IMaterialSystem*)GetCore()->GetInterface( MATSYSTEM_INTERFACE_VERSION );

	if(!materials)
	{
		ErrorMsg("ERROR! Couldn't get interface of EqMatSystem!");
		exit(0);
	}
	else
	{
		int bpp = 32;

		ETextureFormat format = FORMAT_RGB8;

		// Figure display format to use
		if(bpp == 32) 
		{
			format = FORMAT_RGB8;
		}
		else if(bpp == 24) 
		{
			format = FORMAT_RGB8;
		}
		else if(bpp == 16) 
		{
			format = FORMAT_RGB565;
		}

		matsystem_render_config_t materials_config;

		materials_config.enableBumpmapping = false;
		materials_config.enableSpecular = true; // specular for cubemaps
		materials_config.enableShadows = false;
		materials_config.wireframeMode = false;
		materials_config.editormode = false;

		materials_config.ffp_mode = false;
		materials_config.lighting_model = MATERIAL_LIGHT_FORWARD;
		materials_config.stubMode = false;
		materials_config.threadedloader = true;

		DefaultShaderAPIParameters(&materials_config.shaderapi_params);
		materials_config.shaderapi_params.bIsWindowed = true;
		materials_config.shaderapi_params.hWindow = window;
		materials_config.shaderapi_params.nScreenFormat = format;
		materials_config.shaderapi_params.bEnableVerticalSync = false;

		bool materialSystemStatus = materials->Init("materials/", "EqD3D9RHI", materials_config);

		FogInfo_t fog;
		fog.enableFog = true;
		fog.fogColor = ColorRGB(0.25,0.25,0.25);
		fog.fogdensity = 1.0f;
		fog.fogfar = 14250;
		fog.fognear = -2750;

		materials->SetFogInfo(fog);

		g_pShaderAPI = materials->GetShaderAPI();

		if(!materialSystemStatus)
			exit(0);
	}

	int sw,sh;
	wxDisplaySize(&sw,&sh);
	materials->SetDeviceBackbufferSize(sw,sh);

	materials->LoadShaderLibrary("eqBaseShaders.dll");

	// initialize shader overrides after libraries are loaded
	DrvSyn_RegisterShaderOverrides();

	g_parallelJobs->Init( g_cpuCaps->GetCPUCount() );

	g_fontCache->Init();

	g_pModelCache->PrecacheModel("models/error.egf");

	// register all shaders
	REGISTER_INTERNAL_SHADERS();

	if(!proxyfactory)
		proxyfactory = materials->GetProxyFactory();

	InitMaterialProxies();
}

void InitScene();


CMainWindow::CMainWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) 
	: wxFrame( parent, id, title, pos, size, style )
{
	wxIcon ico;
	if(ico.LoadFile("IDI_MAINICON"))
	{
		SetIcon(ico);
	}
	else
		MsgError("Can't load icon!\n");

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
	m_pRenderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_pRenderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );
	
	bSizer1->Add( m_pRenderPanel, 1, wxEXPAND, 5 );

	InitMatSystem(m_pRenderPanel->GetHWND());

	g_sysConsole->ClearCommandBuffer();
	g_sysConsole->ParseFileToCommandBuffer("editor.cfg");
	g_sysConsole->ExecuteCommandBuffer();

	soundsystem->Init();
	ses->Init(EQ_DRVSYN_DEFAULT_SOUND_DISTANCE);
	
	m_nbPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( -1,300 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_notebook1 = new wxNotebook( m_nbPanel, wxID_ANY, wxDefaultPosition, wxSize( -1,300 ), wxNB_FIXEDWIDTH );
	m_hmapedit = new CUI_HeightEdit( m_notebook1 );
	m_notebook1->AddPage( (CUI_HeightEdit*)m_hmapedit, wxT("Height field"), true );

	m_tools.append(m_hmapedit);

	m_modelsedit = new CUI_LevelModels( m_notebook1 );
	m_notebook1->AddPage( (CUI_LevelModels*)m_modelsedit, wxT("Level objects"), false );

	m_tools.append(m_modelsedit);

	m_roadedit = new CUI_BuildingConstruct( m_notebook1 );
	m_notebook1->AddPage( (CUI_BuildingConstruct*)m_roadedit, wxT("Building constructor"), false );

	m_tools.append(m_roadedit);

	m_occluderEditor = new CUI_OccluderEditor( m_notebook1 );
	m_notebook1->AddPage( (CUI_OccluderEditor*)m_occluderEditor, wxT("Occluder editor"), false );

	m_tools.append(m_occluderEditor);

	m_roadedit = new CUI_RoadEditor( m_notebook1 );
	m_notebook1->AddPage( (CUI_RoadEditor*)m_roadedit, wxT("Road editor"), false );

	m_tools.append(m_roadedit);

	m_regionEditorFrame = new CRegionEditFrame(this);

	/*
	m_objectsedit = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook1->AddPage( m_objectsedit, wxT("Level Objects"), false );
	m_aiedit = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook1->AddPage( m_aiedit, wxT("AI"), false );
	m_missionedit = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook1->AddPage( m_missionedit, wxT("Mission"), false );
	*/
	
	bSizer4->Add( m_notebook1, 1, wxEXPAND, 5 );
	
	
	m_nbPanel->SetSizer( bSizer4 );
	m_nbPanel->Layout();
	bSizer1->Add( m_nbPanel, 0, wxEXPAND, 5 );
	
	Maximize();

	this->SetSizer( bSizer1 );
	this->Layout();
	m_pMenu = new wxMenuBar( 0 );
	m_menu_file = new wxMenu();
	m_pMenu->Append( m_menu_file, wxT("File") ); 

	m_menu_file->Append( Event_File_New, DKLOC("TOKEN_NEW", L"New\tCtrl+N") );
	m_menu_file->AppendSeparator();
	m_menu_file->Append( Event_File_Load, DKLOC("TOKEN_LOAD", L"Load level\tCtrl+O") );
	m_menu_file->Append( Event_File_Save, DKLOC("TOKEN_SAVE", L"Save level\tCtrl+S") );
	m_menu_file->Append( Event_File_SaveAs, DKLOC("TOKEN_SAVE", L"Save as...\tCtrl+S") );
	//m_menu_file->AppendSeparator();
#pragma todo("previous loaded list")
	m_menu_file->AppendSeparator();
	m_menu_file->Append( Event_File_Exit, DKLOC("TOKEN_EXIT", L"Exit\tAlt+F4") );
	
	
	m_menu_edit = new wxMenu();
	m_pMenu->Append( m_menu_edit, wxT("Edit") );

	m_menu_edit->Append( Event_Edit_Undo, L"Undo\tCtrl+Z");
	m_menu_edit->Append( Event_Edit_Redo, L"Redo\tCtrl+Y");
	
	m_menu_view = new wxMenu();
	m_pMenu->Append( m_menu_view, wxT("View") );
	m_menu_view->Append(Event_View_ShowRegionEditor, DKLOC("TOKEN_SHOWREGIONEDITOR", L"Show region editor"));
	m_menu_view->AppendSeparator();

	m_menu_build = new wxMenu();
	m_pMenu->Append( m_menu_build, wxT("Level") );
	m_menu_build->Append(Event_Level_RebuildRoadTextures, DKLOC("TOKEN_REBUILDROADTEXTURES", L"Rebuild road textures"));
	
	this->SetMenuBar( m_pMenu );
	
	
	this->Centre( wxBOTH );
	
	// Connect Events
	m_notebook1->Connect( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxNotebookEventHandler( CMainWindow::OnPageSelectMode ), NULL, this );

	Connect(wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&CMainWindow::OnCloseCmd, NULL, this);

	debugoverlay->Init();

		//EVT_ERASE_BACKGROUND(OnEraseBackground)
	//EVT_IDLE(OnIdle)
	//EVT_PAINT(OnPaint)
	
	m_pRenderPanel->Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(CMainWindow::OnEraseBackground), NULL, this);
	m_pRenderPanel->Connect(wxEVT_IDLE, wxIdleEventHandler(CMainWindow::OnIdle), NULL, this);
	m_pRenderPanel->Connect(wxEVT_PAINT, wxPaintEventHandler(CMainWindow::OnPaint), NULL, this);

	m_pRenderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), NULL, this);
	
	
	m_pRenderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_DCLICK, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);

	
	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DCLICK, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	

	m_pRenderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);

	m_pRenderPanel->Connect(wxEVT_KEY_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessKeyboardDownEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_KEY_UP, (wxObjectEventFunction)&CMainWindow::ProcessKeyboardUpEvents, NULL, this);

	

	/*
	m_pRenderPanel->Connect(wxEVT_ENTER_WINDOW, (wxObjectEventFunction)&CMainWindow::ProcessMouseEnter, NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEAVE_WINDOW, (wxObjectEventFunction)&CMainWindow::ProcessMouseLeave, NULL, this);

	m_pRenderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEnter, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseLeave, NULL, this);

	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEnter, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseLeave, NULL, this);
	*/

	m_bIsMoving = false;
	m_bDoRefresh = false;

	RefreshGUI();

	for(int i = 0; i < m_tools.numElem(); i++)
	{
		m_tools[i]->InitTool();
	}

	m_selectedTool = m_hmapedit;

	m_newLevelDialog = new CNewLevelDialog(this);
	m_loadleveldialog = new CLoadLevelDialog(this);
	m_levelsavedialog = new wxTextEntryDialog(this, DKLOC("TOKEN_WORLDNAME", L"World name"), DKLOC("TOKEN_SPECIFYWORLDNAME", L"Specify world name"));

	// create physics scene and add infinite plane
	InitScene();

	m_bNeedsSave = false;
	m_bSavedOnDisk = false;

	m_menu_environment = NULL;
	m_menu_environment_itm = NULL;
}

void CMainWindow::OnPageSelectMode( wxNotebookEvent& event )
{
	int sel_id = event.GetSelection();
	IEditorTool* tool = dynamic_cast<IEditorTool*>(m_notebook1->GetPage(sel_id));

	if(tool)
		m_selectedTool = tool;
	else
		m_selectedTool = m_hmapedit;
}

void InitScene()
{
	g_pPhysics->SceneInit();

	g_pGameWorld->SetEnvironmentName("day_clear");

	g_pGameWorld->Init();
	g_pGameWorld->InitEnvironment();
	g_pGameWorld->m_level.Init(16, 16, 64, true);
}

void CMainWindow::OnIdle(wxIdleEvent &event)
{
	event.RequestMore(true);
	ReDraw();
}

void CMainWindow::ProcessMouseEnter(wxMouseEvent& event)
{
	CaptureMouse();
}

void CMainWindow::ProcessMouseLeave(wxMouseEvent& event)
{
	ReleaseMouse();
}

void CMainWindow::MakeEnvironmentListMenu()
{
	if(m_menu_environment_itm)
		m_menu_view->Delete(m_menu_environment_itm);

	m_menu_environment = new wxMenu();

	for(int i = 0; i < m_environmentList.numElem(); i++)
	{
		m_menu_environment->Append( Event_View_Environments_Start+i, m_environmentList[i].c_str() );
	}

	m_menu_environment_itm = m_menu_view->AppendSubMenu( m_menu_environment, wxT("Environment") );
}

void CMainWindow::OnLevelUnload()
{
	for(int i = 0; i < m_tools.numElem(); i++)
		m_tools[i]->OnLevelUnload();

	g_pEditorActionObserver->OnLevelUnload();

	m_regionEditorFrame->OnLevelUnload();
}

void CMainWindow::OnLevelLoad()
{
	for(int i = 0; i < m_tools.numElem(); i++)
		m_tools[i]->OnLevelLoad();

	g_pEditorActionObserver->OnLevelLoad();

	m_regionEditorFrame->OnLevelLoad();
}

void CMainWindow::OpenLevelPrompt()
{
	if(!SavePrompt())
		return;

	// first we've need to rebuild level list
	m_loadleveldialog->RebuildLevelList();

	if(m_loadleveldialog->ShowModal() == wxID_OK)
	{
		OnLevelUnload();
		g_pGameWorld->Cleanup();

		g_pGameWorld->SetLevelName( m_loadleveldialog->GetSelectedLevelString() );
		g_pGameWorld->Init();

		g_pGameWorld->LoadLevel();

		g_pGameWorld->Ed_FillEnviromentList(m_environmentList);

		m_bSavedOnDisk = true;
		m_bNeedsSave = false;

		OnLevelLoad();

		//ResetViews();
		//UpdateAllWindows();

		MakeEnvironmentListMenu();
	}

	//if(IsSavedOnDisk())
	//	AddRecentWorld((char*)g_pLevel->GetLevelName());
}

void CMainWindow::NewLevelPrompt()
{
	if(m_newLevelDialog->ShowModal() == wxID_OK)
	{
		int cellSize = m_newLevelDialog->GetLevelCellSize();
		int wide, tall;
		m_newLevelDialog->GetLevelWideTall(wide,tall);

		if(wide == 0 || tall == 0)
		{
			wxMessageBox("Invalid level dimensions, please input correct ones", "Error", wxOK | wxCENTRE | wxICON_EXCLAMATION, this);
			return;
		}

		if(!SavePrompt())
			return;

		OnLevelUnload();
		g_pGameWorld->Cleanup();

		g_pGameWorld->SetLevelName("unnamed");

		m_bNeedsSave = false;
		m_bSavedOnDisk = false;

		// init new level
		g_pPhysics->SceneInit();

		g_pGameWorld->SetEnvironmentName("day_clear");

		g_pGameWorld->Init();
		g_pGameWorld->InitEnvironment();

		if( m_newLevelDialog->GetLevelImageFileName() != NULL )
		{
			Msg("USING IMAGE '%s'\n", m_newLevelDialog->GetLevelImageFileName());

			EqString imageFilename = m_newLevelDialog->GetLevelImageFileName();

			CImage img;
			if( img.LoadImage( imageFilename.c_str() ) )
			{
				// load tile description from txt
				LevelGenParams_t genParams;
				genParams.cellsPerRegion = cellSize;

				LoadTileTextureFile((imageFilename.Path_Strip_Ext()+"_textures.txt").c_str(), genParams);

				if(!g_pGameWorld->m_level.Ed_GenerateMap( genParams, &img ))
				{
					wxMessageBox("Image is bigger that level size! Nothing is generated.", "Warning", wxOK | wxCENTRE, this);
				}
			}
		}
		else
			g_pGameWorld->m_level.Init(wide, tall, cellSize, true);

		OnLevelLoad();
	}

}

void CMainWindow::NotifyUpdate()
{
	m_bNeedsSave = true;
}

bool CMainWindow::IsNeedsSave()
{
	return m_bNeedsSave;
}

bool CMainWindow::IsSavedOnDisk()
{
	return m_bSavedOnDisk;
}

bool CMainWindow::SavePrompt(bool showQuestion, bool changeLevelName, bool bForceSave)
{
	
	if(!IsNeedsSave() && !changeLevelName)
	{
		// if it's already on disk, save it
		if(IsSavedOnDisk() && bForceSave)
		{
			g_pGameWorld->SaveLevel();
			
			m_bSavedOnDisk = true;
			m_bNeedsSave = false;
		}

		return true;
	}
	
	if(showQuestion)
	{
		int result = wxMessageBox(varargs("Do you want to save level '%s'?", g_pGameWorld->GetLevelName()), "Question", wxYES_NO | wxCANCEL | wxCENTRE | wxICON_WARNING, this);

		if(result == wxCANCEL)
			return false;

		if(result == wxNO)
			return true;
	}

	if(!IsSavedOnDisk() || changeLevelName)
	{
		m_levelsavedialog->SetValue(g_pGameWorld->GetLevelName());

		if(m_levelsavedialog->ShowModal() == wxID_OK)
		{
			g_pGameWorld->SetLevelName(m_levelsavedialog->GetValue());
			g_pGameWorld->SaveLevel();

			m_bSavedOnDisk = true;
			m_bNeedsSave = false;

			for(int i = 0; i < m_tools.numElem(); i++)
				m_tools[i]->OnLevelSave();
		}
	}
	else
	{
		g_pGameWorld->SaveLevel();

		for(int i = 0; i < m_tools.numElem(); i++)
			m_tools[i]->OnLevelSave();

		m_bSavedOnDisk = true;
		m_bNeedsSave = false;
	}

	return true;
}

void CMainWindow::ProcessAllMenuCommands(wxCommandEvent& event)
{
	
	if(event.GetId() == Event_File_New)
	{
		NewLevelPrompt();
	}
	else if(event.GetId() == Event_File_Load)
	{
		OpenLevelPrompt();
	}
	else if(event.GetId() == Event_File_Save)
	{
		SavePrompt(false, false, true);
	}
	else if(event.GetId() == Event_File_SaveAs)
	{
		SavePrompt(false, true);
	}
	else if(event.GetId() == Event_Edit_Undo)
	{
		g_pEditorActionObserver->Undo();
	}
	else if(event.GetId() == Event_Edit_Redo)
	{
		g_pEditorActionObserver->Redo();
	}
	else if(event.GetId() == Event_View_ResetView)
	{
		g_camera_rotation = Vector3D(25,225,0);
		g_camera_target = vec3_zero;
		g_fCamSpeed = 10.0f;
	}
	else if(event.GetId() == Event_View_ShowRegionEditor)
	{
		m_regionEditorFrame->Show();
	}
	else if(event.GetId() >= Event_View_Environments_Start && event.GetId() <= Event_View_Environments_End)
	{
		int envId = event.GetId()-Event_View_Environments_Start;

		g_pGameWorld->SetEnvironmentName( m_environmentList[envId].c_str() );
		g_pGameWorld->InitEnvironment();
	}
	else if(event.GetId() == Event_Level_RebuildRoadTextures)
	{
		wxFileDialog* file = new wxFileDialog(NULL, "Open texture info file", 
													wxEmptyString, 
													"*_textures.txt", 
													"Texture info file (*_textures.txt)|*_textures.txt;", 
													wxFD_FILE_MUST_EXIST | wxFD_OPEN);

		if(file && file->ShowModal() == wxID_OK)
		{
			EqString fname(file->GetPath().wchar_str());

			// load tile description from txt
			LevelGenParams_t genParams;
			genParams.keepOldLevel = true;
			genParams.onlyRoadTextures = true;

			LoadTileTextureFile(fname.c_str(), genParams);

			if(!g_pGameWorld->m_level.Ed_GenerateMap( genParams, NULL ))
			{
				wxMessageBox("Error happen.", "Error", wxOK | wxCENTRE, this);
			}
			g_pMainFrame->NotifyUpdate();
		}
		else
			return;
	}
	else if(event.GetId() == Event_File_Exit)
	{
		Close();
	}
}

Vector2D		g_vLastMousePosition(0);

void CMainWindow::ProcessMouseEvents(wxMouseEvent& event)
{
	/*
	if(GetForegroundWindow() != m_hmapedit->GetHWND())
	{
		SetFocus();

		// windows-only
		//SetForegroundWindow( GetHWND() );
	}*/

	m_pRenderPanel->SetFocus();

	Vector3D cam_angles = g_camera_rotation;
	Vector3D cam_pos = g_camera_target;

	float fov = g_fCamSpeed;

	float move_delta_x = (event.GetX() - g_vLastMousePosition.x);
	float move_delta_y = (event.GetY() - g_vLastMousePosition.y);

	wxPoint point(g_vLastMousePosition.x, g_vLastMousePosition.y);

	if(!m_bIsMoving)
	{
		m_vLastCursorPos = point;
		m_vLastClientCursorPos = ClientToScreen(point);
	}

	point = ClientToScreen(point);

	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	float zoom_factor = (fov/100)*0.5f*0.55f;

	// make x and y size
	float camera_move_factor = (fov/h)*2; // why height? Because x has aspect...

	Vector2D prev_mouse_pos = g_vLastMousePosition;
	g_vLastMousePosition = Vector2D(event.GetX(),event.GetY());

	bool bAnyMoveButton = false;

	if(event.ShiftDown())
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			if(event.Dragging())
			{
				cam_angles.x += move_delta_y*0.3f;
				cam_angles.y -= move_delta_x*0.3f;

				g_vLastMousePosition = prev_mouse_pos;
			}

			m_bIsMoving = true;
			bAnyMoveButton = true;
			//wxCursor cursor(wxCURSOR_ARROW);
			//SetCursor(cursor);
		}

		if(event.ButtonIsDown(wxMOUSE_BTN_RIGHT))
		{
			if(event.Dragging())
			{
				Vector3D forward;
				AngleVectors(cam_angles, &forward, NULL, NULL);

				cam_pos -= forward*move_delta_y * camera_move_factor;

				m_bIsMoving = true;
				bAnyMoveButton = true;

				g_vLastMousePosition = prev_mouse_pos;
			}

			//wxCursor cursor(wxCURSOR_MAGNIFIER);
			//SetCursor(cursor);
		}

		if(event.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
		{
			if(event.Dragging())
			{
				Vector3D right, up;
				AngleVectors(cam_angles, NULL, &right, &up);

				camera_move_factor *= -1;

				m_bIsMoving = true;
				bAnyMoveButton = true;

				extern float g_frametime;

				cam_pos -= right*move_delta_x * camera_move_factor;
				cam_pos += up*move_delta_y * camera_move_factor;

				g_vLastMousePosition = prev_mouse_pos;
			}
		}

		// change cam speed
		float mouse_wheel = event.GetWheelRotation()*0.5f * zoom_factor;

		fov += mouse_wheel;

		fov = clamp(fov,10,1024);

		g_fCamSpeed = fov;
	}
	else
	{
		m_selectedTool->ProcessMouseEvents(event);
	}

	if(!bAnyMoveButton)
	{
		m_bIsMoving = false;
		while(ShowCursor(TRUE) < 0);
	}

	if(m_bIsMoving)
	{
		SetCursorPos(m_vLastClientCursorPos.x, m_vLastClientCursorPos.y);
		ShowCursor(FALSE);
	}

	cam_pos = clamp(cam_pos, Vector3D(-MAX_COORD_UNITS), Vector3D(MAX_COORD_UNITS));

	g_camera_rotation = cam_angles;
	g_camera_target = cam_pos;
}

void CMainWindow::ProcessKeyboardDownEvents(wxKeyEvent& event)
{
	//m_pRenderPanel->SetFocus();

	if(m_selectedTool)
		m_selectedTool->OnKey(event, true);
}

void CMainWindow::ProcessKeyboardUpEvents(wxKeyEvent& event)
{
	//m_pRenderPanel->SetFocus();

	if(m_selectedTool)
		m_selectedTool->OnKey(event, false);
}

void CMainWindow::GetMouseScreenVectors(int x, int y, Vector3D& origin, Vector3D& dir)
{
	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	ScreenToDirection(g_pCameraParams.GetOrigin(), Vector2D(w-x,y), Vector2D(w,h), origin, dir, g_mProjMat*g_mViewMat, false);

	dir = normalize(dir);
}

CLevelRegion* CMainWindow::GetRegionAtScreenPos(int mx, int my, float height, Vector3D& pointPos)
{
	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	Vector3D ray_start, ray_dir;

	// set object
	ScreenToDirection(g_pCameraParams.GetOrigin(), Vector2D(w-mx,my), Vector2D(w,h), ray_start, ray_dir, g_mProjMat*g_mViewMat, false);
	
	// Trace many tiles from maximum height and testing the position

	Plane pl(0,1,0,-height);

	float fNearest = MAX_COORD_UNITS;

	if( pl.GetIntersectionWithRay(ray_start, normalize(ray_dir), pointPos) )
	{
		CLevelRegion* region = g_pGameWorld->m_level.GetRegionAtPosition(pointPos);

		// trace every tile quad to output position
		if(region)
		{
			for(int x = 0; x < region->m_heightfield[0]->m_sizew; x++)
			{
				for(int y = 0; y < region->m_heightfield[0]->m_sizeh; y++)
				{
					hfieldtile_t* tile = region->m_heightfield[0]->GetTile(x, y);

					Plane pl2(0,1,0,-(tile->height*HFIELD_HEIGHT_STEP));
					Vector3D tileTracedPos;

					float frac = 1.0f;

					if(pl2.GetIntersectionLineFraction(ray_start, ray_start+normalize(ray_dir)*MAX_COORD_UNITS, tileTracedPos, frac))
					{
						int c_x, c_y;

						if(region->m_heightfield[0]->PointAtPos(tileTracedPos, c_x, c_y) && 
							region->m_heightfield[0]->GetTile(c_x, c_y) == tile && frac < fNearest)
						{
							fNearest = frac;
							pointPos = tileTracedPos;
							break;
						}
					}
				}
			}
		}

		return region;
	}

	return NULL;
}

void CMainWindow::OnSashSize( wxSplitterEvent& event )
{
	if(materials)
	{
		m_bDoRefresh = true;
	}
}

void CMainWindow::OnSize(wxSizeEvent& event)
{
	wxFrame::OnSize( event );

	m_bDoRefresh = true;
}

float g_realtime = 0;
float g_oldrealtime = 0;
float g_frametime = 0;

void ShowFPS()
{
	// FPS status
	static float accTime = 0.1f;
	static int fps = 0;
	static int nFrames = 0;

	if (accTime > 0.1f)
	{
		fps = (int) (nFrames / accTime + 0.5f);
		nFrames = 0;
		accTime = 0;
	}

	accTime += g_frametime;
	nFrames++;

	ColorRGBA col(1,0,0,1);

	if(fps > 25)
	{
		col = ColorRGBA(1,1,0,1);
		if(fps > 45)
			col = ColorRGBA(0,1,0,1);
	}

	debugoverlay->Text(col, "FPS: %d (%g s)", fps, g_frametime);
}

// PERFORMANCE INFO
#define MIN_FPS         0.1         // Host minimum fps value for maxfps.
#define MAX_FPS         5000.0      // Upper limit for maxfps.

#define MAX_FRAMETIME	0.3
#define MIN_FRAMETIME	0.00001

void CMainWindow::ReDraw()
{
	if(!materials)
		return;

	//	ses->Update();

	if(!IsShown())
		return;

	if(m_bDoRefresh)
	{
		//int w, h;
		//m_pRenderPanel->GetSize(&w,&h);
		//materials->SetDeviceBackbufferSize(w,h);
		m_bDoRefresh = false;
	}

	// compute time since last frame
	g_realtime = Platform_GetCurrentTime();
	
	/*
	float fps = 1000;

	if ( fps != 0 )
	{
		// Limit fps to withing tolerable range
		fps = max( MIN_FPS, fps );
		fps = min( MAX_FPS, fps );

		float minframetime = 1.0 / fps;

		if(( g_realtime - g_oldrealtime ) < minframetime )
			return;
	}*/
	
	g_frametime = g_realtime - g_oldrealtime;
	g_oldrealtime = g_realtime;

	// update material system and proxies
	materials->Update(g_frametime);



	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w,h);

	if(materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.5,0.5,0.5, 1));

		Vector3D forward, right;
		AngleVectors(g_camera_rotation, &forward, &right);

		g_pCameraParams.SetAngles(g_camera_rotation);
		g_pCameraParams.SetOrigin(g_camera_target + forward);

		ShowFPS();
		debugoverlay->Text(color4_white, "Camera position: %g %g %g\n", g_camera_target.x,g_camera_target.y,g_camera_target.z);

		FogInfo_t fog;
		materials->GetFogInfo(fog);

		fog.viewPos = g_pCameraParams.GetOrigin();

		materials->SetFogInfo(fog);

		// setup perspective
		//g_mProjMat = perspectiveMatrixY(DEG2RAD(g_pCameraParams.GetFOV()), w, h, 1, 5000);

		//g_mViewMat = rotateZXY4(DEG2RAD(-g_pCameraParams.GetAngles().x),DEG2RAD(-g_pCameraParams.GetAngles().y),DEG2RAD(-g_pCameraParams.GetAngles().z));
		//g_mViewMat.translate(-g_pCameraParams.GetOrigin());

		//materials->SetMatrix(MATRIXMODE_PROJECTION, g_mProjMat);
		//materials->SetMatrix(MATRIXMODE_VIEW, g_mViewMat);

		//materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		g_pGameWorld->SetView(g_pCameraParams);
		g_pGameWorld->BuildViewMatrices(w,h, 0);

		materials->GetMatrix(MATRIXMODE_PROJECTION, g_mProjMat);
		materials->GetMatrix(MATRIXMODE_VIEW, g_mViewMat);

		soundsystem->SetListener(g_pCameraParams.GetOrigin(), g_mViewMat.rows[2].xyz(),g_mViewMat.rows[1].xyz(), vec3_zero);
		soundsystem->Update();
		ses->Update();

		// Update things
		// if(g_gameEnabled)
		//		g_pGameWorld->UpdateWorld(g_frametime);

		g_pGameWorld->UpdateWorld(g_frametime);

		int nRenderFlags = 0;

		g_pGameWorld->UpdateOccludingFrustum();

		// Now we can draw our model
		g_pGameWorld->Draw(nRenderFlags);

		// draw tool data
		m_selectedTool->OnRender();

		// draw floor 1x1 meters
		//debugoverlay->Polygon3D(Vector3D(-64,0,-64), Vector3D(-64,0,64), Vector3D(64,0,64), ColorRGBA(1,1,0,0.25));
		//debugoverlay->Polygon3D(Vector3D(64,0,64), Vector3D(64,0,-64), Vector3D(-64,0,-64), ColorRGBA(1,1,0,0.25));

		debugoverlay->Draw(g_mProjMat, g_mViewMat, w,h);

		materials->EndFrame( NULL );
		//wxYield();
	}
}

void CMainWindow::RefreshGUI()
{

}

void CMainWindow::OnComboboxChanged(wxCommandEvent& event)
{
	// TODO: selected slots

}

void CMainWindow::OnCloseCmd(wxCloseEvent& event)
{
	if(!SavePrompt())
		return;

	OnLevelUnload();
	g_pGameWorld->Cleanup();

	g_parallelJobs->Wait();
	g_parallelJobs->Shutdown();

	Msg("EXIT CLEANUP...\n");

	m_levelsavedialog->Destroy();

	for(int i = 0; i < m_tools.numElem(); i++)
		m_tools[i]->ShutdownTool();

	g_pModelCache->ReleaseCache();

	Destroy();

	ses->StopAllSounds();
	ses->Shutdown();

	soundsystem->Shutdown();

	// shutdown material system
	materials->Shutdown();

	materials = NULL;

	g_fileSystem->FreeModule(g_matsysmodule);
	
	// shutdown core
	GetCore()->Shutdown();

	wxExit();
}

void CMainWindow::OnButtons(wxCommandEvent& event)
{
	// TODO: selected slots


}

#undef IMPLEMENT_WXWIN_MAIN

bool InitCore(char *pCmdLine)
{
	// initialize core
	GetCore()->Init("Editor", pCmdLine);

	if(!g_fileSystem->Init(false))
		return false;

	g_fileSystem->AddSearchPath("Editor");

	g_cmdLine->ExecuteCommandLine( true, true );

	return true;
}

IMPLEMENT_APP(CEGFViewApp)

typedef int (*winmain_wx_cb)(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *pCmdLine, int nCmdShow);

winmain_wx_cb g_wxEntryCallback = wxEntry;

CMainWindow *g_pMainFrame = NULL;

bool CEGFViewApp::OnInit()
{
#ifdef _DEBUG
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	//flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	flag |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
#endif

	InitCore(GetCommandLineA());

	setlocale(LC_ALL,"C");

	// first, load matsystem module
	g_matsysmodule = g_fileSystem->LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		return -1;
	}

	g_localizer->AddTokensFile("EGFMan");

	g_pMainFrame = new CMainWindow( NULL, -1, DKLOC("TOKEN_TITLE", L"Driver Syndicate Level Editor"));
	g_pMainFrame->Centre();
	g_pMainFrame->Show(true);

	SetTopWindow(g_pMainFrame);

	g_parallelJobs->Init(1);

    return true;
}

int CEGFViewApp::OnExit()
{

	wxEntryCleanup();
	//WriteCfgFile("cfg/editor.cfg");

    return 0;
} 