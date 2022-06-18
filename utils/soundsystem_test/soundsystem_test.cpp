//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: MaterialSystem wxWidgets template application
//////////////////////////////////////////////////////////////////////////////////

#include <wx/settings.h>

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ILocalize.h"
#include "core/IConsoleCommands.h"
#include "core/IEqParallelJobs.h"

#include "soundsystem_test.h"

#include "font/IFontCache.h"
#include "math/Utility.h"
#include "math/Random.h"

#include "materialsystem1/IMaterialSystem.h"
#include "render/ViewParams.h"
#include "render/IDebugOverlay.h"

#include "audio/IEqAudioSystem.h"


#define APPLICATION_NAME		"SoundTest"
#define LOCALIZED_FILE_PREFIX	"wxAppTest"

#define TITLE_TOKEN				"Equilibrium Sound Engine Test"	// you can use hashtag # to use localization token

static eqJobThreadDesc_t s_jobTypes[] = {
	//{JOB_TYPE_ANY, 1},
	{JOB_TYPE_AUDIO, 1},
	//{JOB_TYPE_PHYSICS, 1},
	{JOB_TYPE_RENDERER, 1},
	{JOB_TYPE_PARTICLES, 1},
	{JOB_TYPE_DECALS, 1},
	{JOB_TYPE_SPOOL_AUDIO, 1},
	{JOB_TYPE_SPOOL_EGF, 2},
	{JOB_TYPE_SPOOL_WORLD, 1},
	{JOB_TYPE_OBJECTS, 1},
};


DKMODULE*			g_matsysmodule = nullptr;
IShaderAPI*			g_pShaderAPI = nullptr;
IMaterialSystem*	materials = nullptr;

CViewParams			g_pCameraParams(Vector3D(0,0,-100), vec3_zero, 70.0f);
Matrix4x4			g_mProjMat, g_mViewMat;
Volume				g_viewFrustum;

sceneinfo_t			scinfo;

Vector3D			g_camera_rotation(25,225,0);
Vector3D			g_camera_target(-4,2.65,4);
float				g_fCamSpeed = 10.0;

CEqTimer			g_timer;
float				g_frametime = 0.0f;

#define SOUND_COUNT_TEST 4
#define MAX_LOOP_SOUNDS 5

//int g_loopingSound[MAX_LOOP_SOUNDS] = {-1};
//int g_staticSound = -1;

//ISoundEngine* g_soundEngine = nullptr;
//ISoundChannel* g_musicChan = nullptr;

ISoundSource* g_loopingSound[MAX_LOOP_SOUNDS];
ISoundSource* g_staticSound;

IEqAudioSource* g_musicChan;

int musicUpdateCb(void* obj, IEqAudioSource::Params& params)
{
	if (params.state != IEqAudioSource::PLAYING)
	{
		params.state = IEqAudioSource::PLAYING;
		return IEqAudioSource::UPDATE_DO_REWIND | IEqAudioSource::UPDATE_STATE;
	}

	return 0;
}

int dummyUpdateCb(void* obj, IEqAudioSource::Params& params)
{
	debugoverlay->Text(color_white, "chan id=%d", params.id);
	return 0;
}

void InitSoundSystem( EQWNDHANDLE wnd )
{
	g_consoleCommands->ClearCommandBuffer();
	g_consoleCommands->ParseFileToCommandBuffer("eqSoundSystemTest.cfg");
	g_consoleCommands->ExecuteCommandBuffer();

	g_audioSystem->Init();

	g_loopingSound[0] = g_audioSystem->LoadSample("sounds/SoundTest/StreamingStereo.wav");
	g_loopingSound[1] = g_audioSystem->LoadSample("sounds/SoundTest/StreamingStereo.ogg");
	g_loopingSound[2] = g_audioSystem->LoadSample("sounds/SoundTest/Sine22.wav");
	g_loopingSound[3] = g_audioSystem->LoadSample("sounds/SoundTest/Sine44.wav");
	g_loopingSound[4] = nullptr;

	g_staticSound = g_audioSystem->LoadSample("sounds/SoundTest/StaticTest.wav");

	g_musicChan = g_audioSystem->CreateSource();
	g_musicChan->Setup(0, g_loopingSound[0], musicUpdateCb, nullptr);

	IEqAudioSource::Params params;
	params.state = IEqAudioSource::PLAYING;
	params.looping = false;
	params.pitch = 4.0;
	params.relative = true;

	g_musicChan->UpdateParams(params, IEqAudioSource::UPDATE_STATE | IEqAudioSource::UPDATE_LOOPING | IEqAudioSource::UPDATE_PITCH | IEqAudioSource::UPDATE_RELATIVE);
}

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

	Event_View_ResetView,

	// combobox
	Event_LoopSounds_Sound,
	Event_LoopSounds_End = Event_LoopSounds_Sound + MAX_LOOP_SOUNDS,

	Event_Max_Menu_Range,

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

