//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem wxWidgets template application
//////////////////////////////////////////////////////////////////////////////////

#include "MainWindow.h"
#include "IFileSystem.h"
#include "ILocalize.h"
#include "FontCache.h"
#include "IConCommandFactory.h"
#include "../../shared_engine/DebugOverlay.h"

#include <wx/settings.h>

#define APPLICATION_NAME		"MatSystem_wxAppTest"
#define LOCALIZED_FILE_PREFIX	"wxAppTest"

#define TITLE_TOKEN				"Equilibrium MatSystem wxApp template"	// you can use hashtag # to use localization token

static CDebugOverlay g_DebugOverlays;
IDebugOverlay *debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

DKMODULE*			g_matsysmodule = NULL;
IShaderAPI*			g_pShaderAPI = NULL;
IMaterialSystem*	materials = NULL;

CViewParams			g_pCameraParams(Vector3D(0,0,-100), vec3_zero, 70.0f);
Matrix4x4			g_mProjMat, g_mViewMat;
Volume				g_viewFrustum;

sceneinfo_t			scinfo;

float				g_fRealtime = 0.0f;
float				g_fOldrealtime = 0.0f;
float				g_fFrametime = 0.0f;

Vector3D			g_camera_rotation(25,225,0);
Vector3D			g_camera_target(0);
float				g_fCamSpeed = 10.0;

class CWXTemplateApplication: public wxApp
{
    virtual bool OnInit();
	virtual int	 OnExit();

	// TODO: more things
};

//
// main frame
//

// GUI events
enum
{
	// FILE

	Event_File_Load = 1000,
	Event_File_Save,
	Event_File_Exit,

	Event_Max_Menu_Range,

	Event_View_ResetView,

	// combobox
	Event_Sequence_Changed,
	Event_PoseCont_Changed,

	// buttons
	Event_Sequence_Play,
	Event_Sequence_Stop,
	Event_Physics_SimulateToggle,

	// slider
	Event_Timeline_Set,
	Event_PoseCont_ValueChanged,
};



BEGIN_EVENT_TABLE(CMainWindow, wxFrame)
	EVT_SIZE(CMainWindow::OnSize)
	EVT_ERASE_BACKGROUND(CMainWindow::OnEraseBackground)
	EVT_IDLE(CMainWindow::OnIdle)
	EVT_PAINT(CMainWindow::OnPaint)
	EVT_COMBOBOX(-1, CMainWindow::OnComboboxChanged)
	EVT_BUTTON(-1, CMainWindow::OnButtons)
	EVT_SLIDER(-1, CMainWindow::OnButtons)

	EVT_MENU_RANGE(Event_File_Load, Event_Max_Menu_Range, CMainWindow::ProcessAllMenuCommands)

	/*
	EVT_KEY_DOWN(ProcessKeyboardDownEvents)
	EVT_KEY_UP(ProcessKeyboardUpEvents)
	EVT_CONTEXT_MENU(OnContextMenu)
	EVT_SET_FOCUS(OnFocus)
	*/
END_EVENT_TABLE()

DkList<shaderfactory_t> pShaderRegistrators;

void InitMatSystem(EQWNDHANDLE window)
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

#ifdef _WIN32
		bool materialSystemStatus = materials->Init("materials/", "eqD3D9RHI", materials_config);
#elif LINUX
        bool materialSystemStatus = materials->Init("materials/", "libeqNullRHI.so", materials_config);
#endif // _WIN32

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

	//materials->LoadShaderLibrary("Shaders_Engine.dll");

	// register all shaders
	for(int i = 0; i < pShaderRegistrators.numElem(); i++)
		materials->RegisterShader( pShaderRegistrators[i].shader_name, pShaderRegistrators[i].dispatcher );
}

