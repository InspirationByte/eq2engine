//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Editor main cycle
//////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commdlg.h>
#include <CommCtrl.h>

#include "EditorHeader.h"

#include "windows/resource.h"

#include "EditorHeader.h"
#include "Sys_Radiant.h"
#include "studio_egf.h"
#include "DebugOverlay.h"
#include "math/math_util.h"
#include "EditorMainFrame.h"
#include "RenderWindows.h"
#include "MaterialListFrame.h"
#include "EditableSurface.h"
#include "EqBrush.h"
#include "GroupEditable.h"
#include "SelectionEditor.h"

#include "SurfaceDialog.h"
#include "EditorVersion.h"
#include "LoadLevelDialog.h"
#include "EntityProperties.h"
#include "BuildOptionsDialog.h"
#include "TransformSelectionDialog.h"
#include "EntityList.h"
#include "SoundList.h"
#include "LayerPanel.h"

#include "Audio/alsound_local.h"
#include "GameSoundEmitterSystem.h"
#include "wxFourWaySplitter.h"

#include "cfgloader.h"
#include "grid.h"

#define ENGINE_EXECUTABLE_NAME	"Eq32.exe"	// TODO: swithable

static CDebugOverlay g_DebugOverlays;
IDebugOverlay *debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

int	g_gridsize = 8;

DKMODULE* g_matsysmodule = NULL;

// The main engine class pointers
IShaderAPI *g_pShaderAPI = NULL;
IMaterialSystem *materials = NULL;

static CEditorLevel s_level;
CEditorLevel *g_pLevel = &s_level;

HWND Splash_HWND;

#define MAX_RECENT 9
DkList<EqString> g_recentPath;
#define REG_PATH "Software\\Inspiration Byte\\Equilibrium\\EqEdit"

// GUI events
enum
{
	// FILE

	Event_File_NewLevel = 100,
	Event_File_OpenLevel,
	Event_File_SaveLevel,
	Event_File_SaveLevelAs,
	//Event_File_Import,
	Event_File_RecentLevel,
	Event_File_Exit = Event_File_RecentLevel + MAX_RECENT,

	// EDIT

	Event_Edit_Undo,
	Event_Edit_Redo,
	Event_Edit_Cut,
	Event_Edit_Copy,
	Event_Edit_Paste,
	Event_Edit_Remove,
	Event_Edit_Clone,
	Event_Edit_Properties,
	Event_Edit_InvertSelection,
	Event_Edit_HideSelected,				// hides selected objects
	Event_Edit_UnHideAll,					// shows hidden objects
	Event_Edit_GroupSelected,				// moves selected objects to group
	Event_Edit_UngroupSelected,			// ungroups selected objects
	Event_Edit_CloneSelected,			// clones selected objects
	Event_Edit_ArbitaryTransform,

	// VIEW

	Event_View_Reset3DView,
	Event_View_TargetViews,
	Event_View_TranslateViews,
	Event_View_DrawPhysicsModels,
	Event_View_Hide,
	Event_View_UnhideAll,

	Event_View_RenderMode_Wireframe,
	Event_View_RenderMode_Shaded,
	Event_View_RenderMode_Textured,
	Event_View_RenderMode_Lighting,

	Event_View_SnapToGrid,
	Event_View_SnapToVisibleGrid,
	Event_View_PreviewRender,
	Event_View_ShowPhysicsModels,
	Event_View_ClipModels,

	Event_View_Filter, // + VisObj_Count spacing parameters

	Event_View_GridSpacing = Event_View_Filter + VisObj_Count, // +9 spacing parameters


	// OBJECT

	Event_Object_Move = Event_View_GridSpacing + 9,		// mouse movement of object
	Event_Object_Rotate,								// mouse object rotation
	Event_Object_Scale,									// mouse scale
	Event_Object_ArbitaryMove,						// context menu access only
	Event_Object_ArbitaryRotate,					// context menu access only
	Event_Object_ArbitaryScale,						// context menu access only

	// LEVEL

	Event_Level_BuildWorld,							// builds world geometry
	Event_Level_BuildLevel,							// builds level
	Event_Level_StartGame,							// start game

	// WINDOW

	Event_Window_ToggleOutput,						// toggles console output
	Event_Window_SurfaceDialog,						// show surface tools
	Event_Window_EntityList,
	Event_Window_GroupList,
	Event_Window_SoundList,
	Event_Window_MaterialsWindow,					// shows materials window

	// HELP
	Event_Help_About,

	// TOOLS
	Event_Tools_ImportOBJ,					// adds editable from OBJ
	Event_Tools_CastBrushToSurface,			// cast to single surface (taking a first material)
	Event_Tools_CastBrushToSurfaces,		// cast to multiple surfaces
	Event_Tools_ReloadMaterials,			// reloads materials
	Event_Tools_FlipSelectedModelFaces,		// flip
	Event_Tools_JoinSelectedSurfaces,		// join

	Event_Tools_Selection = 1100,			// selection tool
	Event_Tools_Surface,					// surface tool
	Event_Tools_Brush,						// brush tool
	Event_Tools_TerrainPatch,				// terrain patch tool
	Event_Tools_DynObj,						// entity tool
	Event_Tools_PathTool,					// patch tool - for creation of any object type
	Event_Tools_Clipper,					// surface/brush clipper
	Event_Tools_VertexManip,				// vertex manipulation - not available for terrain patches
	Event_Tools_Decals,						// decal creator

	Event_Tools_MaterialEdit,				// shows material editor window

	Event_Max_Range
};

bool SaveRegOptions()
{
	HKEY	hKey;

	EqString reg_path = REG_PATH;

	// Create the top level key
	if ( ERROR_SUCCESS != RegCreateKeyExA( HKEY_LOCAL_MACHINE, reg_path.GetData(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL ) )
		return false;

	// Write Recent Files
	for ( int i = 0; i < g_recentPath.numElem(); i++ )	
		RegSetValueExA(hKey, varargs("mru%d",i), 0, REG_SZ, (BYTE*)g_recentPath[i].GetData(), g_recentPath[i].GetLength() );	

	return true;
}

bool LoadRegOptions()
{
	HKEY	hKey;

	if ( ERROR_SUCCESS != RegOpenKeyExA ( HKEY_LOCAL_MACHINE, REG_PATH, 0, KEY_READ, &hKey ) )
		return false;

	// Read Recent Files
	for ( int i = 0; i < MAX_RECENT; i++ )
	{
		DWORD	dwType;
		DWORD	dwSize;
		char	temp[MAX_PATH];
		dwSize = MAX_PATH;

		if ( ERROR_SUCCESS != RegQueryValueExA( hKey, varargs("mru%d", i ), NULL, &dwType, (LPBYTE)temp, &dwSize ) )
			continue;
		
		g_editormainframe->AddRecentWorld(temp);
	}

	return true;
}

bool ViewRenderInitResources()
{
	// init game renderer
	viewrenderer->InitializeResources();

	// do it here
	materials->BeginPreloadMarker();

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	g_pShaderAPI->Reset();

	materials->EndPreloadMarker();
	materials->Wait();

	return true;
}

bool ViewShutdownResources()
{
	// shutdown resources
	viewrenderer->ShutdownResources();

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	g_pShaderAPI->Reset();

	return true;
}

DkList<shaderfactory_t> pShaderRegistrators;

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
		materials_config.editormode = true;
		materials_config.threadedloader = g_editorCfg->matsystem_threaded;
		materials_config.lowShaderQuality = true;

		materials_config.ffp_mode = false;
#ifdef VR_TEST
		materials_config.lighting_model = MATERIAL_LIGHT_DEFERRED;
#else
		materials_config.lighting_model = MATERIAL_LIGHT_FORWARD;
#endif
		materials_config.stubMode = false;

		DefaultShaderAPIParameters(&materials_config.shaderapi_params);
		materials_config.shaderapi_params.bIsWindowed = true;
		materials_config.shaderapi_params.hWindow = window;
		materials_config.shaderapi_params.nScreenFormat = format;

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

	materials->LoadShaderLibrary("eqBaseShaders.dll");

	// register all shaders
	for(int i = 0; i < pShaderRegistrators.numElem(); i++)
		materials->RegisterShader( pShaderRegistrators[i].shader_name, pShaderRegistrators[i].dispatcher );

#ifdef VR_TEST
	// renderer init
	viewrenderer = &g_views[0];

	materials->AddDestroyLostCallbacks(ViewShutdownResources, ViewRenderInitResources);
#endif
}

void nullspew(SpewType_t t,const char* v) {}

void Editor_Init()
{
	GetLocalizer()->AddTokensFile("editor");

	GetCmdLine()->ExecuteCommandLine(true,true);

	Platform_InitTime();

	MsgInfo("\n\nLevel Editor build %d, %s\n", BUILD_NUMBER_EDITOR - BUILD_NUMBER_SUBTRACTED, COMPILE_DATE);

	MsgInfo("Initializing...\n\n");

	/*
	// load editor settings
	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->ParseFileToCommandBuffer("cfg/editor.cfg");
	GetCommandAccessor()->ExecuteCommandBuffer();
	GetCommandAccessor()->ClearCommandBuffer();
	*/
}

class EqEditorApplication: public wxApp
{
    virtual bool OnInit();
	virtual int	 OnExit();

	// TODO: more things
};

BEGIN_EVENT_TABLE(CEditorFrame, wxFrame)
	EVT_MENU_RANGE(Event_File_NewLevel, Event_Max_Range, CEditorFrame::ProcessAllMenuCommands)
	EVT_CLOSE(CEditorFrame::OnClose)
END_EVENT_TABLE()

#undef IMPLEMENT_WXWIN_MAIN