Array<shaderfactory_t> pShaderRegistrators{ PP_SL };

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

		matsystem_init_config_t materials_config;
		matsystem_render_config_t& render_config = materials_config.renderConfig;

		render_config.enableBumpmapping = false;
		render_config.enableSpecular = true; // specular for cubemaps
		render_config.enableShadows = false;
		render_config.wireframeMode = false;
		render_config.editormode = false;

		render_config.lightingModel = MATERIAL_LIGHT_FORWARD;
		render_config.threadedloader = true;

		materials_config.shaderApiParams.windowHandle = window;
		materials_config.shaderApiParams.screenFormat = format;

		if (!materials->Init(materials_config))
			exit(0);

		FogInfo_t fog;
		fog.enableFog = true;
		fog.fogColor = ColorRGB(0.25,0.25,0.25);
		fog.fogdensity = 1.0f;
		fog.fogfar = 14250;
		fog.fognear = -2750;

		materials->SetFogInfo(fog);

		g_pShaderAPI = materials->GetShaderAPI();
	}

	materials->LoadShaderLibrary("eqBaseShaders.dll");

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

	m_menu_file->Append( Event_File_Exit, DKLOC("TOKEN_EXIT", "Exit") );

	m_menu_sound = new wxMenu();
	m_pMenu->Append( m_menu_sound, wxT("Sounds") );

	for(int i = 0; i < MAX_LOOP_SOUNDS; i++)
	{
		m_menu_sound->Append(Event_LoopSounds_Sound+i, wxString::Format("Loop sound %d", i), nullptr);
	}

	m_menu_view = new wxMenu();
	m_pMenu->Append( m_menu_view, wxT("View") );

	this->SetMenuBar( m_pMenu );
	this->Centre( wxBOTH );

	Connect(wxEVT_CLOSE_WINDOW, (wxObjectEventFunction)&CMainWindow::OnCloseCmd, nullptr, this);

	InitMatSystem( (EQWNDHANDLE)m_renderPanel->GetHandle() );

	if (!g_parallelJobs->Init(elementsOf(s_jobTypes), s_jobTypes))
		return;

	if (!g_fontCache->Init())
		return;

	debugoverlay->Init(false);

	m_renderPanel->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), nullptr, this);
	m_renderPanel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), nullptr, this);
	m_renderPanel->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CMainWindow::ProcessMouseEvents), nullptr, this);

	m_renderPanel->Connect(wxEVT_MIDDLE_DCLICK, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);
	m_renderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);
	m_renderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);

	m_renderPanel->Connect(wxEVT_RIGHT_DCLICK, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);
	m_renderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);
	m_renderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);


	m_renderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);
	m_renderPanel->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CMainWindow::ProcessMouseEvents, nullptr, this);

	m_renderPanel->Connect(wxEVT_KEY_DOWN, (wxObjectEventFunction)&CMainWindow::ProcessKeyboardDownEvents, nullptr, this);
	m_renderPanel->Connect(wxEVT_KEY_UP, (wxObjectEventFunction)&CMainWindow::ProcessKeyboardUpEvents, nullptr, this);

	m_bIsMoving = false;
	m_bDoRefresh = false;

	InitSoundSystem( (EQWNDHANDLE)m_renderPanel->GetHandle() );

	g_timer.GetTime(true);

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
	else if(event.GetId() >= Event_LoopSounds_Sound && event.GetId() <= Event_LoopSounds_End)
	{
		int soundId = event.GetId()-Event_LoopSounds_Sound;

		/*
		if(g_loopingSound[soundId] != -1)
		{
			g_musicChan->SetAttenuation(0.0f);
			g_musicChan->PlayLoop(g_loopingSound[soundId]);
		}
		else
			g_musicChan->StopSound();
			*/

		if (g_loopingSound[soundId] != nullptr)
		{
			g_musicChan->Release();
			g_musicChan->Setup(0, g_loopingSound[soundId], musicUpdateCb, nullptr);

			IEqAudioSource::Params params;
			params.state = IEqAudioSource::PLAYING;
			params.looping = true;
			params.pitch = RandomFloat(0.5f, 2.0f);
			params.relative = true;

			g_musicChan->UpdateParams(params, IEqAudioSource::UPDATE_STATE | IEqAudioSource::UPDATE_LOOPING | IEqAudioSource::UPDATE_PITCH | IEqAudioSource::UPDATE_RELATIVE);
		}
		else
		{
			g_musicChan->Release();
		}
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
				AngleVectors(cam_angles, &forward, nullptr, nullptr);

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
				AngleVectors(cam_angles, nullptr, &right, &up);

				camera_move_factor *= -1;

				m_bIsMoving = true;
				bAnyMoveButton = true;

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

	cam_pos = clamp(cam_pos, Vector3D(-F_INFINITY), Vector3D(F_INFINITY));

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

	if(event.GetKeyCode() == WXK_SPACE)
	{
		Vector3D randomPos(RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f), RandomFloat(-10.0f, 10.0f));

		//g_soundEngine->PlaySound(g_staticSound, randomPos, 1.0f, 10.0f);

		IEqAudioSource* newSource = g_audioSystem->CreateSource();
		newSource->Setup(1, g_staticSound, dummyUpdateCb, nullptr);

		IEqAudioSource::Params params;
		params.position = randomPos;
		params.state = IEqAudioSource::PLAYING;
		params.releaseOnStop = true;

		newSource->UpdateParams(params, IEqAudioSource::UPDATE_POSITION | IEqAudioSource::UPDATE_STATE | IEqAudioSource::UPDATE_RELEASE_ON_STOP);
		
		debugoverlay->Box3D(randomPos-1.0f, randomPos+1.0f, ColorRGBA(1,1,0,1), 1.0f);
	}
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
#define MAX_FPS         1000.0      // Upper limit for maxfps.