CMainWindow::CMainWindow( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style )
	: wxFrame( parent, id, title, pos, size, style )
{
#ifdef _WIN32
	wxIcon ico;
	if(ico.LoadFile("IDI_MAINICON"))
	{
		SetIcon(ico);
	}
	else
		MsgError("Can't load icon!\n");
#endif // _WIN32

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	m_renderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_renderPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );

	bSizer1->Add( m_renderPanel, 1, wxEXPAND, 5 );

	this->SetSizer( bSizer1 );
	this->Layout();
	m_pMenu = new wxMenuBar( 0 );
	m_menu_file = new wxMenu();
	m_pMenu->Append( m_menu_file, wxT("File") );

	m_menu_file->Append( Event_File_Exit, DKLOC("TOKEN_EXIT", L"Exit") );

	m_menu_edit = new wxMenu();
	m_pMenu->Append( m_menu_edit, wxT("Edit") );

	m_menu_view = new wxMenu();
	m_pMenu->Append( m_menu_view, wxT("View") );

	this->SetMenuBar( m_pMenu );
	this->Centre( wxBOTH );

	Connect(wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&CMainWindow::OnCloseCmd, NULL, this);

	InitMatSystem( (EQWNDHANDLE)m_renderPanel->GetHandle() );

	g_fontCache->Init();

	debugoverlay->Init();

	m_renderPanel->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), NULL, this);
	m_renderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), NULL, this);
	m_renderPanel->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), NULL, this);

	m_renderPanel->Connect(wxEVT_MIDDLE_DCLICK, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_renderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_renderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);

	m_renderPanel->Connect(wxEVT_RIGHT_DCLICK, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_renderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_renderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);


	m_renderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);
	m_renderPanel->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, NULL, this);

	m_renderPanel->Connect(wxEVT_KEY_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessKeyboardDownEvents, NULL, this);
	m_renderPanel->Connect(wxEVT_KEY_UP, (wxObjectEventFunction)&CMainWindow::ProcessKeyboardUpEvents, NULL, this);

	m_bIsMoving = false;
	m_bDoRefresh = false;

	RefreshGUI();
}

void CMainWindow::OnIdle(wxIdleEvent &event)
{
	ReDraw();

	event.RequestMore(true);
}

void CMainWindow::ProcessMouseEnter(wxMouseEvent& event)
{
	//CaptureMouse();
}

void CMainWindow::ProcessMouseLeave(wxMouseEvent& event)
{
	//ReleaseMouse();
}

void CMainWindow::ProcessAllMenuCommands(wxCommandEvent& event)
{
	if(event.GetId() == Event_File_Exit)
	{
		Close();
	}
	else if(event.GetId() == Event_View_ResetView)
	{
		g_camera_rotation = Vector3D(25,225,0);
		g_camera_target = vec3_zero;
		g_fCamSpeed = 10.0f;
	}
}

Vector2D		g_vLastMousePosition(0);

void CMainWindow::ProcessMouseEvents(wxMouseEvent& event)
{
	m_renderPanel->SetFocus();

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
	m_renderPanel->GetSize(&w, &h);

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

				cam_pos -= right*move_delta_x * camera_move_factor * g_fCamSpeed * g_frametime;
				cam_pos += up*move_delta_y * camera_move_factor * g_fCamSpeed * g_frametime;

				g_vLastMousePosition = prev_mouse_pos;
			}
		}
	}
	else
	{
		//
		// Do whatever you want
		//
	}

	if(!bAnyMoveButton)
	{
		m_bIsMoving = false;
	}

	if(m_bIsMoving)
	{
		SetCursorPos(m_vLastClientCursorPos.x, m_vLastClientCursorPos.y);
	}

	// process zooming
	float mouse_wheel = event.GetWheelRotation()*0.5f * zoom_factor;

	fov += mouse_wheel;

	fov = clamp(fov,10,1024);

	g_fCamSpeed = fov;

	cam_pos = clamp(cam_pos, Vector3D(-MAX_COORD_UNITS), Vector3D(MAX_COORD_UNITS));

	g_camera_rotation = cam_angles;
	g_camera_target = cam_pos;
}

void CMainWindow::ProcessKeyboardDownEvents(wxKeyEvent& event)
{
	// DO WHATEVER YOU WANT
}

void CMainWindow::ProcessKeyboardUpEvents(wxKeyEvent& event)
{
	// DO WHATEVER YOU WANT
}

