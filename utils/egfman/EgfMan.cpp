//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Geometry viewer - new with physics support
//
// TODO:	- Standard EGF viewing
//			- Physics model listing with properties
//			- Physics model testing
//			- Full animation support, may be with ik (BaseAnimating.cpp, for game make entity with BaseAnimatingEntity.cpp)
//			- physics model properties saving
//			- sequence properties saving (speed, smoothing, etc.)
//
//////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commdlg.h>

#include "EditorHeader.h"
#include "FontCache.h"


#include "DebugOverlay.h"
#include "CAnimatedModel.h"
#include "math/math_util.h"
#include "Physics/DkBulletPhysics.h"

ConVar cheats("__cheats", "1");

static DkPhysics s_physics;
IPhysics* physics = &s_physics;

static CDebugOverlay g_DebugOverlays;
IDebugOverlay *debugoverlay = ( IDebugOverlay * )&g_DebugOverlays;

DKMODULE*			g_matsysmodule = NULL;
IShaderAPI*			g_pShaderAPI = NULL;
IMaterialSystem*	materials = NULL;

CViewParams			g_pCameraParams(Vector3D(0,0,-100), vec3_zero, 70);
Matrix4x4			g_mProjMat, g_mViewMat;

sceneinfo_t			scinfo;
CAnimatedModel*		g_pModel = new CAnimatedModel();

float				g_fRealtime = 0.0f;
float				g_fOldrealtime = 0.0f;
float				g_fFrametime = 0.0f;

Vector3D			g_camera_rotation(25,225,0);
Vector3D			g_camera_target(0);
float				g_fCamDistance = 100.0;

void SetOptimalCameraDistance()
{
	if(g_pModel && g_pModel->m_pModel)
	{
		BoundingBox;

		Vector3D bsize = g_pModel->m_pModel->GetAABB().GetSize();

		g_fCamDistance = length(bsize)*2.0f;
	}
}

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

// GUI events
enum
{
	// FILE

	Event_File_OpenModel = 100,
	Event_File_CompileModel,
	Event_File_Save,

	Event_View_ResetView,
	Event_View_ShowPhysModel,

	Event_Max_Menu_Range,

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

class CEGFViewFrame : public wxFrame 
{
public:

	CEGFViewFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("EGFman"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 915,697 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
	void			ReDraw();
	void			OnPaint(wxPaintEvent& event) {}
	void			OnEraseBackground(wxEraseEvent& event) {}
	void			OnSize(wxSizeEvent& event);

	void			OnIdle(wxIdleEvent &event);

	void			ProcessMouseEnter(wxMouseEvent& event);
	void			ProcessMouseLeave(wxMouseEvent& event);
	void			ProcessMouseEvents(wxMouseEvent& event);

	void			ProcessAllMenuCommands(wxCommandEvent& event);

	void			OnComboboxChanged(wxCommandEvent& event);
	void			OnButtons(wxCommandEvent& event);

	void			RefreshGUI();

	//void			ProcessKeyboardDownEvents(wxKeyEvent& event);
	//void			ProcessKeyboardUpEvents(wxKeyEvent& event);
	//void			OnContextMenu(wxContextMenuEvent& event);
	//void			OnFocus(wxFocusEvent& event);
	
protected:
	wxPanel*		m_pRenderPanel;
	wxNotebook*		m_notebook1;
	wxPanel*		m_pModelPanel;
	wxPanel*		m_pMotionPanel;

	wxComboBox*		m_pMotionSelection;
	wxTextCtrl*		m_pAnimFramerate;
	wxSlider*		m_pTimeline;
	wxRadioBox*		m_pAnimMode;
	wxComboBox*		m_pPoseController;
	wxSlider*		m_pPoseValue;

	wxPanel*		m_pPhysicsPanel;
	wxTextCtrl*		m_pObjMass;
	wxTextCtrl*		m_pObjSurfProps;
	wxComboBox*		m_pPhysJoint;
	wxTextCtrl*		m_pLimMinX;
	wxTextCtrl*		m_pLimMaxX;
	wxTextCtrl*		m_pLimMinY;
	wxTextCtrl*		m_pLimMaxY;
	wxTextCtrl*		m_pLimMinZ;
	wxTextCtrl*		m_pLimMaxZ;
	wxTextCtrl*		m_pSimTimescale;
	wxPanel*		m_pIKPanel;
	wxComboBox*		m_pIkLinkSel;

	wxMenuBar*		m_pMenu;
	wxMenuItem*		m_pDrawPhysModel;

	bool			m_bDoRefresh;
	bool			m_bIsMoving;

	wxPoint			m_vLastCursorPos;
	wxPoint			m_vLastClientCursorPos;

	DECLARE_EVENT_TABLE();
};

BEGIN_EVENT_TABLE(CEGFViewFrame, wxFrame)
	EVT_SIZE(OnSize)
	EVT_ERASE_BACKGROUND(OnEraseBackground)
	EVT_IDLE(OnIdle)
	EVT_PAINT(OnPaint)
	EVT_COMBOBOX(-1, OnComboboxChanged)
	EVT_BUTTON(-1, OnButtons)
	EVT_SLIDER(-1, OnButtons)