#define MAX_FRAMETIME	0.3
#define MIN_FRAMETIME	0.00001

void CMainWindow::ReDraw()
{
	if(!materials)
		return;

	//	g_sounds->Update();

	if(!IsShown())
		return;

	int w, h;
	m_renderPanel->GetSize(&w, &h);

	if(m_bDoRefresh)
	{


		materials->SetDeviceBackbufferSize(w,h);

		m_bDoRefresh = false;
	}

	// compute time since last frame
	g_frametime += g_timer.GetTime(true);

	// make 120 FPS
	if (g_frametime < 1.0f / 120.0f)
		return;
 
	g_pShaderAPI->SetViewport(0, 0, w,h);

	if(materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.2,0.2,0.2, 1));

		Vector3D forward, right, up;
		AngleVectors(g_camera_rotation, &forward, &right, &up);

		g_pCameraParams.SetAngles(g_camera_rotation);
		g_pCameraParams.SetOrigin(g_camera_target);

		//g_soundEngine->SetListener(g_camera_target, forward,right,up);
		g_audioSystem->SetListener(g_camera_target, vec3_zero, forward, up);
		g_audioSystem->Update();

		debugoverlay->Box3D(-1.0f, 1.0f, ColorRGBA(1,1,1,1), 1.0f);

		ShowFPS();
		debugoverlay->Text(color_white, "Camera position: %g %g %g\n", g_camera_target.x,g_camera_target.y,g_camera_target.z);

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
		materials->EndFrame(nullptr);
		Platform_Sleep(1);
	}

	g_frametime = 0.0f;
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

	g_audioSystem->Shutdown();

	Destroy();

	// shutdown material system
	materials->Shutdown();

	materials = nullptr;

	g_fileSystem->FreeModule(g_matsysmodule);

	// shutdown core
	GetCore()->Shutdown();
}

void CMainWindow::OnButtons(wxCommandEvent& event)
{
	// TODO: selected slots


}


bool InitCore(const char *pCmdLine)
{
	// initialize core
	GetCore()->Init( APPLICATION_NAME, pCmdLine );

	if(!g_fileSystem->Init(false))
		return false;

	return true;
}

IMPLEMENT_APP(CWXTemplateApplication)

CMainWindow *g_pMainFrame = nullptr;

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

	g_pMainFrame = new CMainWindow(nullptr, -1, LocalizedString( TITLE_TOKEN ));
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
