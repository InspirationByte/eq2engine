//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

#include "utils/KeyValues.h"

#include "soundsystem_test.h"

#include "font/IFontCache.h"
#include "math/Utility.h"
#include "math/Random.h"

#include "materialsystem1/IMaterialSystem.h"
#include "render/ViewParams.h"
#include "render/IDebugOverlay.h"

#include "audio/IEqAudioSystem.h"
#include "audio/eqSoundEmitterSystem.h"
#include "audio/eqSoundEmitterObject.h"

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
IShaderAPI*			g_renderAPI = nullptr;
IMaterialSystem*	g_matSystem = nullptr;

CViewParams			g_pCameraParams(Vector3D(0,0,-100), vec3_zero, 70.0f);
Matrix4x4			g_mProjMat, g_mViewMat;
Volume				g_viewFrustum;

Vector3D			g_camera_rotation(25,225,0);
Vector3D			g_camera_target(-4,2.65,4);
float				g_fCamSpeed = 10.0;

CEqTimer			g_timer;
float				g_frametime = 0.0f;

#define SOUND_COUNT_TEST 4
#define MAX_LOOP_SOUNDS 8

enum ESoundChannelType
{
	CHAN_STATIC = 0,
	CHAN_BODY,
	CHAN_ENGINE,
	CHAN_VOICE,
	CHAN_STREAM,

	CHAN_COUNT
};

static ChannelDef s_soundChannels[] = {
	DEFINE_SOUND_CHANNEL(CHAN_STATIC, 16),	// anything that dont fit categories below
	DEFINE_SOUND_CHANNEL(CHAN_BODY, 16),	// hit sounds
	DEFINE_SOUND_CHANNEL(CHAN_ENGINE, 3),	// rev, non-rev and idle sounds
	DEFINE_SOUND_CHANNEL(CHAN_VOICE, 1),
	DEFINE_SOUND_CHANNEL(CHAN_STREAM, 1)
};
static_assert(elementsOf(s_soundChannels) == CHAN_COUNT, "ESoundChannelType needs to be in sync with s_soundChannels");

static const char* s_loopingSoundNames[] = {
	"test.streaming_stereo_wav",
	"test.streaming_stereo_ogg",
	"test.sine_22",
	"test.sine_44",
	"test.cuetest",
	"test.multiSample",
	"test.multiSample2",
	"test.sample",
};

static constexpr const int s_musicNameId = StringToHashConst("music");
static CSoundingObject* g_musicObject = nullptr;
static CSoundingObject* g_testSoundObject = nullptr;