	EVT_MENU_RANGE(Event_File_OpenModel, Event_Max_Menu_Range, ProcessAllMenuCommands)

	EVT_SIZE(OnSize)

	/*
	EVT_KEY_DOWN(ProcessKeyboardDownEvents)
	EVT_KEY_UP(ProcessKeyboardUpEvents)
	EVT_CONTEXT_MENU(OnContextMenu)
	EVT_SET_FOCUS(OnFocus)
	*/
END_EVENT_TABLE()

DECLARE_INTERNAL_SHADERS();

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

		materials_config.ffp_mode = false;
		materials_config.lighting_model = MATERIAL_LIGHT_FORWARD;
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

	g_pModelCache->PrecacheModel("models/error.egf");

	g_fontCache->Init();

	// register all shaders
	REGISTER_INTERNAL_SHADERS();

	//viewrenderer->InitializeResources();
}

void InitPhysicsScene();

CEGFViewFrame::CEGFViewFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
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
	bSizer1->Add( m_pRenderPanel, 1, wxEXPAND, 5 );
	
	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxSize( -1,180 ), wxNB_FIXEDWIDTH );
	m_pModelPanel = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_notebook1->AddPage( m_pModelPanel, wxT("Model"), false );
	m_pMotionPanel = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Anim/Act:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pMotionSelection = new wxComboBox( m_pMotionPanel, Event_Sequence_Changed, wxT("select animation..."), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( m_pMotionSelection, 0, wxALL, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("FPS:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pAnimFramerate = new wxTextCtrl( m_pMotionPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 35,-1 ), 0 );
	fgSizer1->Add( m_pAnimFramerate, 0, wxALL, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, Event_Timeline_Set, wxT("Timeline:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pTimeline = new wxSlider( m_pMotionPanel, wxID_ANY, 0, 0, 25, wxDefaultPosition, wxSize( 200,42 ), wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	fgSizer1->Add( m_pTimeline, 1, wxALL, 5 );
	
	fgSizer1->Add( new wxButton( m_pMotionPanel, Event_Sequence_Play, wxT("Play"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT ), 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	fgSizer1->Add( new wxButton( m_pMotionPanel, Event_Sequence_Stop, wxT("Stop"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT ), 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Pose controller"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );

	m_pPoseController = new wxComboBox( m_pMotionPanel, Event_PoseCont_Changed, wxT("select controller..."), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( m_pPoseController, 0, wxALL, 5 );
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Control"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pPoseValue = new wxSlider( m_pMotionPanel, Event_PoseCont_ValueChanged, 50, 0, 100, wxDefaultPosition, wxSize( 200,42 ), wxSL_HORIZONTAL );
	fgSizer1->Add( m_pPoseValue, 0, wxALL, 5 );
	
	bSizer3->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	wxString m_pAnimModeChoices[] = { wxT("Sequence Activity"), wxT("Animation") };
	int m_pAnimModeNChoices = sizeof( m_pAnimModeChoices ) / sizeof( wxString );
	m_pAnimMode = new wxRadioBox( m_pMotionPanel, wxID_ANY, wxT("Mode"), wxDefaultPosition, wxDefaultSize, m_pAnimModeNChoices, m_pAnimModeChoices, 1, wxRA_SPECIFY_COLS );
	m_pAnimMode->SetSelection( 0 );
	bSizer3->Add( m_pAnimMode, 0, wxALL, 5 );

	
	//wxStaticBoxSizer* sbSizer5;
	//sbSizer5 = new wxStaticBoxSizer( new wxStaticBox( m_pMotionPanel, wxID_ANY, wxT("Sequence") ), wxVERTICAL );
	
	
	//bSizer3->Add( sbSizer5, 1, wxEXPAND, 5 );
	
	
	m_pMotionPanel->SetSizer( bSizer3 );
	m_pMotionPanel->Layout();
	bSizer3->Fit( m_pMotionPanel );
	m_notebook1->AddPage( m_pMotionPanel, wxT("Motion"), false );
	m_pPhysicsPanel = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer31->Add( new wxStaticLine( m_pPhysicsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL ), 0, wxEXPAND | wxALL, 5 );
	
	wxStaticBoxSizer* sbSizer3;
	sbSizer3 = new wxStaticBoxSizer( new wxStaticBox( m_pPhysicsPanel, wxID_ANY, wxT("Object") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer8->SetFlexibleDirection( wxBOTH );
	fgSizer8->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer8->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("Mass"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pObjMass = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_pObjMass, 0, wxALL, 5 );
	
	fgSizer8->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("Surfaceprops"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pObjSurfProps = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer8->Add( m_pObjSurfProps, 0, wxALL, 5 );
	
	
	sbSizer3->Add( fgSizer8, 1, wxEXPAND, 5 );
	
	
	bSizer31->Add( sbSizer3, 0, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_pPhysicsPanel, wxID_ANY, wxT("Joint limits") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer5;
	fgSizer5 = new wxFlexGridSizer( 0, 3, 0, 0 );
	fgSizer5->SetFlexibleDirection( wxBOTH );
	fgSizer5->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer5->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("Joint"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pPhysJoint = new wxComboBox( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer5->Add( m_pPhysJoint, 0, wxALL, 5 );
	
	
	fgSizer5->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer5->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("X"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pLimMinX = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_pLimMinX, 0, wxALL, 5 );
	
	m_pLimMaxX = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_pLimMaxX, 0, wxALL, 5 );
	
	fgSizer5->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("Y"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pLimMinY = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_pLimMinY, 0, wxALL, 5 );
	
	m_pLimMaxY = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_pLimMaxY, 0, wxALL, 5 );
	
	fgSizer5->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("Z"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pLimMinZ = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_pLimMinZ, 0, wxALL, 5 );
	
	m_pLimMaxZ = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer5->Add( m_pLimMaxZ, 0, wxALL, 5 );
	
	
	sbSizer1->Add( fgSizer5, 1, wxEXPAND, 5 );
	
	
	bSizer31->Add( sbSizer1, 0, wxEXPAND|wxLEFT, 5 );
	
	wxStaticBoxSizer* sbSizer4;
	sbSizer4 = new wxStaticBoxSizer( new wxStaticBox( m_pPhysicsPanel, wxID_ANY, wxT("Simulation") ), wxVERTICAL );
	
	wxFlexGridSizer* fgSizer9;
	fgSizer9 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer9->SetFlexibleDirection( wxBOTH );
	fgSizer9->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer9->Add( new wxStaticText( m_pPhysicsPanel, wxID_ANY, wxT("Timescale"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pSimTimescale = new wxTextCtrl( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer9->Add( m_pSimTimescale, 0, wxALL, 5 );
	
	fgSizer9->Add( new wxButton( m_pPhysicsPanel, Event_Physics_SimulateToggle, wxT("Run/Pause"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	fgSizer9->Add( new wxButton( m_pPhysicsPanel, wxID_ANY, wxT("Reset"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	
	sbSizer4->Add( fgSizer9, 1, wxEXPAND, 5 );
	
	
	bSizer31->Add( sbSizer4, 1, wxEXPAND|wxLEFT, 5 );
	
	
	m_pPhysicsPanel->SetSizer( bSizer31 );
	m_pPhysicsPanel->Layout();
	bSizer31->Fit( m_pPhysicsPanel );
	m_notebook1->AddPage( m_pPhysicsPanel, wxT("Physics"), false );
	m_pIKPanel = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer6->SetFlexibleDirection( wxBOTH );
	fgSizer6->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer6->Add( new wxStaticText( m_pIKPanel, wxID_ANY, wxT("Link"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pIkLinkSel = new wxComboBox( m_pIKPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer6->Add( m_pIkLinkSel, 0, wxALL, 5 );
	
	
	m_pIKPanel->SetSizer( fgSizer6 );
	m_pIKPanel->Layout();
	fgSizer6->Fit( m_pIKPanel );
	m_notebook1->AddPage( m_pIKPanel, wxT("Inverse Kinematics"), true );
	
	bSizer1->Add( m_notebook1, 0, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer1 );
	this->Layout();

	m_notebook1->SetSelection( 1 );
	
	m_pMenu = new wxMenuBar( 0 );

	wxMenu *menuFile = new wxMenu;
	
	menuFile->Append( Event_File_OpenModel, DKLOC("TOKEN_OPEN", L"Open\tCtrl+O") );
	menuFile->AppendSeparator();
	menuFile->Append( Event_File_CompileModel, DKLOC("TOKEN_COMPILEMODEL", L"Compile ESC/ASC") );
	menuFile->Append( Event_File_Save, DKLOC("TOKEN_SAVE", L"Save") );

	wxMenu *menuView = new wxMenu;
	
	menuView->Append( Event_View_ResetView, DKLOC("TOKEN_RESETVIEW", L"Reset View\tR") );

	m_pDrawPhysModel = menuView->Append( Event_View_ShowPhysModel, DKLOC("TOKEN_SHOWPHYSICSMODEL", L"Show physics model"), wxEmptyString, wxITEM_CHECK );

	m_pMenu->Append( menuFile, DKLOC("TOKEN_FILE", L"File") );
	m_pMenu->Append( menuView, DKLOC("TOKEN_VIEW", L"View") );

	this->SetMenuBar( m_pMenu );
	
	this->Centre( wxBOTH );

	// create physics scene and add infinite plane
	InitPhysicsScene();

	InitMatSystem(m_pRenderPanel->GetHWND());

	debugoverlay->Init();

	m_pRenderPanel->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_DCLICK, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_DCLICK, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DCLICK, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);

	m_pRenderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, NULL, this);

	m_pRenderPanel->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEnter, NULL, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseLeave, NULL, this);

	m_pRenderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEnter, NULL, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseLeave, NULL, this);

	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEnter, NULL, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseLeave, NULL, this);

	m_bIsMoving = false;
	m_bDoRefresh = true;

	RefreshGUI();
}

#define PHYS_PLANE_SIZE 512

#define PLANE_W 32
#define PLANE_H 32

#define PLANE_VERTCOUNT (PLANE_W*PLANE_H)
#define PLANE_IDXCOUNT ((PLANE_W-1)*(PLANE_H-1)*6)

Vector3D	g_physPlaneVertices[PLANE_W*PLANE_H];
uint		g_physPlaneIndices[PLANE_IDXCOUNT];

void InitPhysicsScene()
{
	physics->Init( MAX_COORD_UNITS );

	// create physics scene and add infinite plane
	physics->CreateScene();
	/*
	pritimiveinfo_t prim;
	prim.primType = PHYSPRIM_BOX;
	prim.boxInfo.boxSizeX = PHYS_PLANE_SIZE;
	prim.boxInfo.boxSizeY = 50;
	prim.boxInfo.boxSizeZ = PHYS_PLANE_SIZE;

	physobject_t obj;
	obj.body_part = 0;
	obj.mass = 0;
	obj.mass_center = vec3_zero;
	obj.numShapes = 1;
	obj.shape_indexes[0] = physics->AddPrimitiveShape( prim );
	strcpy(obj.surfaceprops, "default");

	IPhysicsObject* pObj = physics->CreateObject( &obj );
	pObj->SetPosition( Vector3D(0,-25, 0));
	*/
	
	physmodelcreateinfo_t info;
	SetDefaultPhysModelInfoParams(&info);

	dkCollideData_t collData;

	collData.pMaterial = NULL; // null groups means default material
	collData.surfaceprops = "default";

	// create big plane

	// compute vertices
	for(int c = 0; c < PLANE_H; c++)
	{
		for(int r = 0; r < PLANE_W; r++)
		{
			int vertex_id = r + c*PLANE_W;

			float tc_x = (float)(r)/(float)(PLANE_W-1);
			float tc_y = (float)(c)/(float)(PLANE_H-1);

			float v_pos_x = (tc_x-0.5f)*2.0f*PHYS_PLANE_SIZE;
			float v_pos_y = (tc_y-0.5f)*2.0f*PHYS_PLANE_SIZE;

			g_physPlaneVertices[vertex_id] = Vector3D(v_pos_x,0,v_pos_y);
		}
	}

	// compute indices
	// support edge turning - this creates more smoothed terrain, but needs more polygons
	bool bTurnEdge = false;
	int nTriangle = 0;
	for(int c = 0; c < (PLANE_H-1); c++)
	{
		for(int r = 0; r < (PLANE_W-1); r++)
		{
			int index1 = r + c*PLANE_W;
			int index2 = r + (c+1)*PLANE_W;
			int index3 = (r+1) + c*PLANE_W;
			int index4 = (r+1) + (c+1)*PLANE_W;

			if(!bTurnEdge)
			{
				g_physPlaneIndices[nTriangle*3] = index1;
				g_physPlaneIndices[nTriangle*3+1] = index2;
				g_physPlaneIndices[nTriangle*3+2] = index3;

				nTriangle++;

				g_physPlaneIndices[nTriangle*3] = index3;
				g_physPlaneIndices[nTriangle*3+1] = index2;
				g_physPlaneIndices[nTriangle*3+2] = index4;

				nTriangle++;
			}
			else
			{
				g_physPlaneIndices[nTriangle*3] = index1;
				g_physPlaneIndices[nTriangle*3+1] = index2;
				g_physPlaneIndices[nTriangle*3+2] = index4;

				nTriangle++;

				g_physPlaneIndices[nTriangle*3] = index1;
				g_physPlaneIndices[nTriangle*3+1] = index4;
				g_physPlaneIndices[nTriangle*3+2] = index3;

				nTriangle++;
			}

			bTurnEdge = !bTurnEdge;
		}
		//bTurnEdge = !bTurnEdge;
	}
	
	collData.indices = g_physPlaneIndices;
	collData.numIndices = PLANE_IDXCOUNT;
	collData.vertices = g_physPlaneVertices;
	collData.vertexPosOffset = 0;
	collData.vertexSize = sizeof(Vector3D);
	collData.numVertices = PLANE_VERTCOUNT;

	info.isStatic = true;
	info.flipXAxis = false;
	info.mass = 0.0f;
	info.data = &collData;

	IPhysicsObject* pObj = physics->CreateStaticObject(&info, COLLISION_GROUP_WORLD);
}

void CEGFViewFrame::OnIdle(wxIdleEvent &event)
{
	event.RequestMore(true);

	ReDraw();
}

void CEGFViewFrame::ProcessMouseEnter(wxMouseEvent& event)
{
	//CaptureMouse();
}

void CEGFViewFrame::ProcessMouseLeave(wxMouseEvent& event)
{
	//ReleaseMouse();
}

void CEGFViewFrame::ProcessAllMenuCommands(wxCommandEvent& event)
{
	if(event.GetId() == Event_File_OpenModel)
	{
		wxFileDialog* file = new wxFileDialog(NULL, "Open EGF model", 
													varargs("%s/models", g_fileSystem->GetCurrentGameDirectory()), 
													"*.egf", 
													"Equilibrium Geometry File (*.egf)|*.egf;", 
													wxFD_FILE_MUST_EXIST | wxFD_OPEN);

		if(file && file->ShowModal() == wxID_OK)
		{
			EqString fname(file->GetPath().wchar_str());

			FlushCache();
			g_pModel->SetModel( NULL );

			int cache_index = g_pModelCache->PrecacheModel( fname.GetData() );
			if(cache_index == 0)
			{
				ErrorMsg("Can't open %s\n", fname.GetData());
			}

			g_pModel->SetModel( g_pModelCache->GetModel(cache_index) );
		}

		SetOptimalCameraDistance();
		RefreshGUI();

		delete file;
	}
	else if(event.GetId() == Event_File_CompileModel)
	{
		wxFileDialog* file = new wxFileDialog(NULL, "Select script files to compile", 
													wxEmptyString, 
													"*.esc;*.asc", 
													"Script (*.esc;*.asc)|*.esc;*.asc", 
													wxFD_FILE_MUST_EXIST | wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

		if(file && file->ShowModal() == wxID_OK)
		{
			wxArrayString paths;
			file->GetPaths(paths);

			for(size_t i = 0; i < paths.size(); i++)
			{
				EqString model_aftercomp_name;

				KeyValues script;

				EqString fname( paths[i].wchar_str() );
				EqString ext = fname.Path_Extract_Ext();

				if(!stricmp(ext.GetData(), "asc"))
				{
					EqString cmdLine(varargs("bin32\\animca.exe +filename \"%s\"", fname.GetData()));
					Msg("***Command line: %s\n", cmdLine.GetData());

					system( cmdLine.GetData() );
				}
				else
				{
					if(script.LoadFromFile(fname.GetData()))
					{
						// load all script data
						kvkeybase_t* mainsection = script.GetRootSection();
						if(mainsection)
						{
							kvkeybase_t* pPair = mainsection->FindKeyBase("modelfilename");

							if(pPair)
							{
								model_aftercomp_name = pPair->values[0];
								Msg("***Starting egfca for %s\n", fname.GetData());

								EqString cmdLine(varargs("egfca.exe +filename \"%s\"", fname.GetData()));

								if( g_fileSystem->FileExist("bin32\\egfca.exe") )
								{
									cmdLine = "bin32\\" + cmdLine;
								}

								Msg("***Command line: %s\n", cmdLine.GetData());
								system( cmdLine.GetData() );
							}
							else
							{
								ErrorMsg("ERROR! 'modelfilename' isn't specified in the script!!!\n");
							}
						}
					}

					if(paths.size() == 1)
					{
						FlushCache();
						g_pModel->SetModel( NULL );

						int cache_index = g_pModelCache->PrecacheModel( model_aftercomp_name.GetData() );
						if(cache_index == 0)
						{
							ErrorMsg("Can't open %s\n", model_aftercomp_name.GetData());
						}

						g_pModel->SetModel( g_pModelCache->GetModel(cache_index) );

						SetOptimalCameraDistance();

						materials->PreloadNewMaterials();
					}
				}
			}
		}

		RefreshGUI();

		delete file;
	}
	else if(event.GetId() == Event_File_Save)
	{

	}
	else if(event.GetId() == Event_View_ResetView)
	{
		g_camera_rotation = Vector3D(25,225,0);
		g_camera_target = vec3_zero;
		SetOptimalCameraDistance();
	}
}

Vector2D		g_vLastMousePosition(0);
IPhysicsObject* g_pDragable = NULL;
Vector3D		g_vDragLocalPos = NULL;
Vector3D		g_vOriginalObjectPosition(0);
Vector3D		g_vDragTarget(0);

void CEGFViewFrame::ProcessMouseEvents(wxMouseEvent& event)
{
	Vector3D cam_angles = g_camera_rotation;
	Vector3D cam_pos = g_camera_target;

	float fov = g_pCameraParams.GetFOV();

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
				cam_angles.x += move_delta_y*0.5f;
				cam_angles.y -= move_delta_x*0.5f;

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

				g_fCamDistance += move_delta_y*0.5f;
				//cam_pos -= forward*move_delta_y * camera_move_factor;

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

				cam_pos += right*move_delta_x * camera_move_factor/* * g_frametime * CAM_MOVE_SPEED*/;
				cam_pos -= up*move_delta_y * camera_move_factor/* * g_frametime * CAM_MOVE_SPEED*/;

				g_vLastMousePosition = prev_mouse_pos;
			}
		}
	}
	else
	{
		if(event.ButtonIsDown(wxMOUSE_BTN_LEFT))
		{
			Vector3D ray_start, ray_dir;
			Vector3D last_ray_start, last_ray_dir;

			// set object
			ScreenToDirection(g_pCameraParams.GetOrigin(), Vector2D(w-event.GetX(),event.GetY()), Vector2D(w,h), ray_start, ray_dir, g_mProjMat*g_mViewMat, false);
			ScreenToDirection(g_pCameraParams.GetOrigin(), Vector2D(w-g_vLastMousePosition.x,g_vLastMousePosition.y), Vector2D(w,h), last_ray_start, last_ray_dir, g_mProjMat*g_mViewMat, false);
	
			// we found ray, find object

			
			if(g_pDragable == NULL)
			{
				internaltrace_t tr;
				physics->InternalTraceLine(ray_start, ray_start+ray_dir, COLLISION_GROUP_ALL, &tr);

				debugoverlay->Line3D(ray_start, tr.traceEnd, ColorRGBA(1,0,0,1), ColorRGBA(0,1,0,1), 10);

				if(tr.hitObj)
				{
					g_pDragable = tr.hitObj;
					g_vDragLocalPos = tr.traceEnd - g_pDragable->GetPosition();
					g_vOriginalObjectPosition = g_pDragable->GetPosition();
				}
			}

			if(g_pDragable)
			{
				Vector3D forward = g_mViewMat.rows[2].xyz();

				// setup edit plane
				Plane pl(forward.x,forward.y,forward.z, -dot(forward, g_vOriginalObjectPosition));

				Vector3D intersection;

				pl.GetIntersectionWithRay(ray_start, normalize(ray_dir), intersection);

				g_vDragTarget = intersection;
			}
		}
		else if(!event.ButtonIsDown(wxMOUSE_BTN_LEFT))
			g_pDragable = NULL;
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

void CEGFViewFrame::OnSize(wxSizeEvent& event)
{
	wxFrame::OnSize( event );

	if(materials)
	{
		int w, h;
		m_pRenderPanel->GetSize(&w,&h);

		materials->SetDeviceBackbufferSize(w,h);

		ReDraw();
	}
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

void CEGFViewFrame::ReDraw()
{
	if(!materials)
		return;

	//	ses->Update();

	if(!m_bDoRefresh)
	{
		return;
	}

	if(!IsShown())
		return;

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

	//m_bDoRefresh = false;

	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w,h);

	if(materials->BeginFrame())
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.5,0.5,0.5, 1));

		Vector3D forward, right;
		AngleVectors(g_camera_rotation, &forward, &right);

		g_pCameraParams.SetAngles(g_camera_rotation);
		g_pCameraParams.SetOrigin(g_camera_target + forward*-g_fCamDistance);

		ShowFPS();

		FogInfo_t fog;
		materials->GetFogInfo(fog);

		fog.viewPos = g_pCameraParams.GetOrigin();

		materials->SetFogInfo(fog);

		// setup perspective
		g_mProjMat = perspectiveMatrixY(DEG2RAD(g_pCameraParams.GetFOV()), w, h, 0.25f, 2500.0f);

		g_mViewMat = rotateZXY4(DEG2RAD(-g_pCameraParams.GetAngles().x),DEG2RAD(-g_pCameraParams.GetAngles().y),DEG2RAD(-g_pCameraParams.GetAngles().z));
		g_mViewMat.translate(-g_pCameraParams.GetOrigin());

		materials->SetMatrix(MATRIXMODE_PROJECTION, g_mProjMat);
		materials->SetMatrix(MATRIXMODE_VIEW, g_mViewMat);

		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		// Update things

		if(g_pDragable)
		{
			Vector3D point = g_pDragable->GetTransformMatrix().getRotationComponent()*g_vDragLocalPos;

			Vector3D force = (g_vDragTarget - (point+g_pDragable->GetPosition()))*g_pDragable->GetInvMass()*8000.0f;

			Vector3D torque = cross(point,force);

			g_pDragable->ApplyImpulse(force, point);
			//g_pDragable->AddForce(force);
			//g_pDragable->AddLocalTorqueImpulse(torque);

				//(g_vDragTarget - point)*g_pDragable->GetInvMass()*1000.0f, g_vDragLocalPos
		}

		physics->Simulate( g_frametime, 1 );
		g_pModel->Update( g_frametime );

		g_pShaderAPI->ResetCounters();

		// Now we can draw our model
		g_pModel->Render((int)m_pDrawPhysModel->IsChecked(), g_fCamDistance);

		debugoverlay->Text(color4_white, "polygon count: %d\n", g_pShaderAPI->GetTrianglesCount());

		// draw floor 1x1 meters
		debugoverlay->Polygon3D(Vector3D(-64,0,-64), Vector3D(-64,0,64), Vector3D(64,0,64), ColorRGBA(1,1,0,0.25));
		debugoverlay->Polygon3D(Vector3D(64,0,64), Vector3D(64,0,-64), Vector3D(-64,0,-64), ColorRGBA(1,1,0,0.25));

		debugoverlay->Draw(g_mProjMat, g_mViewMat, w,h);

		materials->EndFrame( NULL );
		Platform_Sleep(1);
	}
}

void CEGFViewFrame::RefreshGUI()
{
	m_pMotionSelection->Clear();
	m_pPoseController->Clear();

	// populate all lists
	if(g_pModel)
	{
		// sequences
		if(m_pAnimMode->GetSelection() == 0)
		{
			for(int i = 0; i < g_pModel->m_pSequences.numElem(); i++)
			{
				m_pMotionSelection->Append( g_pModel->m_pSequences[i].name );
			}
		}
		else // animations
		{
			/*
			FIXME: UNSUPPORTED for now
			for(int i = 0; i < g_pModel->m_pSequences.numElem(); i++)
			{
				m_pMotionSelection->Append( g_pModel->m_pSequences[i].name );
			}*/
		}

		for(int i = 0; i < g_pModel->m_poseControllers.numElem(); i++)
		{
			m_pPoseController->Append( g_pModel->m_poseControllers[i].pDesc->name );
		}
	}
}

void CEGFViewFrame::OnComboboxChanged(wxCommandEvent& event)
{
	// TODO: selected slots

	if(event.GetId() == Event_Sequence_Changed)
	{
		int nSeq = m_pMotionSelection->GetSelection();

		if(g_pModel && nSeq != -1)
		{
			g_pModel->SetSequence( nSeq, 0 );
			g_pModel->ResetAnimationTime(0);
		}
	}
	else if(event.GetId() == Event_PoseCont_Changed)
	{
		int nPoseContr = m_pPoseController->GetSelection();

		if(g_pModel && nPoseContr != -1)
		{
			m_pPoseValue->SetMin( g_pModel->m_poseControllers[nPoseContr].pDesc->blendRange[0]*10 );
			m_pPoseValue->SetMax( g_pModel->m_poseControllers[nPoseContr].pDesc->blendRange[1]*10 );

			m_pPoseValue->SetValue( g_pModel->m_poseControllers[nPoseContr].value*10 );
		}
	}
}

void CEGFViewFrame::OnButtons(wxCommandEvent& event)
{
	// TODO: selected slots

	if(event.GetId() == Event_Sequence_Play)
	{
		if(g_pModel)
		{
			g_pModel->ResetAnimationTime(0);
			g_pModel->PlayAnimation( 0 );
		}
	}
	else if(event.GetId() == Event_Sequence_Stop)
	{
		if(g_pModel)
		{
			g_pModel->StopAnimation( 0 );
		}
	}
	else if(event.GetId() == Event_Timeline_Set)
	{
		if(g_pModel)
		{
			
		}
	}
	else if(event.GetId() == Event_PoseCont_ValueChanged)
	{
		int nPoseContr = m_pPoseController->GetSelection();

		if(g_pModel && nPoseContr != -1)
		{
			g_pModel->m_poseControllers[nPoseContr].value = float(m_pPoseValue->GetValue()) * 0.1;
		}
	}
	else if(event.GetId() == Event_Physics_SimulateToggle)
	{
		if(g_pModel)
		{
			g_pModel->TogglePhysicsState();
		}
	}
}

#undef IMPLEMENT_WXWIN_MAIN

bool InitCore(HINSTANCE hInstance, char *pCmdLine)
{
	// initialize core
	GetCore()->Init("EGFMan", pCmdLine);

	if(!g_fileSystem->Init(false))
		return false;

	g_cmdLine->ExecuteCommandLine( true, true );

	return true;
}

#undef wxIMPLEMENT_WXWIN_MAIN

#define wxIMPLEMENT_WXWIN_MAIN                                              \
    extern "C" int WINAPI WinMain(HINSTANCE hInstance,                      \
                                  HINSTANCE hPrevInstance,                  \
                                  wxCmdLineArgType lpCmdLine,     \
                                  int nCmdShow)                             \
    {                                                                       \
        wxDISABLE_DEBUG_SUPPORT();                                          \
                                                                            \
        /* NB: We pass NULL in place of lpCmdLine to behave the same as  */ \
        /*     Borland-specific wWinMain() above. If it becomes needed   */ \
        /*     to pass lpCmdLine to wxEntry() here, you'll have to fix   */ \
        /*     wWinMain() above too.                                     */ \
		if(!InitCore(hInstance,lpCmdLine)) return -1;						\
        return wxEntry(hInstance, hPrevInstance, NULL, 0);           \
    }                                                                       \
    wxIMPLEMENT_WXWIN_MAIN_BORLAND_NONSTANDARD


IMPLEMENT_APP(CEGFViewApp)

typedef int (*winmain_wx_cb)(HINSTANCE hInstance, HINSTANCE hPrevInstance, char *pCmdLine, int nCmdShow);

winmain_wx_cb g_wxEntryCallback = wxEntry;

CEGFViewFrame *g_pMainFrame = NULL;

bool CEGFViewApp::OnInit()
{
	setlocale(LC_ALL,"C");

	// first, load matsystem module
	g_matsysmodule = g_fileSystem->LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		return -1;
	}

	g_localizer->AddTokensFile("EGFMan");

	g_pMainFrame = new CEGFViewFrame( NULL, -1, DKLOC("TOKEN_TITLE", L"EGFMan"));
	g_pMainFrame->Centre();
	g_pMainFrame->Show(true);

	SetTopWindow(g_pMainFrame);

    return true;
}

int CEGFViewApp::OnExit()
{
	//WriteCfgFile("cfg/editor.cfg");
	
	// shutdown material system
	materials->Shutdown();

	g_fileSystem->FreeModule(g_matsysmodule);
	
	// shutdown core
	GetCore()->Shutdown();

    return 0;
} 