bool InitCore(HINSTANCE hInstance, char *pCmdLine)
{
	/*
	RECT rect;

	//Show splash!

	Splash_HWND = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, NULL);

	if (Splash_HWND)
	{
		if (GetWindowRect(Splash_HWND, &rect))
		{
			if (rect.left > (rect.top * 2))
			{
				SetWindowPos (Splash_HWND, 0,
					(rect.left / 2) - ((rect.right - rect.left) / 2),
					rect.top, 0, 0,
					SWP_NOZORDER | SWP_NOSIZE);
			}
		}

		ShowWindow(Splash_HWND, SW_NORMAL);
		UpdateWindow(Splash_HWND);
		SetForegroundWindow (Splash_HWND);
	}

	*/

	if(!GetFileSystem()->Init(true))
		return false;

	return true;
}

#undef wxIMPLEMENT_WXWIN_MAIN

#define wxIMPLEMENT_WXWIN_MAIN

/*
#define wxIMPLEMENT_WXWIN_MAIN                                              \
    extern "C" int WINAPI WinMain(HINSTANCE hInstance,                      \
                                  HINSTANCE hPrevInstance,                  \
                                  wxCmdLineArgType lpCmdLine,     \
                                  int nCmdShow)                             \
    {                                                                       \
        wxDISABLE_DEBUG_SUPPORT();                                          \
                                                                            \
		if(!InitCore(hInstance,lpCmdLine)) return -1;						\
        return wxEntry(hInstance, hPrevInstance, NULL, 0);					\
    }                                                                       \
    wxIMPLEMENT_WXWIN_MAIN_BORLAND_NONSTANDARD
*/

IMPLEMENT_APP(EqEditorApplication)

typedef int (*winmain_wx_cb)(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *pCmdLine, int nCmdShow);

winmain_wx_cb g_wxEntryCallback = wxEntry;

CEditorFrame *g_editormainframe = NULL;

void EngineSpewFunc(SpewType_t type,const char* pMsg)
{
	printf("%s", pMsg );
	OutputDebugStringA(pMsg);

	g_editormainframe->OnConsoleMessage(type, pMsg);
}

void InstallSpewFunction()
{
	SetSpewFunction(EngineSpewFunc);
}