void CMainWindow::GetMouseScreenVectors(int x, int y, Vector3D& origin, Vector3D& dir)
{
	int w, h;
	m_renderPanel->GetSize(&w, &h);

	ScreenToDirection(g_pCameraParams.GetOrigin(), Vector2D(w-x,y), Vector2D(w,h), origin, dir, g_mProjMat*g_mViewMat, false);

	dir = normalize(dir);
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

void GR_BuildViewMatrices(int width, int height, int nRenderFlags)
{
	Vector3D vRadianRotation = VDEG2RAD(g_pCameraParams.GetAngles());

	// this is negated matrix
	Matrix4x4 projMatrix;

	g_mProjMat	= perspectiveMatrixY(DEG2RAD(g_pCameraParams.GetFOV()), width, height, 1, 1000);

	g_mViewMat = rotateZXY4(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);
	g_mViewMat.translate(-g_pCameraParams.GetOrigin());

	// store the viewprojection matrix for some purposes
	Matrix4x4 viewProj = g_mProjMat * g_mViewMat;

	materials->SetMatrix(MATRIXMODE_PROJECTION, g_mProjMat);
	materials->SetMatrix(MATRIXMODE_VIEW, g_mViewMat);

	g_viewFrustum.LoadAsFrustum(viewProj);
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
		int w, h;
		m_renderPanel->GetSize(&w,&h);

		materials->SetDeviceBackbufferSize(w,h);

		m_bDoRefresh = false;
	}

	// compute time since last frame
	g_realtime = Platform_GetCurrentTime();

	float fps = 100;

	if ( fps != 0 )
	{
		// Limit fps to withing tolerable range
		fps = max( MIN_FPS, fps );
		fps = min( MAX_FPS, fps );

		float minframetime = 1.0 / fps;

		if(( g_realtime - g_oldrealtime ) < minframetime )
			return;
	}

	g_frametime = g_realtime - g_oldrealtime;
	g_oldrealtime = g_realtime;

	// update material system and proxies
	materials->Update(g_frametime);

	int w, h;
	m_renderPanel->GetSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w,h);

	if(materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.0,0.0,0.0, 1));

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
		g_mProjMat = perspectiveMatrixY(DEG2RAD(g_pCameraParams.GetFOV()), w, h, 1, 5000);

		g_mViewMat = rotateZXY4(DEG2RAD(-g_pCameraParams.GetAngles().x),DEG2RAD(-g_pCameraParams.GetAngles().y),DEG2RAD(-g_pCameraParams.GetAngles().z));
		g_mViewMat.translate(-g_pCameraParams.GetOrigin());

		//GR_BuildViewMatrices(w,h, nRenderFlags);

		materials->SetMatrix(MATRIXMODE_PROJECTION, g_mProjMat);
		materials->SetMatrix(MATRIXMODE_VIEW, g_mViewMat);

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		int nRenderFlags = 0;

		// draw axe
		debugoverlay->Line3D(vec3_zero, Vector3D(1,0,0), ColorRGBA(0,0,0,1), ColorRGBA(1,0,0,1));
		debugoverlay->Line3D(vec3_zero, Vector3D(0,1,0), ColorRGBA(0,0,0,1), ColorRGBA(0,1,0,1));
		debugoverlay->Line3D(vec3_zero, Vector3D(0,0,1), ColorRGBA(0,0,0,1), ColorRGBA(0,0,1,1));

		debugoverlay->Draw(g_mProjMat, g_mViewMat, w,h);

        // TODO: swap chain
		materials->EndFrame( NULL );
		Platform_Sleep(1);
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
	Msg("EXIT CLEANUP...\n");

	Destroy();

	// shutdown material system
	materials->Shutdown();

	materials = NULL;

	g_fileSystem->FreeModule(g_matsysmodule);

	// shutdown core
	GetCore()->Shutdown();
}

void CMainWindow::OnButtons(wxCommandEvent& event)
{
	// TODO: selected slots


}

//#undef IMPLEMENT_WXWIN_MAIN

bool InitCore(const char *pCmdLine)
{
	// initialize core
	GetCore()->Init( APPLICATION_NAME, pCmdLine );

	if(!g_fileSystem->Init(false))
		return false;

	return true;
}

IMPLEMENT_APP(CWXTemplateApplication)

//typedef int (*winmain_wx_cb)(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *pCmdLine, int nCmdShow);

//winmain_wx_cb g_wxEntryCallback = wxEntry;

CMainWindow *g_pMainFrame = NULL;

bool CWXTemplateApplication::OnInit()
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

#ifdef _WIN32
	InitCore(GetCommandLineA());
#elif __WXGTK__
    InitCore("");
#endif

	setlocale(LC_ALL,"C");

	// first, load matsystem module
#ifdef _WIN32
	g_matsysmodule = g_fileSystem->LoadModule("EqMatSystem.dll");
#elif LINUX
    g_matsysmodule = g_fileSystem->LoadModule("libeqMatSystem.so");
#endif

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		return -1;
	}

	g_localizer->AddTokensFile(LOCALIZED_FILE_PREFIX);

	g_pMainFrame = new CMainWindow( NULL, -1, LocalizedString( TITLE_TOKEN ));
	g_pMainFrame->Centre();
	g_pMainFrame->Show(true);

	SetTopWindow(g_pMainFrame);

    return true;
}

int CWXTemplateApplication::OnExit()
{
	//WriteCfgFile("cfg/editor.cfg");

    return 0;
}