void InitSoundSystem( EQWNDHANDLE wnd )
{
	g_consoleCommands->ClearCommandBuffer();
	g_consoleCommands->ParseFileToCommandBuffer("eqSoundSystemTest.cfg");
	g_consoleCommands->ExecuteCommandBuffer();

	g_audioSystem->Init();
	g_sounds->Init(120.0f, s_soundChannels, elementsOf(s_soundChannels));
	
	{
		KVSection soundSec;
		soundSec.SetName("test.streaming_stereo_wav");
		soundSec.SetKey("wave", "SoundTest/StreamingStereo.wav");
		soundSec.SetKey("is2d", true);
		soundSec.SetKey("loop", true);
		soundSec.SetKey("channel", "CHAN_STREAM");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.streaming_stereo_ogg");
		soundSec.SetKey("wave", "SoundTest/StreamingStereo.ogg");
		soundSec.SetKey("is2d", true);
		soundSec.SetKey("loop", true);
		soundSec.SetKey("channel", "CHAN_STREAM");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.sine_22");
		soundSec.SetKey("wave", "SoundTest/Sine22.wav");
		soundSec.SetKey("is2d", true);
		soundSec.SetKey("loop", true);
		soundSec.SetKey("channel", "CHAN_STREAM");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.sine_44");
		soundSec.SetKey("is2d", true);
		soundSec.SetKey("loop", true);
		soundSec.SetKey("wave", "SoundTest/Sine44.wav");
		soundSec.SetKey("channel", "CHAN_STREAM");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.multiSample");
		KVSection& wave = *soundSec.CreateSection("wave");
		wave.AddKey("wave", "cars/skid.wav");
		wave.AddKey("wave", "cars/skid3.wav");
		wave.AddKey("wave", "cars/skid_traction.wav");

		soundSec.SetKey("loop", true);
		soundSec.SetKey("channel", "CHAN_BODY");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.multiSample2");
		KVSection& wave = *soundSec.CreateSection("wave");
		wave.AddKey("wave", "engines/engine3_high.wav");
		wave.AddKey("wave", "engines/engine2_high.wav");
		wave.AddKey("wave", "engines/engine3_low.wav");
		

		soundSec.SetKey("loop", true);
		soundSec.SetKey("channel", "CHAN_BODY");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.cuetest");
		soundSec.SetKey("is2d", true);
		soundSec.SetKey("loop", false);
		soundSec.SetKey("wave", "SoundTest/CueTest.wav");
		soundSec.SetKey("channel", "CHAN_VOICE");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}
	
	{
		KVSection soundSec;
		soundSec.SetName("test.sample");
		soundSec.SetKey("is2d", false);
		soundSec.SetKey("loop", true);
		soundSec.SetKey("wave", "cars/cone.wav");
		soundSec.SetKey("channel", "CHAN_VOICE");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	{
		KVSection soundSec;
		soundSec.SetName("test.static");
		soundSec.SetKey("wave", "SoundTest/StaticTest.wav");
		soundSec.SetKey("channel", "CHAN_STATIC");
		g_sounds->CreateSoundScript(&soundSec);
		g_sounds->PrecacheSound(soundSec.GetName());
	}

	g_musicObject = PPNew CSoundingObject();
	g_testSoundObject = PPNew CSoundingObject();

	EmitParams ep(s_loopingSoundNames[0]);
	g_musicObject->EmitSound(s_musicNameId, &ep);
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

Array<ShaderFactory> pShaderRegistrators(PP_SL);

void InitMatSystem(EQWNDHANDLE window)
{
	g_matSystem = g_eqCore->GetInterface<IMaterialSystem>();

	if(!g_matSystem)
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

		MaterialsInitSettings materials_config;
		MatSysRenderSettings& render_config = materials_config.renderConfig;
		render_config.enableShadows = false;
		render_config.wireframeMode = false;
		render_config.editormode = false;
		render_config.threadedloader = true;
		materials_config.shaderApiParams.screenFormat = format;

		static void* s_engineWindow = window;

		RenderWindowInfo& winInfo = materials_config.shaderApiParams.windowInfo;

		winInfo.windowType = RHI_WINDOW_HANDLE_NATIVE_WINDOWS;
		winInfo.get = [](void*, RenderWindowInfo::Attribute attrib) -> void* {
			switch (attrib)
			{
				case RenderWindowInfo::WINDOW:
					return s_engineWindow;
				case RenderWindowInfo::DISPLAY:
					return GetDC((HWND)s_engineWindow);
			}
			return nullptr;
		};

		if (!g_matSystem->Init(materials_config))
			exit(0);

		FogInfo fog;
		fog.enableFog = true;
		fog.fogColor = ColorRGB(0.25,0.25,0.25);
		fog.fogdensity = 1.0f;
		fog.fogfar = 14250;
		fog.fognear = -2750;

		g_matSystem->SetFogInfo(fog);

		g_renderAPI = g_matSystem->GetShaderAPI();
	}

	g_matSystem->LoadShaderLibrary("eqBaseShaders.dll");

	// register all shaders
	for(int i = 0; i < pShaderRegistrators.numElem(); i++)
		g_matSystem->RegisterShader( pShaderRegistrators[i]);
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
		m_menu_sound->Append(Event_LoopSounds_Sound+i, s_loopingSoundNames[i], nullptr);
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

		EmitParams ep(s_loopingSoundNames[soundId]);
		g_musicObject->EmitSound(s_musicNameId, &ep);

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

		EmitParams ep("test.static");
		ep.origin = randomPos;
		g_testSoundObject->EmitSound(CSoundingObject::ID_RANDOM, &ep);

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
	if(g_matSystem)
	{
		m_bDoRefresh = true;
	}
}

void CMainWindow::OnSize(wxSizeEvent& event)
{
	wxFrame::OnSize( event );

	m_bDoRefresh = true;
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
	if(!g_matSystem)
		return;

	if(!IsShown())
		return;

	int w, h;
	m_renderPanel->GetSize(&w, &h);

	if(m_bDoRefresh)
	{
		g_matSystem->SetDeviceBackbufferSize(w,h);
		m_bDoRefresh = false;
	}

	// compute time since last frame
	g_frametime += g_timer.GetTime(true);

	// make 120 FPS
	if (g_frametime < 1.0f / 120.0f)
		return;

	static float totalTime = 0.0f;
	totalTime += g_frametime;
	// g_musicObject->SetVolume(CSoundingObject::EncodeId(s_musicNameId, 0), 0.5f);
	// g_audioSystem->SetChannelVolume(CHAN_STREAM, 0.25f);

	g_musicObject->SetSampleVolume(s_musicNameId, 0, 1.0f - length(g_vLastMousePosition) / 512.0f);
	g_musicObject->SetSampleVolume(s_musicNameId, 1, g_vLastMousePosition.x / 512.0f);
	g_musicObject->SetSampleVolume(s_musicNameId, 2, g_vLastMousePosition.y / 512.0f);
 
	if(g_matSystem->BeginFrame(nullptr))
	{
		Vector3D forward, right, up;
		AngleVectors(g_camera_rotation, &forward, &right, &up);

		g_pCameraParams.SetAngles(g_camera_rotation);
		g_pCameraParams.SetOrigin(g_camera_target);

		//g_soundEngine->SetListener(g_camera_target, forward,right,up);
		g_audioSystem->SetListener(g_camera_target, vec3_zero, forward, up);
		g_sounds->Update();

		debugoverlay->Box3D(-1.0f, 1.0f, ColorRGBA(1,1,1,1), 1.0f);

		ShowFPS();
		debugoverlay->Text(color_white, "Camera position: %g %g %g\n", g_camera_target.x,g_camera_target.y,g_camera_target.z);

		FogInfo fog;
		g_matSystem->GetFogInfo(fog);

		fog.viewPos = g_pCameraParams.GetOrigin();

		g_matSystem->SetFogInfo(fog);

		// setup perspective
		g_mProjMat = perspectiveMatrixY(DEG2RAD(g_pCameraParams.GetFOV()), w, h, 1, 5000);

		g_mViewMat = rotateZXY4(DEG2RAD(-g_pCameraParams.GetAngles().x),DEG2RAD(-g_pCameraParams.GetAngles().y),DEG2RAD(-g_pCameraParams.GetAngles().z));
		g_mViewMat.translate(-g_pCameraParams.GetOrigin());

		g_matSystem->SetMatrix(MATRIXMODE_PROJECTION, g_mProjMat);
		g_matSystem->SetMatrix(MATRIXMODE_VIEW, g_mViewMat);

		g_matSystem->SetMatrix(MATRIXMODE_WORLD, identity4);

		int nRenderFlags = 0;

		// draw axe
		debugoverlay->Line3D(vec3_zero, Vector3D(1,0,0), ColorRGBA(0,0,0,1), ColorRGBA(1,0,0,1));
		debugoverlay->Line3D(vec3_zero, Vector3D(0,1,0), ColorRGBA(0,0,0,1), ColorRGBA(0,1,0,1));
		debugoverlay->Line3D(vec3_zero, Vector3D(0,0,1), ColorRGBA(0,0,0,1), ColorRGBA(0,0,1,1));

		debugoverlay->Draw(g_mProjMat, g_mViewMat, w,h);

        // TODO: swap chain
		g_matSystem->EndFrame();
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
	g_matSystem->Shutdown();

	g_matSystem = nullptr;

	g_fileSystem->CloseModule(g_matsysmodule);

	// shutdown core
	g_eqCore->Shutdown();
}

void CMainWindow::OnButtons(wxCommandEvent& event)
{
	// TODO: selected slots


}


bool InitCore(const char *pCmdLine)
{
	// initialize core
	g_eqCore->Init( APPLICATION_NAME, pCmdLine );

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
	EqString loadErr;
	g_matsysmodule = g_fileSystem->OpenModule("eqMatSystem", &loadErr);

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load eqMatSystem - %s", loadErr.ToCString());
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