class CGameSelectionDialog : public wxDialog 
{
public:
	CGameSelectionDialog( wxWindow* parent )
		 : wxDialog( parent, -1, wxT("Select the mod"), wxDefaultPosition, wxSize( 250,128 ), wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT )
	{
		this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
		wxBoxSizer* bSizer17;
		bSizer17 = new wxBoxSizer( wxVERTICAL );
	
		bSizer17->Add( new wxStaticText( this, wxID_ANY, wxT("Game/Mod"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
		m_pGamesList = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 200,-1 ), 0, NULL, 0 ); 
		bSizer17->Add( m_pGamesList, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
		bSizer17->Add( new wxButton( this, wxID_OK, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );
	
		this->SetSizer( bSizer17 );
		this->Layout();
	
		this->Centre( wxBOTH );
	}

	~CGameSelectionDialog()
	{

	}

	void ScanGames()
	{
		WIN32_FIND_DATAA wfd;
		HANDLE hFile;

		hFile = FindFirstFileA( "*.*", &wfd );
		if(hFile != NULL)
		{
			while(1) 
			{
				if(!FindNextFileA(hFile, &wfd))
					break;

				EqString filename = wfd.cFileName;
				if( filename.GetLength() > 1 && filename != ".." && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				{
					if( GetFileSystem()->FileExist( (filename + "/GameInfo.txt").GetData() ) )
					{
						m_pGamesList->Append( filename.GetData() );
					}
				}
			}

			FindClose(hFile);
		}

		// get the default game name
		KeyValues kv;

		if(kv.LoadFromFile("EqEditor/EqEditSettings.CFG"))
		{
			kvkeybase_t* pSection = kv.GetRootSection();

			kvkeybase_t* pPair = pSection->FindKeyBase("last_game");
			if(pPair)
			{
				for(int i = 0; i < m_pGamesList->GetCount(); i++)
				{
					if(!stricmp(m_pGamesList->GetString(i).c_str(), pPair->values[0]))
					{
						m_pGamesList->SetSelection( i );
						break;
					}
				}
			}
		}

		
	}

	void OnButtonClick(wxCommandEvent& event)
	{
		if(event.GetId() == wxID_OK)
		{
			GetFileSystem()->AddSearchPath( m_pGamesList->GetValue() );

			// get the default game name
			KeyValues kv;

			if(kv.LoadFromFile("EqEditor/EqEditSettings.CFG"))
			{
				kvkeybase_t* pSection = kv.GetRootSection();
				pSection->SetKey("last_game", m_pGamesList->GetValue());

				kv.SaveToFile("EqEditor/EqEditSettings.CFG");
			}

			EndModal(wxID_OK);
		}
	}

protected:

	DECLARE_EVENT_TABLE();

	wxComboBox*			m_pGamesList;
};

BEGIN_EVENT_TABLE(CGameSelectionDialog, wxDialog)
	EVT_BUTTON(-1, OnButtonClick)
END_EVENT_TABLE()

bool EqEditorApplication::OnInit()
{
	setlocale(LC_ALL,"C");

	if(!wxApp::OnInit())
		return false;
	
	CGameSelectionDialog* pGameSelDialog = new CGameSelectionDialog(NULL);

	pGameSelDialog->ScanGames();

	pGameSelDialog->Centre();
	pGameSelDialog->ShowModal();

	SetTopWindow(NULL);

	delete pGameSelDialog;
	
	GetFileSystem()->AddSearchPath("EqEditor");

	LoadEditorConfig();

	// first, load matsystem module
	g_matsysmodule =  GetFileSystem()->LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		return false;
	}

	Editor_Init();

	g_editormainframe = new CEditorFrame( DKLOC("TOKEN_TITLE", "Equilibrium World Editor") , wxPoint(50, 50), wxSize(1024,768) );
	g_editormainframe->Centre();
	g_editormainframe->Show(true);

	SetTopWindow(g_editormainframe);

	DestroyWindow(Splash_HWND);

    return true;
}

int EqEditorApplication::OnExit()
{
    return 0;
}

CEditorFrame::CEditorFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame( NULL, -1, title, pos, size )
{
	// notify wxAUI which frame to use
	m_mgr.SetManagedWindow(this);

	wxIcon ico;
	if(ico.LoadFile("IDI_MAINICON"))
	{
		SetIcon(ico);
	}
	else
		MsgError("Can't load icon!\n");

	SetTitle(wxString(g_pLevel->GetLevelName()) + wxString(" - ") + DKLOC("TOKEN_TITLE", "Equilibrium World Editor"));

	m_nTool = Tool_Selection;

	Maximize();

	m_view_windows[0] = NULL;
	m_view_windows[1] = NULL;
	m_view_windows[2] = NULL;
	m_view_windows[3] = NULL;

	m_output_notebook = new wxAuiNotebook(this, -1, wxDefaultPosition, wxSize(600, 450),wxAUI_NB_TOP |wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS |wxNO_BORDER);

	m_viewspanel = new wxPanel(this,0,0,1000,1000);
	/*
	m_views_aui.SetManagedWindow(m_viewspanel);


	*/

	wxBoxSizer* pSizer = new wxBoxSizer(wxVERTICAL);

	m_fourway_splitter = new wxFourWaySplitter(m_viewspanel, -1, wxDefaultPosition, wxSize(1000,1000));
	pSizer->Add(m_fourway_splitter, 1, wxEXPAND);
	InitializeWindows();

	m_viewspanel->SetSizer(pSizer);
	m_viewspanel->Layout();
	m_viewspanel->Update();
	m_viewspanel->UpdateWindowUI();

	m_mgr.AddPane(m_viewspanel, wxAuiPaneInfo().CenterPane());

	m_fourway_splitter->SetWindow(0, m_view_windows[0]);
	m_fourway_splitter->SetWindow(1, m_view_windows[1]);
	m_fourway_splitter->SetWindow(2, m_view_windows[2]);
	m_fourway_splitter->SetWindow(3, m_view_windows[3]);

	//m_fourway_splitter->AdjustLayout();

	/*
	m_views_aui.AddPane(	m_view_windows[0], 
							wxAuiPaneInfo().Caption("Perspective").
							Top().CenterPane().CaptionVisible(true).PinButton(false).MaximizeButton());

	m_views_aui.AddPane(	m_view_windows[1], 
							wxAuiPaneInfo().Caption("Y-Z Right").
							Bottom().Position(1).CaptionVisible(true).PinButton(false).MaximizeButton().CloseButton(false));

	m_views_aui.AddPane(	m_view_windows[2], 
							wxAuiPaneInfo().Caption("X-Z Top").
							Top().Right().Position(1).CaptionVisible(true).PinButton(false).MaximizeButton().CloseButton(false));

	m_views_aui.AddPane(	m_view_windows[3], 
							wxAuiPaneInfo().Caption("X-Y Front").
							Bottom().Position(2).CaptionVisible(true).PinButton(false).MaximizeButton().CloseButton(false));

	m_mgr.AddPane(m_viewspanel, wxAuiPaneInfo().CenterPane());

	m_views_aui.Update();
	*/
	g_editormainframe = this;
	
	m_console_out = new wxTextCtrl(m_output_notebook, -1, wxEmptyString,
						wxDefaultPosition, wxSize(600,150),
						wxNO_BORDER | wxTE_MULTILINE);

	m_errors_out = new wxTextCtrl(m_output_notebook, -1, wxEmptyString,
						wxDefaultPosition, wxSize(600,150),
						wxNO_BORDER | wxTE_MULTILINE);

	m_warnings_out = new wxTextCtrl(m_output_notebook, -1, wxEmptyString,
						wxDefaultPosition, wxSize(600,150),
						wxNO_BORDER | wxTE_MULTILINE);

	InstallSpewFunction();

	m_console_out->SetEditable(false);
	m_errors_out->SetEditable(false);
	m_warnings_out->SetEditable(false);

	m_output_notebook->AddPage(m_console_out, "All Messages", true);
	m_output_notebook->AddPage(m_errors_out, "Errors", true);
	m_output_notebook->AddPage(m_warnings_out, "Warnings", true);
	
	// add the panes to the manager
	m_mgr.AddPane(m_output_notebook, wxAuiPaneInfo().Bottom().Caption("Output").DestroyOnClose(false).Hide());
	

	ConstructMenus();
	ConstructToolbars();

	m_toolsettings_multipanel = new wxPanel(this, 10,10,200,400, wxTAB_TRAVERSAL | wxNO_BORDER, "ToolSettings");

	m_mgr.AddPane(m_toolsettings_multipanel, wxAuiPaneInfo().Right().CloseButton(false).MaxSize(200,400).Caption(DKLOC("TOKEN_TOOLSETTINGS", "Tool settings")));

	m_entprops = new CEntityPropertiesPanel();

	
	m_worldlayers = new CWorldLayerPanel();
	m_mgr.AddPane(m_worldlayers,  wxAuiPaneInfo().Right().CloseButton(false).MaxSize(200,400).Caption(DKLOC("TOKEN_WLAYERS", "World layers")));
	

	// tell the manager to "commit" all the changes just made
	m_mgr.Update();

	InitMatSystem( (HWND) m_texturelist->GetTexturePanel()->GetHWND() );

	debugoverlay->Init();

	m_texturelist->ReloadMaterialList();

	// init sound system
	soundsystem->Init();

	// initialize game sound emitter system
	ses->Init();
	
	// create dialogs
	m_surfacedialog = new CSurfaceDialog();
	m_loadleveldialog = new CLoadLevelDialog();
	m_buildoptions = new CBuildOptionsDialog();
	m_transformseldialog = new CTransformSelectionDialog();
	m_sndlistdialog = new CSoundList();
	m_grplistdialog = new CGroupList();
	m_entlistdialog = new CEntityList();
	
	Connect(wxEVT_SIZE, wxSizeEventHandler(CEditorFrame::ViewPanelResize));

	g_pModelCache->PrecacheModel("models/error.egf");
	
	Msg("Loading GameInfo.txt...\n");
	
	// parse gameinfo.txt for entity definition file
	KeyValues kvGameInfo;
	if(kvGameInfo.LoadFromFile("GameInfo.txt"))
	{
		kvkeybase_t* pSec = kvGameInfo.GetRootSection()->FindKeyBase("GameInfo", KV_FLAG_SECTION);
		if(pSec)
		{
			kvkeybase_t* pair = pSec->FindKeyBase("EditorEntities");

			if(pair)
			{
				Msg("EDef_Load: parsing %s\n", pair->values[0]);
				g_entdefname = pair->values[0];
				EDef_Load(pair->values[0]);
			}
			else
			{
				MsgError("Couldn't find EditorEntities in GameInfo, using default definition file!\n");
				g_entdefname = "eqengine_base.edf";
				EDef_Load("eqengine_base.edf");
			}

			for(int i = 0; i < pSec->keys.numElem(); i++)
			{
				if(!stricmp(pSec->keys[i]->name,"AddSearchPath"))
				{
					GetFileSystem()->AddSearchPath(pSec->keys[i]->values[0]);
				}
				else if(!stricmp(pSec->keys[i]->name,"AddPackage"))
				{
					GetFileSystem()->AddPackage(pSec->keys[i]->values[0], SP_MOD);
				}
			}
		}
		else
		{
			MsgError("Couldn't parse GameInfo section!\n");
		}
	}
	else
		MsgError("Couldn't load GameInfo.txt!\n");
	

	// add tool panels
	for(int i = 0; i < Tool_Count; i++)
	{
		if(g_pSelectionTools[i])
		{
			g_pSelectionTools[i]->InitializeToolPanel(m_toolsettings_multipanel);

			if(g_pSelectionTools[i]->GetToolPanel())
				g_pSelectionTools[i]->GetToolPanel()->Hide();
		}
	}

	OnActivateCurrentTool();

	LoadRegOptions();

	m_levelsavedialog = new wxTextEntryDialog(this, DKLOC("TOKEN_WORLDNAME", "World name"), DKLOC("TOKEN_SPECIFYWORLDNAME", "Specify world name"));
	m_groupnamedialog = new wxTextEntryDialog(this, DKLOC("TOKEN_GROUPNAME", "Group name"), DKLOC("TOKEN_SPECIFYGROUPNAME", "Specify group name"));

	g_pLevel->CreateNew();

	/*
	if(g_config.showloadingdialogonstart)
		 m_loadleveldialog->ShowModal();
		*/
	g_gridsize = g_editorCfg->default_grid_size;
}

bool CEditorFrame::IsObjectVisibiltyEnabled(VisObjType_e type)
{
	return m_objectvis[type]->IsChecked();
}

void CEditorFrame::ViewPanelResize(wxSizeEvent& event)
{
	/*
	if(!materials)
		return;

	for(int i = 0; i < 4; i++)
	{
		m_views_aui.GetPane(m_view_windows[i]).rect.SetWidth(m_viewspanel->GetSize().x/2);
		m_views_aui.GetPane(m_view_windows[i]).rect.SetHeight(m_viewspanel->GetSize().y/2);
		m_view_windows[i]->SetSize(m_viewspanel->GetSize().x/2,m_viewspanel->GetSize().y/2);

		m_views_aui.GetPane(m_view_windows[i]).MinSize(m_viewspanel->GetSize().x/2,m_viewspanel->GetSize().y/2);
		m_views_aui.GetPane(m_view_windows[i]).MaxSize(m_viewspanel->GetSize().x/2,m_viewspanel->GetSize().y/2);
		m_views_aui.GetPane(m_view_windows[i]).BestSize(m_viewspanel->GetSize().x/2,m_viewspanel->GetSize().y/2);
		
	}

	m_views_aui.Update()
	*/
}

void CEditorFrame::InitializeWindows()
{
	int views_panel_w;
	int views_panel_h;

	m_viewspanel->GetSize(&views_panel_w, &views_panel_h);

	views_panel_w /= 2;
	views_panel_h /= 2;

	m_view_windows[0] = new CViewWindow(m_fourway_splitter, "View 1", wxPoint(0, 0), wxSize(views_panel_w,views_panel_h));
	m_view_windows[0]->Show();
	m_view_windows[0]->ChangeView(CPM_PERSPECTIVE);

	m_view_windows[1] = new CViewWindow(m_fourway_splitter, "View 2", wxPoint(0, 0), wxSize(views_panel_w,views_panel_h));
	m_view_windows[1]->Show();
	m_view_windows[1]->ChangeView(CPM_RIGHT);

	m_view_windows[2] = new CViewWindow(m_fourway_splitter, "View 3", wxPoint(0, 0), wxSize(views_panel_w,views_panel_h));
	m_view_windows[2]->Show();
	m_view_windows[2]->ChangeView(CPM_TOP);

	m_view_windows[3] = new CViewWindow(m_fourway_splitter, "View 4", wxPoint(0, 0), wxSize(128,128));
	m_view_windows[3]->Show();
	m_view_windows[3]->ChangeView(CPM_FRONT);

	m_texturelist = new CTextureListWindow(this, "Materials", wxPoint(0, 0), wxSize(640,480));
}

CTextureListWindow*	CEditorFrame::GetMaterials()
{
	return m_texturelist;
}

void CEditorFrame::UpdateAllWindows()
{
	for(int i = 0; i < 4; i++)
	{
		if(m_view_windows[i])
			m_view_windows[i]->NotifyUpdate();
	}
}

void CEditorFrame::Update3DView()
{
	m_view_windows[0]->NotifyUpdate();
}

void CEditorFrame::GetMaxRenderWindowSize(int &wide, int &tall)
{
	wide = 0;
	tall = 0;

	for(int i = 0; i < 4; i++)
	{
		if(m_view_windows[i])
		{
			int w, h;
			m_view_windows[i]->GetSize(&w,&h);
			
			if(w > wide)
				wide = w;

			if(h > tall)
				tall = h;
		}
	}

	// also check the texture viewer
	int w, h;
	m_texturelist->GetTexturePanel()->GetSize(&w,&h);
			
	if(w > wide)
		wide = w;

	if(h > tall)
		tall = h;
}

wxMenu* CEditorFrame::GetContextMenu()
{
	return m_contextMenu;
}


void CEditorFrame::AddRecentWorld(char* pszName)
{
	for(int i = 0; i < g_recentPath.numElem(); i++)
		m_pRecent->Delete(Event_File_RecentLevel+i);

	m_pRecent->UpdateUI();

	for(int i = 0; i < g_recentPath.numElem(); i++)
	{
		if(!strcmp(g_recentPath[i].GetData(), pszName))
		{
			g_recentPath.removeIndex(i);
			i--;
		}
	}

	int nOffs = g_recentPath.numElem();

	g_recentPath.append(pszName);

	if(g_recentPath.numElem() == MAX_RECENT)
		g_recentPath.removeIndex(0);

	for(int i = 0; i < g_recentPath.numElem(); i++)
		m_pRecent->Insert(0, Event_File_RecentLevel+i, wxString(g_recentPath[i].GetData()));

	m_pRecent->UpdateUI();
}

void CEditorFrame::ConstructMenus()
{
	wxMenu *menuFile = new wxMenu;
	
	menuFile->Append( Event_File_NewLevel, DKLOC("TOKEN_NEW", "New\tCtrl+N") );
	menuFile->AppendSeparator();
	menuFile->Append( Event_File_OpenLevel, DKLOC("TOKEN_OPEN", "Open\tCtrl+O") );
	menuFile->Append( Event_File_SaveLevel, DKLOC("TOKEN_SAVE", "Save\tCtrl+S") );
	menuFile->Append( Event_File_SaveLevelAs, DKLOC("TOKEN_SAVEAS", "Save As...") );
	menuFile->AppendSeparator();

	m_pRecent = new wxMenu;
	menuFile->AppendSubMenu(m_pRecent, DKLOC("TOKEN_RECENT", "Recent"));
	
	//menuFile->Append( Event_File_Import, DKLOC("TOKEN_IMPORT", "Import Map...") );
	menuFile->AppendSeparator();
	menuFile->Append( Event_File_Exit, DKLOC("TOKEN_EXIT", "Exit\tAlt+F4") );

	wxMenu *menuEdit = new wxMenu;
	menuEdit->Append(Event_Edit_ArbitaryTransform, DKLOC("TOKEN_ARBITARYTRANSFORM", "Arbitary transform..."));
	menuEdit->AppendSeparator();
	menuEdit->Append(Event_Edit_Redo, DKLOC("TOKEN_REDO", "ReDo\tCtrl+Y"));
	menuEdit->Append(Event_Edit_Undo, DKLOC("TOKEN_UNDO", "UnDo\tCtrl+Z"));
	menuEdit->AppendSeparator();
	menuEdit->Append(Event_Edit_Cut, DKLOC("TOKEN_CUT", "Cut\tCtrl+X"));
	menuEdit->Append(Event_Edit_Copy, DKLOC("TOKEN_COPY", "Copy\tCtrl+C"));
	menuEdit->Append(Event_Edit_Paste, DKLOC("TOKEN_PASTE", "Paste\tCtrl+V"));
	menuEdit->AppendSeparator();
	menuEdit->Append(Event_Edit_InvertSelection, DKLOC("TOKEN_INVERTSELECTION", "Invert selection\tCtrl+I"));
	menuEdit->Append(Event_Edit_HideSelected, DKLOC("TOKEN_HIDE", "Hide selection\tCtrl+H"));
	menuEdit->Append(Event_Edit_UnHideAll, DKLOC("TOKEN_SHOWALL", "Unhide selection\tShift+Ctrl+H"));
	menuEdit->AppendSeparator();
	menuEdit->Append(Event_Edit_GroupSelected, DKLOC("TOKEN_GROUPSELECTED", "Group selection\tCtrl+G"));
	menuEdit->Append(Event_Edit_UngroupSelected, DKLOC("TOKEN_UNGROUPSELECTED", "Ungroup selection\tShift+Ctrl+G"));
	menuEdit->Append(Event_Edit_CloneSelected, DKLOC("TOKEN_CLONESELECTED", "Clone selected objects\tSpace"));
	menuEdit->AppendSeparator();
	menuEdit->Append(Event_Edit_Properties, DKLOC("TOKEN_PROPERTIES", "Properties...\tN"));
	
	wxMenu *menuView = new wxMenu;

	menuView->Append(Event_View_Reset3DView, DKLOC("TOKEN_RESET3DVIEW", "Reset 3D view"), wxEmptyString);
	menuView->Append(Event_View_TargetViews, DKLOC("TOKEN_TARGETTOSELECTION", "Target views to selection center\tY"), wxEmptyString);
	menuView->Append(Event_View_TranslateViews, DKLOC("TOKEN_MOVETOSELECTION", "Move views to selection center\tU"), wxEmptyString);

	wxMenu *menuFilter = new wxMenu;

	m_objectvis[0] = menuFilter->Append(Event_View_Filter, DKLOC("TOKEN_SHOWENTS", "Show entities"), wxEmptyString, wxITEM_CHECK);
	m_objectvis[1] = menuFilter->Append(Event_View_Filter+1, DKLOC("TOKEN_SHOWGEOM", "Show world geometry"), wxEmptyString, wxITEM_CHECK);
	m_objectvis[2] = menuFilter->Append(Event_View_Filter+2, DKLOC("TOKEN_SHOWROOMS", "Show rooms"), wxEmptyString, wxITEM_CHECK);
	m_objectvis[3] = menuFilter->Append(Event_View_Filter+3, DKLOC("TOKEN_SHOWAREAPORTALS", "Show area portals"), wxEmptyString, wxITEM_CHECK);
	m_objectvis[4] = menuFilter->Append(Event_View_Filter+4, DKLOC("TOKEN_SHOWCLIPS", "Show clip"), wxEmptyString, wxITEM_CHECK);

	for(int i = 0; i < VisObj_Count; i++)
		m_objectvis[i]->Check();

	menuView->AppendSubMenu(menuFilter, DKLOC("TOKEN_SHOWFILTER", "View filter"));

	menuView->AppendSeparator();
	m_gridsizes[0] = menuView->Append(Event_View_GridSpacing, DKLOC("TOKEN_GRID1", "Grid 1\t1"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[1] = menuView->Append(Event_View_GridSpacing+1, DKLOC("TOKEN_GRID2", "Grid 2\t2"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[2] = menuView->Append(Event_View_GridSpacing+2, DKLOC("TOKEN_GRID4", "Grid 4\t3"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[3] = menuView->Append(Event_View_GridSpacing+3, DKLOC("TOKEN_GRID8", "Grid 8\t4"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[4] = menuView->Append(Event_View_GridSpacing+4, DKLOC("TOKEN_GRID16", "Grid 16\t5"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[5] = menuView->Append(Event_View_GridSpacing+5, DKLOC("TOKEN_GRID32", "Grid 32\t6"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[6] = menuView->Append(Event_View_GridSpacing+6, DKLOC("TOKEN_GRID64", "Grid 64\t7"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[7] = menuView->Append(Event_View_GridSpacing+7, DKLOC("TOKEN_GRID128", "Grid 128\t8"), wxEmptyString, wxITEM_RADIO);
	m_gridsizes[8] = menuView->Append(Event_View_GridSpacing+8, DKLOC("TOKEN_GRID256", "Grid 256\t9"), wxEmptyString, wxITEM_RADIO);
	menuView->AppendSeparator();

	m_gridsizes[3]->Toggle();

	m_snaptogrid = menuView->Append(Event_View_SnapToGrid, DKLOC("TOKEN_SNAPTOGRID", "Snap to grid\tG"), wxEmptyString, wxITEM_CHECK);
	m_snaptogrid->Check();

	m_snaptovisiblegrid = menuView->Append(Event_View_SnapToVisibleGrid, DKLOC("TOKEN_SNAPTOVISIBLEGRID", "Snap to visible grid\tAlt+G"), wxEmptyString, wxITEM_CHECK);
	m_snaptovisiblegrid->Check();

	menuView->AppendSeparator();
	m_drawphysmodels = menuView->Append(Event_View_ShowPhysicsModels, DKLOC("TOKEN_SHOWPHYSMODELS", "Show physics models"), wxEmptyString, wxITEM_CHECK);
	m_preview = menuView->Append(Event_View_PreviewRender, DKLOC("TOKEN_PREVIEWRENDER", "Preview rendering"), wxEmptyString, wxITEM_CHECK);
	menuView->AppendSeparator();
	m_clipmodels = menuView->Append(Event_View_ClipModels, DKLOC("TOKEN_CLIPMODELS2D", "Clip models in 2D view"), wxEmptyString, wxITEM_CHECK);

	/*
	wxMenu *menuObject = new wxMenu;
	m_objectmenu = menuObject;
	menuObject->Append(0, DKLOC("TOKEN_NEWOBJECT", "New object..."));
	*/

	wxMenu *menuLevel = new wxMenu;
	menuLevel->Append( Event_Level_BuildWorld, DKLOC("TOKEN_BUILDWORLD", "Build world\tF7"), wxEmptyString);
	menuLevel->Append( Event_Level_BuildLevel, DKLOC("TOKEN_BUILDLEVEL", "Build level and objects\tF9"), wxEmptyString);
	menuLevel->AppendSeparator();
	menuLevel->Append( Event_Level_StartGame, DKLOC("TOKEN_RUNGAME", "Run level in game"), wxEmptyString);
	
	//menuLevel->Append( Event_Level_BuildVisibility, DKLOC("TOKEN_BUILDVIS", "Build visibility\tF9"), wxEmptyString);
	

	wxMenu *menuTools = new wxMenu;
	menuTools->Append( Event_Tools_ImportOBJ, DKLOC("TOKEN_IMPORTOBJ", "Import OBJ to world..."), wxEmptyString);
	menuView->AppendSeparator();
	menuTools->Append( Event_Tools_CastBrushToSurface, DKLOC("TOKEN_CONVERTTOSURF", "Convert brush to surface"), wxEmptyString);
	menuTools->Append( Event_Tools_CastBrushToSurfaces, DKLOC("TOKEN_CONVERTTOSURFS", "Convert selected brush faces to free editables..."), wxEmptyString);
	menuTools->Append( Event_Tools_FlipSelectedModelFaces, DKLOC("TOKEN_FLIPFACES", "Flip free editable faces..."), wxEmptyString);
	menuTools->Append( Event_Tools_JoinSelectedSurfaces, DKLOC("TOKEN_JOINFACES", "Join free editables to one..."), wxEmptyString);
	
	
	menuTools->AppendSeparator();
	menuTools->Append( Event_Tools_ReloadMaterials, DKLOC("TOKEN_RELOADMATERIALS", "Reload all materials"), wxEmptyString);

	


	wxMenu *menuWindow = new wxMenu;
	menuWindow->Append( Event_Window_ToggleOutput, DKLOC("TOKEN_OUTPUT", "Output\tShift+O"), wxEmptyString);
	menuWindow->Append( Event_Window_SurfaceDialog, DKLOC("TOKEN_SURFDIALOG", "Surface dialog\tShift+S"), wxEmptyString);
	menuWindow->Append( Event_Window_EntityList, DKLOC("TOKEN_ENTITYLISTDIALOG", "Entity list\tE"), wxEmptyString);
	menuWindow->Append( Event_Window_GroupList, DKLOC("TOKEN_GROUPLISTDIALOG", "Group list\tG"), wxEmptyString);
	menuWindow->Append( Event_Window_SoundList, DKLOC("TOKEN_SOUNDLISTDIALOG", "Sound list"), wxEmptyString);
	
	
	menuWindow->Append( Event_Window_MaterialsWindow, DKLOC("TOKEN_MATERIALS", "Materials window\tT"), wxEmptyString);

	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append( Event_Help_About, DKLOC("TOKEN_ABOUT", "About")  );

	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, DKLOC("TOKEN_FILE", "File") );
	menuBar->Append( menuEdit, DKLOC("TOKEN_EDIT", "Edit") );
	menuBar->Append( menuView, DKLOC("TOKEN_VIEW", "View") );
//	menuBar->Append( menuObject, DKLOC("TOKEN_OBJECT", "Object") );
	menuBar->Append( menuLevel, DKLOC("TOKEN_LEVEL", "Level") );
	menuBar->Append( menuTools, DKLOC("TOKEN_TOOLS", "Tools") );
	menuBar->Append( menuWindow, DKLOC("TOKEN_WINDOW", "Window") );
	menuBar->Append( menuHelp, DKLOC("TOKEN_HELP", "Help") );

	SetMenuBar( menuBar );

	// make context menu with useful functions
	m_contextMenu = new wxMenu;

	m_contextMenu->Append(Event_Edit_ArbitaryTransform, DKLOC("TOKEN_ARBITARYTRANSFORM", "Arbitary transform..."));
	m_contextMenu->AppendSeparator();
	m_contextMenu->Append(Event_Edit_Cut, DKLOC("TOKEN_CUT", "Cut\tCtrl+X"));
	m_contextMenu->Append(Event_Edit_Copy, DKLOC("TOKEN_COPY", "Copy\tCtrl+C"));
	m_contextMenu->Append(Event_Edit_Paste, DKLOC("TOKEN_PASTE", "Paste\tCtrl+V"));
	m_contextMenu->AppendSeparator();
	m_contextMenu->Append( Event_Tools_CastBrushToSurface, DKLOC("TOKEN_CONVERTTOSURF", "Convert brush to surface"), wxEmptyString);
	m_contextMenu->Append( Event_Tools_CastBrushToSurfaces, DKLOC("TOKEN_CONVERTTOSURFS", "Convert selected brush faces to free editables..."), wxEmptyString);
	m_contextMenu->Append( Event_Tools_FlipSelectedModelFaces, DKLOC("TOKEN_FLIPFACES", "Flip free editable faces..."), wxEmptyString);
	m_contextMenu->Append( Event_Tools_JoinSelectedSurfaces, DKLOC("TOKEN_JOINFACES", "Join free editables to one..."), wxEmptyString);
	m_contextMenu->AppendSeparator();
	m_contextMenu->Append(Event_Edit_InvertSelection, DKLOC("TOKEN_INVERTSELECTION", "Invert selection\tCtrl+I"));
	m_contextMenu->Append(Event_Edit_HideSelected, DKLOC("TOKEN_HIDE", "Hide selection\tCtrl+H"));
	m_contextMenu->AppendSeparator();
	m_contextMenu->Append(Event_Edit_GroupSelected, DKLOC("TOKEN_GROUPSELECTED", "Group selection\tCtrl+G"));
	m_contextMenu->Append(Event_Edit_UngroupSelected, DKLOC("TOKEN_UNGROUPSELECTED", "Ungroup selection\tShift+Ctrl+G"));
	m_contextMenu->Append(Event_Edit_CloneSelected, DKLOC("TOKEN_CLONESELECTED", "Clone selected objects\tSpace"));
	m_contextMenu->AppendSeparator();
	m_contextMenu->Append(Event_Edit_Properties, DKLOC("TOKEN_PROPERTIES", "Properties...\tN"));
}

void CEditorFrame::ConstructToolbars()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxToolBar *pToolBar = new wxToolBar(this, -1,wxDefaultPosition,wxSize(36,400),wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL/* | wxTB_HORZ_TEXT*/);//CreateToolBar(wxTB_FLAT | wxTB_NODIVIDER | wxTB_VERTICAL);
	
    wxBitmap toolBarBitmaps[15];

	toolBarBitmaps[0] = wxBitmap("EqEditor/resource/tool_select.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[1] = wxBitmap("EqEditor/resource/tool_brush.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[2] = wxBitmap("EqEditor/resource/tool_surface.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[3] = wxBitmap("EqEditor/resource/tool_terrainpatch.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[4] = wxBitmap("EqEditor/resource/tool_entity.bmp", wxBITMAP_TYPE_BMP);

	toolBarBitmaps[7] = wxBitmap("EqEditor/resource/tool_clipper.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[8] = wxBitmap("EqEditor/resource/tool_vertexmanip.bmp", wxBITMAP_TYPE_BMP);

	toolBarBitmaps[9] = wxBitmap("EqEditor/resource/tool_hide.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[10] = wxBitmap("EqEditor/resource/tool_unhideall.bmp", wxBITMAP_TYPE_BMP);

	toolBarBitmaps[11] = wxBitmap("EqEditor/resource/tool_group.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[12] = wxBitmap("EqEditor/resource/tool_ungroup.bmp", wxBITMAP_TYPE_BMP);
	toolBarBitmaps[13] = wxBitmap("EqEditor/resource/tool_decal.bmp", wxBITMAP_TYPE_BMP);

    int w = toolBarBitmaps[Tool_Selection].GetWidth(),
        h = toolBarBitmaps[Tool_Selection].GetHeight();

    // this call is actually unnecessary as the toolbar will adjust its tools
    // size to fit the biggest icon used anyhow but it doesn't hurt neither
    pToolBar->SetToolBitmapSize(wxSize(w, h));

	pToolBar->AddSeparator();

#define ADD_TOOLRADIO(event_id, text, bitmap) \
	pToolBar->AddTool(event_id, text, bitmap, wxNullBitmap, wxITEM_RADIO ,text, text);

#define ADD_TOOLBUTTON(event_id, text, bitmap) \
	pToolBar->AddTool(event_id, text, bitmap, wxNullBitmap, wxITEM_NORMAL ,text, text);

	ADD_TOOLRADIO(Event_Tools_Selection, DKLOC("TOKEN_SELECTION", "Selection"),toolBarBitmaps[0]);
	ADD_TOOLRADIO(Event_Tools_Brush,  DKLOC("TOKEN_BRUSH", "Brushes"),toolBarBitmaps[1]);
	ADD_TOOLRADIO(Event_Tools_Surface,  DKLOC("TOKEN_SURFACES", "Surfaces"),toolBarBitmaps[2]);
	ADD_TOOLRADIO(Event_Tools_TerrainPatch,  DKLOC("TOKEN_TERRAINPATCHES", "Terrains"),toolBarBitmaps[3]);
	ADD_TOOLRADIO(Event_Tools_Decals,  DKLOC("TOKEN_DECALS", "Decals"),toolBarBitmaps[13]);
	ADD_TOOLRADIO(Event_Tools_DynObj, DKLOC("TOKEN_ENTITIES",  "Entities"),toolBarBitmaps[4]);
	ADD_TOOLRADIO(Event_Tools_VertexManip,  DKLOC("TOKEN_VERTEXMANIP", "Vertex nudge"),toolBarBitmaps[8]);
	ADD_TOOLRADIO(Event_Tools_Clipper,  DKLOC("TOKEN_CLIPPER", "Clipper"),toolBarBitmaps[7]);

	pToolBar->AddSeparator();

	ADD_TOOLBUTTON(Event_Edit_HideSelected, DKLOC("TOKEN_TOOLHIDE", "Hide"),toolBarBitmaps[9]);
	ADD_TOOLBUTTON(Event_Edit_UnHideAll,DKLOC("TOKEN_TOOLSHOWALL", "Show all"),toolBarBitmaps[10]);

	pToolBar->AddSeparator();

	ADD_TOOLBUTTON(Event_Edit_GroupSelected, DKLOC("TOKEN_TOOLGROUP", "Group selection"), toolBarBitmaps[11]);
	ADD_TOOLBUTTON(Event_Edit_UngroupSelected,DKLOC("TOKEN_TOOLUNGROUP", "Ungroup selection"), toolBarBitmaps[12]);

	/*
	m_projection_selector = new wxComboBox(NULL, -1, "Perspective");
	m_rendermode_selector = new wxComboBox(NULL, -1, "Textured");

	pToolBar->InsertControl(0, m_projection_selector);
	pToolBar->InsertControl(1, m_rendermode_selector);
	*/

	pToolBar->AddSeparator();

	pToolBar->Realize();

	GetSizer()->Add(pToolBar, 0, wxGROW);
	m_mgr.AddPane(pToolBar, wxAuiPaneInfo().CloseButton(false));

	CreateStatusBar(3);

	int status_width[] = {512,512,512};

	SetStatusWidths(3,status_width);

	SetStatusText("Ready");
	SetStatusText(varargs("Grid size: %s %d\n", m_snaptogrid->IsChecked() ? "On" : "Off", g_gridsize),2);
	
}

void CEditorFrame::OnConsoleMessage(SpewType_t type, const char* text)
{
	if(type == SPEW_ERROR)
		m_errors_out->AppendText(text);
	else if(type == SPEW_WARNING)
		m_warnings_out->AppendText(text);

	m_console_out->AppendText(text);
}

bool CEditorFrame::IsSnapToGridEnabled()
{
	return m_snaptogrid->IsChecked();
}

bool CEditorFrame::IsSnapToVisibleGridEnabled()
{
	return m_snaptovisiblegrid->IsChecked();
}

bool CEditorFrame::IsPhysmodelDrawEnabled()
{
	return m_drawphysmodels->IsChecked();
}

bool CEditorFrame::IsClipModelsEnabled()
{
	return m_clipmodels->IsChecked();
}

void CEditorFrame::SnapVector3D(Vector3D &val)
{
	if(IsSnapToGridEnabled())
	{
		if(m_snaptovisiblegrid->IsChecked())
		{
			int nGridSize = g_gridsize;

			for(int i = 0; i < 4; i++)
			{
				CViewWindow* win = GetViewWindow(i);

				if(win->IsMouseInWindow())
				{
					nGridSize = win->GetActiveView()->GetVisibleGridSizeBy(g_gridsize);
					break;
				}
			}

			val = SnapVector(nGridSize, val);
		}
		else
			val = SnapVector(g_gridsize, val);
	}
}

void CEditorFrame::SnapFloatValue(float &val)
{
	if(IsSnapToGridEnabled())
	{
		if(m_snaptovisiblegrid->IsChecked())
		{
			int nGridSize = g_gridsize;

			for(int i = 0; i < 4; i++)
			{
				CViewWindow* win = GetViewWindow(i);

				if(win->IsMouseInWindow())
				{
					nGridSize = win->GetActiveView()->GetVisibleGridSizeBy(g_gridsize);
					break;
				}
			}

			val = SnapFloat(nGridSize, val);
		}
		else
			val = SnapFloat(g_gridsize, val);
	}
}

extern void ResetViews();

void CEditorFrame::OpenLevelPrompt()
{
	if(!SavePrompt())
		return;

	// first we've need to rebuild level list
	m_loadleveldialog->RebuildLevelList();

	if(m_loadleveldialog->ShowModal() == wxID_OK)
	{
		g_pLevel->SetLevelName(m_loadleveldialog->GetSelectedLevelString());
		ResetViews();
		g_pLevel->Load();
		UpdateAllWindows();
	}

	if(g_pLevel->IsSavedOnDisk())
		AddRecentWorld((char*)g_pLevel->GetLevelName());

	SetTitle(wxString(g_pLevel->GetLevelName()) + wxString(" - ") + DKLOC("TOKEN_TITLE", "Equilibrium World Editor"));
}

void CEditorFrame::NewLevelPrompt()
{
	if(!SavePrompt())
		return;

	ResetViews();
	g_pLevel->CreateNew();
	UpdateAllWindows();

	SetTitle(wxString(g_pLevel->GetLevelName()) + wxString(" - ") + DKLOC("TOKEN_TITLE", "Equilibrium World Editor"));
}

bool CEditorFrame::SavePrompt(bool showQuestion, bool changeLevelName, bool bForceSave)
{
	if(!g_pLevel->IsNeedsSave() && !changeLevelName)
	{
		// if it's already on disk, save it
		if(g_pLevel->IsSavedOnDisk() && bForceSave)
			g_pLevel->Save();

		return true;
	}

	if(showQuestion)
	{
		int result = wxMessageBox(varargs((char*)DKLOC("TOKEN_SAVEWORLDQUESTION", "Do you want to save world '%s'?"),g_pLevel->GetLevelName()), "Question", wxYES_NO | wxCANCEL | wxCENTRE | wxICON_WARNING, this);

		if(result == wxCANCEL)
			return false;

		if(result == wxNO)
			return true;
	}

	if(!g_pLevel->IsSavedOnDisk() || changeLevelName)
	{
		m_levelsavedialog->SetValue(g_pLevel->GetLevelName());

		if(m_levelsavedialog->ShowModal() == wxID_OK)
		{
			g_pLevel->SetLevelName(m_levelsavedialog->GetValue());

			AddRecentWorld((char*)m_levelsavedialog->GetValue().c_str().AsChar());

			g_pLevel->Save();
		}
	}
	else
		g_pLevel->Save();

	SetTitle(wxString(g_pLevel->GetLevelName()) + wxString(" - ") + DKLOC("TOKEN_TITLE", "Equilibrium World Editor"));

	return true;
}

void CEditorFrame::OnClose(wxCloseEvent& event)
{
	if(!SavePrompt())
		return;

	m_levelsavedialog->Destroy();
	m_groupnamedialog->Destroy();


	Msg("EXIT CLEANUP...\n");
	g_pLevel->CleanLevel(true);

	SetSpewFunction(nullspew);
	Destroy();

	SaveRegOptions();

	ses->StopAllSounds();
	ses->Shutdown();

	soundsystem->Shutdown();

	g_pModelCache->ReleaseCache();

	// shutdown material system
	materials->Shutdown();
	GetFileSystem()->FreeModule(g_matsysmodule);

	// shutdown core
	GetCore()->Shutdown();
}

bool CastBrushToSurfaces(CBaseEditableObject* selection, void* userdata)
{
	if(selection->GetType() != EDITABLE_BRUSH)
		return false;

	CEditableBrush* pBrush = (CEditableBrush*)selection;

	CEditableSurface* pSurface = NULL;
	IMaterial* pMaterial = NULL;

	bool any_casted = false;

	for(int i = 0; i < pBrush->GetFaceCount(); i++)
	{
		if(pBrush->GetFace(i)->nFlags & STFL_SELECTED)
		{
			CEditableSurface* pNewSurface = pBrush->GetFacePolygon(i)->MakeEditableSurface();
			g_pLevel->AddEditable(pNewSurface);
			any_casted = true;
		}
	}

	return any_casted;
}

void CastSelectedBrushesToSurfaces()
{
	DkList<CBaseEditableObject*> marked_editables;
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		if(CastBrushToSurfaces(g_pLevel->GetEditable(i),NULL))
		{
			marked_editables.append(g_pLevel->GetEditable(i));
		}
	}

	for(int i = 0; i < marked_editables.numElem(); i++)
	{
		g_pLevel->RemoveEditable(marked_editables[i]);
	}
}

bool CastBrushToSurface(CBaseEditableObject* selection, void* userdata)
{
	if(selection->GetType() != EDITABLE_BRUSH)
		return false;

	CEditableBrush* pBrush = (CEditableBrush*)selection;

	DkList<eqlevelvertex_t> verts;
	DkList<int>				indices;

	int start_vert = 0;

	bool any_casted = false;

	for(int i = 0; i < pBrush->GetFaceCount(); i++)
	{
		if(pBrush->GetFace(i)->nFlags & STFL_SELECTED)
		{
			any_casted = true;

			int num_triangles = ((pBrush->GetFacePolygon(i)->vertices.numElem() < 4) ? 1 : (2 + pBrush->GetFacePolygon(i)->vertices.numElem() - 4));
			int num_indices = num_triangles*3;

			// add visual indices
			for(int j = 0; j < pBrush->GetFacePolygon(i)->vertices.numElem(); j++)
				verts.append(pBrush->GetFacePolygon(i)->vertices[j]);

			for(int j = 0; j < num_triangles; j++)
			{
				int idx0 = 0;
				int idx1 = j + 1;
				int idx2 = j + 2;

				// add vertex indices
				indices.append(start_vert+idx0);
				indices.append(start_vert+idx1);
				indices.append(start_vert+idx2);
			}

			start_vert += pBrush->GetFacePolygon(i)->vertices.numElem();
		}
	}

	if(any_casted)
	{
		CEditableSurface* pNewSurface = new CEditableSurface;
		pNewSurface->MakeCustomMesh(verts.ptr(),indices.ptr(),verts.numElem(), indices.numElem());
		pNewSurface->SetMaterial(pBrush->GetFace(0)->pMaterial);
		g_pLevel->AddEditable(pNewSurface);
	}

	return any_casted;
}

bool AddSelectedWindingsToList(CBaseEditableObject* selection, DkList<EqBrushWinding_t> &windings)
{
	if(selection->GetType() != EDITABLE_BRUSH)
		return false;

	CEditableBrush* pBrush = (CEditableBrush*)selection;

	bool bAnyAdded = false;

	for(int i = 0; i < pBrush->GetFaceCount(); i++)
	{
		if(pBrush->GetFace(i)->nFlags & STFL_SELECTED)
		{
			windings.append(*pBrush->GetFacePolygon(i));
			bAnyAdded = true;
		}
	}

	return bAnyAdded;
}

extern void SubdivideWindings(DkList<EqBrushWinding_t> &windings, DkList<bool> &bUseTable);

void CastSelectedBrushesToSurface()
{
	DkList<EqBrushWinding_t> windings;

	DkList<CBaseEditableObject*> marked_editables;
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		if(AddSelectedWindingsToList(g_pLevel->GetEditable(i), windings))
			marked_editables.append(g_pLevel->GetEditable(i));
	}

	DkList<bool> bUseTable;

	SubdivideWindings(windings, bUseTable);

	for(int i = 0; i < windings.numElem(); i++)
	{
		if(!bUseTable[i])
			continue;

		CEditableSurface* pNewSurf = windings[i].MakeEditableSurface();
		g_pLevel->AddEditable(pNewSurf);
	}

	for(int i = 0; i < marked_editables.numElem(); i++)
		g_pLevel->RemoveEditable(marked_editables[i]);

	/*
	DkList<CBaseEditableObject*> marked_editables;
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		if(CastBrushToSurface(g_pLevel->GetEditable(i),NULL))
		{
			marked_editables.append(g_pLevel->GetEditable(i));
		}
	}

	for(int i = 0; i < marked_editables.numElem(); i++)
	{
		g_pLevel->RemoveEditable(marked_editables[i]);
	}
	*/
}

void CloneSelection()
{
	if(g_editormainframe->GetToolType() != Tool_Selection)
		return;

	DkList<CBaseEditableObject*> *pObjects = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();

	CMemoryStream* pCopyStream = (CMemoryStream*)OpenMemoryStream(VS_OPEN_WRITE, 4096);

	// save as level
	g_pLevel->SaveObjects(pCopyStream, pObjects->ptr(), pObjects->numElem());

	pCopyStream->Close();

	pCopyStream->Open(NULL, VS_OPEN_READ, 0);

	DkList<CBaseEditableObject*> pRestoredObjects;

	// just load objects from our level, and also select them
	g_pLevel->LoadObjects(pCopyStream, pRestoredObjects);

	pCopyStream->Close();

	CObjectAction* pNewAction = new CObjectAction;
	pNewAction->Begin();
	pNewAction->SetObjectIsCreated();

	for(int i = 0; i < pRestoredObjects.numElem(); i++)
	{
		g_pLevel->AddEditable( pRestoredObjects[i], false );
		pNewAction->AddObject(pRestoredObjects[i]);
	}

	pCopyStream->Close();
	delete pCopyStream;

	((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection(true);
	((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
	((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, pRestoredObjects.ptr(), pRestoredObjects.numElem());

	pNewAction->End();
	g_pLevel->AddAction(pNewAction);
}

void DeleteSelectedObjects()
{
	DkList<CBaseEditableObject*> *pObjects = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();

	for(int i = 0; i < pObjects->numElem(); i++)
	{
		g_pLevel->RemoveEditable(pObjects->ptr()[i]);
		i--;
	}
}

void Reset3DView()
{
	g_editormainframe->GetViewWindow(0)->GetActiveView()->GetView()->SetOrigin(vec3_zero);
	g_editormainframe->GetViewWindow(0)->GetActiveView()->GetView()->SetAngles(vec3_zero);
	UpdateAllWindows();
}

void TargetAllViewsToSelectionCenter()
{
	if(((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectionState() == SELECTION_NONE)
		return;

	Vector3D pos = g_editormainframe->GetViewWindow(0)->GetActiveView()->GetView()->GetOrigin();
	Vector3D sel_center = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectionCenter();
	Vector3D dir = normalize(sel_center - pos);

	Vector3D angles = VectorAngles(dir);
	g_editormainframe->GetViewWindow(0)->GetActiveView()->GetView()->SetAngles(angles);

	// target another views to selection center
	g_editormainframe->GetViewWindow(1)->GetActiveView()->GetView()->SetOrigin(sel_center);
	g_editormainframe->GetViewWindow(2)->GetActiveView()->GetView()->SetOrigin(sel_center);
	g_editormainframe->GetViewWindow(3)->GetActiveView()->GetView()->SetOrigin(sel_center);
	UpdateAllWindows();
}

void TranslateAllViewsToSelectionCenter()
{
	if(((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectionState() == SELECTION_NONE)
		return;

	Vector3D sel_center = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectionCenter();

	g_editormainframe->GetViewWindow(0)->GetActiveView()->GetView()->SetOrigin(sel_center);

	g_editormainframe->GetViewWindow(1)->GetActiveView()->GetView()->SetOrigin(sel_center);
	g_editormainframe->GetViewWindow(2)->GetActiveView()->GetView()->SetOrigin(sel_center);
	g_editormainframe->GetViewWindow(3)->GetActiveView()->GetView()->SetOrigin(sel_center);
	UpdateAllWindows();
}

void SelectionArbitaryTransform(CTransformSelectionDialog* pDialog)
{
	DkList<CBaseEditableObject*> pObjects;
	pObjects.append(*((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects());

	Vector3D selection_center = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectionCenter();

	((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

	for(int i = 0; i < pObjects.numElem(); i++)
	{
		CBaseEditableObject* pObject = (CBaseEditableObject*)pObjects.ptr()[i];
		
		switch(pDialog->GetMode())
		{
		case 0:
			pObject->Translate(pDialog->GetModifyValue());
			break;
		case 1:
			pObject->Rotate(pDialog->GetModifyValue(),pDialog->IsOverSelectionCenter(), selection_center);
			break;
		case 2:
			pObject->Scale(pDialog->GetModifyValue(),pDialog->IsOverSelectionCenter(), selection_center);
			break;
		}
	}

	((CSelectionBaseTool*)g_pSelectionTools[0])->TransformByPreview();

	UpdateAllWindows();
}

void GroupSelection(const char* pszName)
{
	// don't group less than two objects
	if(((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects()->numElem() <= 1)
		return;

	CEditableGroup* pGroup = new CEditableGroup;
	pGroup->SetName(pszName);

	pGroup->InitGroup(*((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects());

	g_pLevel->AddEditable(pGroup);
}

void UngroupSelection()
{
	DkList<CBaseEditableObject*>* pSelection = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();

	// try to remove selected objects from all groups
	for(int i = 0; i < g_pLevel->GetEditableCount(); i++)
	{
		if(g_pLevel->GetEditable(i)->GetType() != EDITABLE_GROUP)
			continue;

		CEditableGroup* pGroup = (CEditableGroup*)g_pLevel->GetEditable(i);

		for(int j = 0; j < pSelection->numElem(); j++)
			pGroup->RemoveObject(pSelection->ptr()[j]);

		/*
		if(pGroup->GetGroupList()->numElem() == 0)
		{
			g_pLevel->RemoveEditable(pGroup);
			i--;
		}
		*/
	}

	g_pLevel->NotifyUpdate();
}

// callback to process each selected objects
bool FlipEditableSurfFaces(CBaseEditableObject* selection, void* userdata)
{
	if(selection->GetType() <= EDITABLE_TERRAINPATCH)
	{
		CEditableSurface* pSurf = (CEditableSurface*)selection;
		pSurf->FlipFaces();
	}

	return false;
}

void JoinSelectedSurfaces()
{
	DkList<CBaseEditableObject*> *pSelection = ((CSelectionBaseTool*)g_pSelectionTools[0])->GetSelectedObjects();
	CEditableSurface* pSurface = NULL;

	int numSurfaces = 0;

	for(int i = 0; i < pSelection->numElem(); i++)
	{
		if(pSelection->ptr()[i]->GetType() <= EDITABLE_TERRAINPATCH)
		{
			// detected
			if(!pSurface)
				pSurface = (CEditableSurface*)pSelection->ptr()[i];

			numSurfaces++;
		}
	}

	if(pSurface && numSurfaces > 1)
	{
		DkList<CBaseEditableObject*> pRemovalObjects;

		((CSelectionBaseTool*)g_pSelectionTools[0])->BackupSelectionForUndo();

		pSurface->BeginModify();

		for(int i = 0; i < pSelection->numElem(); i++)
		{
			if((pSelection->ptr()[i]->GetType() <= EDITABLE_TERRAINPATCH) && (pSurface != pSelection->ptr()[i]))
			{
				// detected
				pSurface->AddSurfaceGeometry( (CEditableSurface*)pSelection->ptr()[i] );

				// automatically remove editable
				pRemovalObjects.append(pSelection->ptr()[i]);
			}
		}

		pSurface->EndModify();
		
		((CSelectionBaseTool*)g_pSelectionTools[0])->CancelSelection();

		for(int i = 0; i < pRemovalObjects.numElem(); i++)
			g_pLevel->RemoveEditable(pRemovalObjects[i]);
		
		((CSelectionBaseTool*)g_pSelectionTools[0])->SetSelectionState(SELECTION_PREPARATION);
		((CSelectionBaseTool*)g_pSelectionTools[0])->SelectObjects(true, (CBaseEditableObject**)&pSurface, 1);
	}
}

extern void AddOBJToWorld(const char* pszFileName);

void CEditorFrame::ProcessAllMenuCommands(wxCommandEvent& event)
{
	switch(event.GetId())
	{
		case Event_Tools_CastBrushToSurfaces:
			CastSelectedBrushesToSurfaces();
			break;
		case Event_Tools_CastBrushToSurface:
			CastSelectedBrushesToSurface();
			break;
		case Event_File_SaveLevel:
			SavePrompt(false, false, true);
			break;
		case Event_File_SaveLevelAs:
			SavePrompt(false, true);
			break;
		case Event_File_OpenLevel:
			OpenLevelPrompt();
			break;
		case Event_File_NewLevel:
			NewLevelPrompt();
			break;
		case Event_File_RecentLevel:
		case Event_File_RecentLevel+1:
		case Event_File_RecentLevel+2:
		case Event_File_RecentLevel+3:
		case Event_File_RecentLevel+4:
		case Event_File_RecentLevel+5:
		case Event_File_RecentLevel+6:
		case Event_File_RecentLevel+7:
		case Event_File_RecentLevel+8:
		{
			EqString levelName = g_recentPath[event.GetId()-Event_File_RecentLevel];

			g_pLevel->SetLevelName(levelName.GetData());
			ResetViews();
			if(g_pLevel->Load())
			{
				UpdateAllWindows();

				// set to last
				AddRecentWorld((char*)g_pLevel->GetLevelName());
			}

			SetTitle(wxString(g_pLevel->GetLevelName()) + wxString(" - ") + DKLOC("TOKEN_TITLE", "Equilibrium World Editor"));

			break;
		}
			/*
		case Event_File_Import:
		{
			wxFileDialog *file = new wxFileDialog(NULL, "Import map files...", "./", "*.map", "Radiant or WorldCraft map files (*.map)|*.map;", wxFD_FILE_MUST_EXIST | wxFD_OPEN);

			if(file->ShowModal() == wxID_OK)
			{
				g_pLevel->ImportMapFile(file->GetPath());
			}
			delete file;
			break;
		}
		*/
		case Event_File_Exit:
			if(!SavePrompt())
				return;

			Close(TRUE);
			break;
		case Event_Edit_InvertSelection:
			((CSelectionBaseTool*)g_pSelectionTools[0])->InvertSelection();
			break;
		case Event_Edit_Cut:
			g_pLevel->ClipboardAction( CLIPBOARD_COPY );
			DeleteSelectedObjects();
			break;
		case Event_Edit_Copy:
			g_pLevel->ClipboardAction( CLIPBOARD_COPY );
			break;
		case Event_Edit_Paste:
			g_pLevel->ClipboardAction( CLIPBOARD_PASTE );
			break;
		case Event_Edit_HideSelected:
			OnDeactivateCurrentTool();
			g_pLevel->HideSelectedObjects();
			OnActivateCurrentTool();
			break;
		case Event_Edit_UnHideAll:
			g_pLevel->UnhideAll();
			break;
		case Event_Edit_CloneSelected:
			CloneSelection();
			break;
		case Event_Edit_GroupSelected:

			if(GetToolType() != Tool_Selection)
				return;

			if(m_groupnamedialog->ShowModal() == wxID_OK)
				GroupSelection(m_groupnamedialog->GetValue());

			break;
		case Event_Edit_UngroupSelected:
			UngroupSelection();
			break;
		case Event_Edit_Properties:
			m_entprops->Show();
			m_entprops->SetFocus();
			break;
		case Event_Edit_Undo:
			g_pLevel->Undo();
			break;
		case Event_Edit_Redo:

			break;
		case Event_Edit_ArbitaryTransform:
			if(m_transformseldialog->ShowModal() == wxID_OK)
			{
				SelectionArbitaryTransform(m_transformseldialog);
			}
			break;
		case Event_Tools_ImportOBJ:
			{
				wxFileDialog *file = new wxFileDialog(NULL, "Import OBJ", "./", "*.obj", "Wavefront OBJ files (*.obj)|*.obj;", wxFD_FILE_MUST_EXIST | wxFD_OPEN);

				if(file->ShowModal() == wxID_OK)
				{
					AddOBJToWorld(file->GetPath());
				}
				delete file;
				break;
			}
		case Event_Tools_Selection:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Selection;
			OnActivateCurrentTool();
			break;
		case Event_Tools_Brush:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Brush;
			OnActivateCurrentTool();
			break;
		case Event_Tools_Surface:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Surface;
			OnActivateCurrentTool();
			break;
		case Event_Tools_DynObj:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Entity;
			OnActivateCurrentTool();
			break;
		case Event_Tools_TerrainPatch:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Patch;
			OnActivateCurrentTool();
			break;
		case Event_Tools_Clipper:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Clipper;
			OnActivateCurrentTool();
			break;
		case Event_Tools_PathTool:
			OnDeactivateCurrentTool();
			m_nTool = Tool_PathTool;
			OnActivateCurrentTool();
			break;
		case Event_Tools_VertexManip:
			OnDeactivateCurrentTool();
			m_nTool = Tool_VertexManip;
			OnActivateCurrentTool();
			break;
		case Event_Tools_Decals:
			OnDeactivateCurrentTool();
			m_nTool = Tool_Decals;
			OnActivateCurrentTool();
			break;
		case Event_Tools_ReloadMaterials:
			GetMaterials()->ReloadMaterialList();
			break;
		case Event_Tools_FlipSelectedModelFaces:
			((CSelectionBaseTool*)g_pSelectionTools[0])->DoForEachSelectedObjects(FlipEditableSurfFaces, NULL);
			break;
		case Event_Tools_JoinSelectedSurfaces:
			JoinSelectedSurfaces();
			break;
		case Event_Level_BuildWorld:
			g_pLevel->BuildWorld();
			break;
		case Event_Level_StartGame:
		{
			EqString args;
			args.Clear();

			args.Append( ENGINE_EXECUTABLE_NAME " " );
			args.Append(varargs("-game \"%s\" ", GetFileSystem()->GetCurrentGameDirectory()));
			args.Append( varargs("+start_world \"%s\" ", g_pLevel->GetLevelName()) );

			// there is a console window that user can close and resume editing world
			system( args.GetData() );
			break;
		}
		case Event_View_Reset3DView:
			Reset3DView();
			break;
		case Event_View_TargetViews:
			TargetAllViewsToSelectionCenter();
			break;
		case Event_View_TranslateViews:
			TranslateAllViewsToSelectionCenter();
			break;
		case Event_Window_SurfaceDialog:
		{
			SetWindowPos(m_surfacedialog->GetHWND(),HWND_TOP,0,0,0,0,SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
			m_surfacedialog->Show();
			m_surfacedialog->SetFocus();
			break;
		}
		case Event_Window_EntityList:
		{
			m_entlistdialog->Show();
			m_entlistdialog->SetFocus();
			break;
		}
		case Event_Window_GroupList:
		{
			m_grplistdialog->Show();
			m_grplistdialog->SetFocus();
			break;
		}
		case Event_Window_SoundList:
		{
			m_sndlistdialog->Show();
			m_sndlistdialog->SetFocus();
			break;
		}
		case Event_Window_MaterialsWindow:
		{
			m_texturelist->Show();
			m_texturelist->SetFocus();
			break;
		}
		case Event_Window_ToggleOutput:
		{
			if(!m_output_notebook->IsShown())
			{
				m_output_notebook->Show();
				m_mgr.GetPane(m_output_notebook).Show();
			}
			else
			{
				m_mgr.GetPane(m_output_notebook).Hide();
			}

			// tell the manager to "commit" all the changes just made
			m_mgr.Update();
			break;
		}
		case Event_Help_About:
			wxMessageBox("Equilibrium Game Engine World Editor\n\n\nCopyright (C) 2012\
						 Damage Byte L.L.C. All rights reserved\n\n\nVersion 0.67b\n\n\
						 Programmed by Shurumov Ilya\nTesters: Shurumov Ilya, Seytkamalov Damir", "About Equilibrium Level Editor", wxOK | wxICON_INFORMATION, this);
			break;
		case Event_View_SnapToGrid:
			SetStatusText(varargs("Grid size: %s %d\n", m_snaptogrid->IsChecked() ? "On" : "Off", g_gridsize),2);
			break;
		case Event_View_GridSpacing:
		case Event_View_GridSpacing + 1:
		case Event_View_GridSpacing + 2:
		case Event_View_GridSpacing + 3:
		case Event_View_GridSpacing + 4:
		case Event_View_GridSpacing + 5:
		case Event_View_GridSpacing + 6:
		case Event_View_GridSpacing + 7:
		case Event_View_GridSpacing + 8:
		{
			g_gridsize = (1 << (event.GetId() - Event_View_GridSpacing));

			SetStatusText(varargs("Grid size: %s %d\n", m_snaptogrid->IsChecked() ? "On" : "Off", g_gridsize),2);
			break;
		}
	}

	UpdateAllWindows();
}

bool CEditorFrame::IsPreviewEnabled()
{
	return m_preview->IsChecked();
}

CViewWindow* CEditorFrame::GetViewWindow(int id)
{
	return m_view_windows[id];
}

CSurfaceDialog* CEditorFrame::GetSurfaceDialog()
{
	return m_surfacedialog;
}

ToolType_e CEditorFrame::GetToolType()
{
	return m_nTool;
}

CEntityPropertiesPanel* CEditorFrame::GetEntityPropertiesPanel()
{
	return m_entprops;
}

CBuildOptionsDialog* CEditorFrame::GetBuildOptionsDialog()
{
	return m_buildoptions;
}

CGroupList*	CEditorFrame::GetGroupListDialog()
{
	return m_grplistdialog;
}

CEntityList* CEditorFrame::GetEntityListDialog()
{
	return m_entlistdialog;
}

CSoundList* CEditorFrame::GetSoundListDialog()
{
	return m_sndlistdialog;
}

CWorldLayerPanel* CEditorFrame::GetWorldLayerPanel()
{
	return m_worldlayers;
}

static editor_user_prefs_t s_editCfg;
editor_user_prefs_t* g_editorCfg = &s_editCfg;

void LoadEditorConfig()
{
	// setup defaults
	g_editorCfg->background_color = ColorRGB(0.25,0.25,0.25);
	g_editorCfg->grid1_color = ColorRGBA(0.35,0.35,0.35,1);
	g_editorCfg->grid2_color = ColorRGBA(0.55,0.55,0.55,1);
	g_editorCfg->level_center_color = ColorRGB(0,0.45f,0.45f);
	g_editorCfg->selection_color = ColorRGB(1,0.25f,0.25f);
	g_editorCfg->default_grid_size = 8;
	g_editorCfg->zoom_2d_gridsnap = false;
	g_editorCfg->matsystem_threaded = true;
	g_editorCfg->default_material = "dev/dev_generic";

	KeyValues kv;

	if(kv.LoadFromFile("EqEditor/EqEditSettings.CFG"))
	{
		kvkeybase_t* pSection = kv.GetRootSection();

		kvkeybase_t* pPair = pSection->FindKeyBase("matsystem_threaded");
		if(pPair)
			g_editorCfg->matsystem_threaded = KV_GetValueBool(pPair);

		pPair = pSection->FindKeyBase("background_color");
		if(pPair)
			g_editorCfg->background_color = UTIL_StringToColor3(pPair->values[0]);

		pPair = pSection->FindKeyBase("grid1_color");
		if(pPair)
			g_editorCfg->grid1_color = UTIL_StringToColor4(pPair->values[0]);

		pPair = pSection->FindKeyBase("grid2_color");
		if(pPair)
			g_editorCfg->grid2_color = UTIL_StringToColor4(pPair->values[0]);

		pPair = pSection->FindKeyBase("level_center_color");
		if(pPair)
			g_editorCfg->level_center_color = UTIL_StringToColor3(pPair->values[0]);

		pPair = pSection->FindKeyBase("selection_color");
		if(pPair)
			g_editorCfg->selection_color = UTIL_StringToColor3(pPair->values[0]);

		pPair = pSection->FindKeyBase("default_grid_size");
		if(pPair)
			g_editorCfg->default_grid_size = atoi(pPair->values[0]);

		pPair = pSection->FindKeyBase("zoom_2d_gridsnap");
		if(pPair)
			g_editorCfg->zoom_2d_gridsnap = atoi(pPair->values[0]);

		pPair = pSection->FindKeyBase("default_material");
		if(pPair)
			g_editorCfg->default_material = pPair->values[0];
	}
}

void SaveEditorConfig()
{

}