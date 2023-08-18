//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
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

#include <wx/settings.h>

#include "core/core_common.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ILocalize.h"
#include "core/IEqParallelJobs.h"
#include "utils/KeyValues.h"

#include "font/IFontCache.h"
#include "studio/StudioCache.h"
#include "studio/StudioGeom.h"

#include "render/IDebugOverlay.h"
#include "CAnimatedModel.h"

#include "math/Utility.h"

#include "dkphysics/DkBulletPhysics.h"
#include "physics/PhysicsCollisionGroup.h"
#include "physics/BulletShapeCache.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "render/ViewParams.h"

#include "EditorHeader.h"
#include "grid.h"


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

DECLARE_CVAR(__cheats, "1", nullptr, CV_PROTECTED | CV_INVISIBLE);

static DkPhysics s_physics;
IPhysics* physics = &s_physics;

CBulletStudioShapeCache	s_shapeCache;

DKMODULE*			g_matsysmodule = nullptr;
IShaderAPI*			g_pShaderAPI = nullptr;
IMaterialSystem*	materials = nullptr;

CViewParams			g_pCameraParams(Vector3D(0,0,-100), vec3_zero, 70);
Matrix4x4			g_mProjMat, g_mViewMat;

sceneinfo_t			scinfo;
CAnimatedModel		g_model;

Vector3D			g_camera_rotation(25,225,0);
Vector3D			g_camera_target(0);
float				g_fCamDistance = 10.0;

CEqTimer			g_timer;

void SetOptimalCameraDistance()
{
	if(g_model.m_pModel)
	{
		const BoundingBox& bbox = g_model.m_pModel->GetBoundingBox();
		g_fCamDistance = length(bbox.GetSize())*2.0f;
	}
}

void FlushCache()
{
	g_studioModelCache->ReleaseCache();
	g_studioModelCache->PrecacheModel("models/error.egf");
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
	Event_File_ReloadModel,
	Event_File_Save,

	Event_View_ResetView,
	Event_View_ShowPhysModel,
	Event_View_ShowBones,
	Event_View_ShowAttachments,
	Event_View_ShowFloor,
	Event_View_ShowGrid,
	Event_View_Wireframe,
	

	Event_Max_Menu_Range,

	// Model tab
	Event_Model_ToggleBodypart,

	// combobox
	Event_Sequence_Changed,
	Event_PoseCont_Changed,

	// buttons
	Event_Sequence_Play,
	Event_Sequence_Stop,
	Event_Physics_SimulateToggle,
	Event_Physics_Reset,

	// slider
	Event_Timeline_Set,
	Event_PoseCont_ValueChanged,
};

class CEGFViewFrame : public wxFrame 
{
public:

	CEGFViewFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("EGFman"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 915,697 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
	void			ReDraw();
	void			OnPaint(wxPaintEvent& event);
	void			OnEraseBackground(wxEraseEvent& event) {}
	void			OnSize(wxSizeEvent& event);

	void			OnIdle(wxIdleEvent &event);

	void			ProcessMouseEnter(wxMouseEvent& event);
	void			ProcessMouseLeave(wxMouseEvent& event);
	void			ProcessMouseEvents(wxMouseEvent& event);

	void			ProcessAllMenuCommands(wxCommandEvent& event);

	void			OnComboboxChanged(wxCommandEvent& event);
	void			OnButtons(wxCommandEvent& event);
	void			OnBodyGroupToggled( wxCommandEvent& event );

	void			InitializeEq();
	void			RefreshGUI();

	//void			ProcessKeyboardDownEvents(wxKeyEvent& event);
	//void			ProcessKeyboardUpEvents(wxKeyEvent& event);
	//void			OnContextMenu(wxContextMenuEvent& event);
	//void			OnFocus(wxFocusEvent& event);
	
protected:
#ifdef PLAT_LINUX
	wxGLCanvas*		m_pRenderPanel;
#else
	wxPanel*		m_pRenderPanel;
#endif
	wxNotebook*		m_notebook1;
	wxPanel*		m_pModelPanel;
	wxPanel*		m_pMotionPanel;

	// model tab
	wxCheckListBox*	m_enabledBodyParts;
	wxSpinCtrl*		m_lodSpin;
	wxCheckBox*		m_lodOverride;

	// motion tab
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
	wxMenuItem*		m_drawPhysModel;
	wxMenuItem*		m_drawBones;
	wxMenuItem*		m_drawAttachments;
	wxMenuItem*		m_drawFloor;
	wxMenuItem*		m_drawGrid;
	wxMenuItem*		m_wireframe;

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
	EVT_CHECKLISTBOX( -1, OnBodyGroupToggled )

	EVT_MENU_RANGE( Event_File_OpenModel, Event_Max_Menu_Range, ProcessAllMenuCommands)
	
	EVT_SIZE(OnSize)

	/*
	EVT_KEY_DOWN(ProcessKeyboardDownEvents)
	EVT_KEY_UP(ProcessKeyboardUpEvents)
	EVT_CONTEXT_MENU(OnContextMenu)
	EVT_SET_FOCUS(OnFocus)
	*/
END_EVENT_TABLE()

DECLARE_INTERNAL_SHADERS();

#define PHYS_PLANE_SIZE 512

#define PHY_PLANE_W 32
#define PHY_PLANE_H 32

#define PHY_PLANE_VERTCOUNT (PHY_PLANE_W*PHY_PLANE_H)
#define PHY_PLANE_IDXCOUNT ((PHY_PLANE_W-1)*(PHY_PLANE_H-1)*6)

Vector3D	g_physPlaneVertices[PHY_PLANE_W*PHY_PLANE_H];
uint		g_physPlaneIndices[PHY_PLANE_IDXCOUNT];

static void InitPhysicsScene()
{
	physics->Init(F_INFINITY);

	// create physics scene and add infinite plane
	physics->CreateScene();
	
	physmodelcreateinfo_t info;
	SetDefaultPhysModelInfoParams(&info);

	dkCollideData_t collData;

	collData.pMaterial = nullptr; // null groups means default material
	collData.surfaceprops = "default";

	// create big plane

	// compute vertices
	for(int c = 0; c < PHY_PLANE_H; c++)
	{
		for(int r = 0; r < PHY_PLANE_W; r++)
		{
			int vertex_id = r + c* PHY_PLANE_W;

			float tc_x = (float)(r)/(float)(PHY_PLANE_W-1);
			float tc_y = (float)(c)/(float)(PHY_PLANE_H-1);

			float v_pos_x = (tc_x-0.5f)*2.0f*PHYS_PLANE_SIZE;
			float v_pos_y = (tc_y-0.5f)*2.0f*PHYS_PLANE_SIZE;

			g_physPlaneVertices[vertex_id] = Vector3D(v_pos_x,0,v_pos_y);
		}
	}

	// compute indices
	// support edge turning - this creates more smoothed terrain, but needs more polygons
	bool bTurnEdge = false;
	int nTriangle = 0;
	for(int c = 0; c < (PHY_PLANE_H-1); c++)
	{
		for(int r = 0; r < (PHY_PLANE_W-1); r++)
		{
			int index1 = r + c* PHY_PLANE_W;
			int index2 = r + (c+1)*PHY_PLANE_W;
			int index3 = (r+1) + c* PHY_PLANE_W;
			int index4 = (r+1) + (c+1)*PHY_PLANE_W;

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
	collData.numIndices = PHY_PLANE_IDXCOUNT;
	collData.vertices = g_physPlaneVertices;
	collData.vertexPosOffset = 0;
	collData.vertexSize = sizeof(Vector3D);
	collData.numVertices = PHY_PLANE_VERTCOUNT;

	info.isStatic = true;
	info.flipXAxis = false;
	info.mass = 0.0f;
	info.data = &collData;

	IPhysicsObject* pObj = physics->CreateStaticObject(&info, COLLISION_GROUP_WORLD);
}

static void InitMatSystem(void* window)
{
	ASSERT_MSG(window, "InitMatSystem - NULL window");

	materials = g_eqCore->GetInterface<IMaterialSystem>();

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

		materialsInitSettings_t materials_config;
		materialsRenderSettings_t& render_config = materials_config.renderConfig;

		render_config.enableBumpmapping = false;
		render_config.enableSpecular = true; // specular for cubemaps
		render_config.enableShadows = false;
		render_config.wireframeMode = false;
		render_config.editormode = true;

		render_config.lightingModel = MATERIAL_LIGHT_FORWARD;
		materials_config.shaderApiParams.screenFormat = format;

		static void* s_engineWindow = window;

		shaderAPIWindowInfo_t& winInfo = materials_config.shaderApiParams.windowInfo;
		winInfo.windowType = RHI_WINDOW_HANDLE_NATIVE_WINDOWS;
		winInfo.get = [](shaderAPIWindowInfo_t::Attribute attrib) -> void* {
			switch (attrib)
			{
			case shaderAPIWindowInfo_t::WINDOW:
				return s_engineWindow;
			case shaderAPIWindowInfo_t::DISPLAY:
				return GetDC((HWND)s_engineWindow);
			}
			return nullptr;
		};

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

	materials->LoadShaderLibrary("eqBaseShaders");

	if (!g_parallelJobs->Init(elementsOf(s_jobTypes), s_jobTypes))
		return;

	if (!g_fontCache->Init())
	{
		ErrorMsg("Unable to init Font cache!\n");
		return;
	}

	g_studioModelCache->PrecacheModel("models/error.egf");

	// register all shaders
	REGISTER_INTERNAL_SHADERS();

	//viewrenderer->InitializeResources();
}

void CEGFViewFrame::InitializeEq()
{
	// create physics scene and add infinite plane
	InitPhysicsScene();

#ifdef PLAT_LINUX
	// NOTE: we use wxGLCanvas instead
	InitMatSystem(m_pRenderPanel->GetXWindow());
#else
	InitMatSystem(m_pRenderPanel->GetHandle());
#endif

	debugoverlay->Init(false);
}

CEGFViewFrame::CEGFViewFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
#ifdef PLAT_WIN
	wxIcon ico;
	if(ico.LoadFile("IDI_MAINICON"))
		SetIcon(ico);
	else
		MsgError("Can't load icon!\n");
#endif

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	
#ifdef PLAT_LINUX
	// create gl canvas without gl context. We only need it as it would provide us with XWindow
	m_pRenderPanel = new wxGLCanvas( this, wxID_ANY, nullptr, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, "GLCanvas", wxNullPalette );
#else
	m_pRenderPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
#endif
	bSizer1->Add( m_pRenderPanel, 1, wxEXPAND, 5 );
	
	m_notebook1 = new wxNotebook( this, wxID_ANY, wxDefaultPosition, wxSize( -1,180 ), wxNB_FIXEDWIDTH );
	m_pModelPanel = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	bSizer5->Add( new wxStaticText( m_pModelPanel, wxID_ANY, wxT("Body groups"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	wxArrayString m_enabledBodyPartsChoices;
	m_enabledBodyParts = new wxCheckListBox( m_pModelPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_enabledBodyPartsChoices, 0 );
	bSizer5->Add( m_enabledBodyParts, 0, wxALL|wxEXPAND, 5 );
	
	bSizer5->Add( new wxStaticText( m_pModelPanel, wxID_ANY, wxT("LODs"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_lodSpin = new wxSpinCtrl( m_pModelPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	bSizer5->Add( m_lodSpin, 0, wxALL, 5 );
	
	m_lodOverride = new wxCheckBox( m_pModelPanel, wxID_ANY, wxT("override"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_lodOverride, 0, wxALL, 5 );
	
	
	bSizer4->Add( bSizer5, 1, wxEXPAND, 5 );
	
	
	m_pModelPanel->SetSizer( bSizer4 );
	m_pModelPanel->Layout();


	m_notebook1->AddPage( m_pModelPanel, wxT("Model"), false );
	m_pMotionPanel = new wxPanel( m_notebook1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Anim/Act:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pMotionSelection = new wxComboBox( m_pMotionPanel, Event_Sequence_Changed, wxT("select animation..."), wxDefaultPosition, wxDefaultSize, 0, nullptr, 0 );
	fgSizer1->Add( m_pMotionSelection, 0, wxALL, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("FPS:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pAnimFramerate = new wxTextCtrl( m_pMotionPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 35,-1 ), 0 );
	fgSizer1->Add( m_pAnimFramerate, 0, wxALL, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Timeline:"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pTimeline = new wxSlider( m_pMotionPanel, Event_Timeline_Set, 0, 0, 25, wxDefaultPosition, wxSize( 200,42 ), wxSL_AUTOTICKS|wxSL_HORIZONTAL|wxSL_LABELS );
	fgSizer1->Add( m_pTimeline, 1, wxALL, 5 );
	
	fgSizer1->Add( new wxButton( m_pMotionPanel, Event_Sequence_Play, wxT("Play"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT ), 0, wxTOP|wxBOTTOM|wxLEFT, 5 );

	fgSizer1->Add( new wxButton( m_pMotionPanel, Event_Sequence_Stop, wxT("Stop"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT ), 0, wxTOP|wxBOTTOM|wxRIGHT, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Pose controller"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );

	m_pPoseController = new wxComboBox( m_pMotionPanel, Event_PoseCont_Changed, wxT("select controller..."), wxDefaultPosition, wxDefaultSize, 0, nullptr, 0 );
	fgSizer1->Add( m_pPoseController, 0, wxALL, 5 );
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	fgSizer1->Add( new wxStaticText( m_pMotionPanel, wxID_ANY, wxT("Control"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	m_pPoseValue = new wxSlider( m_pMotionPanel, Event_PoseCont_ValueChanged, 50000, 0, 100000, wxDefaultPosition, wxSize( 200,42 ), wxSL_HORIZONTAL );
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
	
	m_pPhysJoint = new wxComboBox( m_pPhysicsPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0 );
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
	
	fgSizer9->Add( new wxButton( m_pPhysicsPanel, Event_Physics_Reset, wxT("Reset"), wxDefaultPosition, wxDefaultSize, 0 ), 0, wxALL, 5 );
	
	
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
	
	m_pIkLinkSel = new wxComboBox( m_pIKPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0 );
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
	
	menuFile->Append( Event_File_OpenModel, DKLOC("TOKEN_OPEN", "Open\tCtrl+O") );
	menuFile->AppendSeparator();
	menuFile->Append( Event_File_ReloadModel, DKLOC("TOKEN_RELOADMODEL", "Reload model") );
	menuFile->Append( Event_File_Save, DKLOC("TOKEN_SAVE", "Save") );
	menuFile->AppendSeparator();
	menuFile->Append( Event_File_CompileModel, DKLOC("TOKEN_COMPILEMODEL", "Compile scripts...") );
	
	

	wxMenu *menuView = new wxMenu;
	
	menuView->Append( Event_View_ResetView, DKLOC("TOKEN_RESETVIEW", "Reset View\tR") );

	m_drawPhysModel = menuView->Append( Event_View_ShowPhysModel, DKLOC("TOKEN_SHOWPHYSICSMODEL", "Show physics objects\tP"), wxEmptyString, wxITEM_CHECK );
	m_drawBones = menuView->Append( Event_View_ShowBones, DKLOC("TOKEN_SHOWBONES", "Show bones\tB"), wxEmptyString, wxITEM_CHECK );
	m_drawAttachments = menuView->Append(Event_View_ShowAttachments, DKLOC("TOKEN_SHOWATTACHMENTS", "Show attachments\tB"), wxEmptyString, wxITEM_CHECK);
	m_drawFloor = menuView->Append( Event_View_ShowFloor, DKLOC("TOKEN_SHOWFLOOR", "Show ground\tB"), wxEmptyString, wxITEM_CHECK);
	m_drawGrid = menuView->Append(Event_View_ShowGrid, DKLOC("TOKEN_SHOWGRID", "Show grid\tB"), wxEmptyString, wxITEM_CHECK);
	m_wireframe = menuView->Append( Event_View_Wireframe, DKLOC("TOKEN_WIREFRAME", "Wireframe\tW"), wxEmptyString, wxITEM_CHECK );

	m_drawGrid->Check();

	m_pMenu->Append( menuFile, DKLOC("TOKEN_FILE", "File") );
	m_pMenu->Append( menuView, DKLOC("TOKEN_VIEW", "View") );

	this->SetMenuBar( m_pMenu );
	this->Centre( wxBOTH );

	// Connect Events
	m_pRenderPanel->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_DCLICK, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_DCLICK, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_DCLICK, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);

	m_pRenderPanel->Connect(wxEVT_MOTION, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEvents, nullptr, this);

	m_pRenderPanel->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEnter, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseLeave, nullptr, this);

	m_pRenderPanel->Connect(wxEVT_MIDDLE_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEnter, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_MIDDLE_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseLeave, nullptr, this);

	m_pRenderPanel->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseEnter, nullptr, this);
	m_pRenderPanel->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&CEGFViewFrame::ProcessMouseLeave, nullptr, this);

	m_bIsMoving = false;
	m_bDoRefresh = true;

	g_timer.GetTime(true);

	RefreshGUI();
}

void CEGFViewFrame::OnIdle(wxIdleEvent &event)
{
	event.RequestMore(true);
	ReDraw();
}

void CEGFViewFrame::OnPaint(wxPaintEvent& event)
{
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
		wxFileDialog* file = new wxFileDialog(nullptr, "Open EGF model",
													wxString::Format("%s/models", g_fileSystem->GetCurrentGameDirectory()), 
													"*.egf", 
													"Equilibrium Graphics File (*.egf)|*.egf;", 
													wxFD_FILE_MUST_EXIST | wxFD_OPEN);

		if(file && file->ShowModal() == wxID_OK)
		{
			EqString model_path(file->GetPath().wchar_str());

			g_model.SetModel(nullptr);
			FlushCache();

			int cache_index = g_studioModelCache->PrecacheModel( model_path.ToCString() );
			if(cache_index == CACHE_INVALID_MODEL)
				wxMessageBox(wxString::Format("Can't open %s\n", model_path.ToCString()), "Error", wxOK | wxICON_EXCLAMATION, this);

			g_model.SetModel( g_studioModelCache->GetModel(cache_index) );
		}

		SetOptimalCameraDistance();
		RefreshGUI();

		delete file;
	}
	else if(event.GetId() == Event_File_ReloadModel)
	{
		CEqStudioGeom* model = g_model.m_pModel;

		if(!model)
			return;

		EqString model_path(model->GetName());

		g_model.SetModel(nullptr);
		FlushCache();

		int cache_index = g_studioModelCache->PrecacheModel( model_path.ToCString() );
		if(cache_index == CACHE_INVALID_MODEL)
			wxMessageBox(wxString::Format("Can't open %s\n", model_path.ToCString()), "Error", wxOK | wxICON_EXCLAMATION, this);

		g_model.SetModel( g_studioModelCache->GetModel(cache_index) );
	}
	else if(event.GetId() == Event_File_CompileModel)
	{
		wxFileDialog* file = new wxFileDialog(nullptr, "Select script files to compile",
													wxEmptyString, 
													"*.esc;*.asc", 
													"Script (*.esc;*.asc)|*.esc;*.asc", 
													wxFD_FILE_MUST_EXIST | wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

		if(file && file->ShowModal() == wxID_OK)
		{
			wxArrayString paths;
			file->GetPaths(paths);

			const char* devAddonDir = g_fileSystem->GetCurrentGameDirectory();

			for(size_t i = 0; i < paths.size(); i++)
			{
				EqString model_path;

				KeyValues script;

				EqString fname( paths[i].wchar_str() );
				EqString ext = fname.Path_Extract_Ext();

				if(!stricmp(ext.GetData(), "asc"))
				{
					EqString cmdLine(EqString::Format("animca.exe -devAddon \"%s\" +filename \"%s\"", devAddonDir, fname.GetData()));

					Msg("***Starting egfCa: %s\n", cmdLine.ToCString());
					if (system(cmdLine.ToCString()) != 0)
					{
						wxMessageBox(wxString::Format("Failed to run command '%s'", cmdLine.ToCString()), "Error", wxOK | wxICON_EXCLAMATION, this);
					}
				}
				else
				{
					if(script.LoadFromFile(fname.GetData()))
					{
						// load all script data
						KVSection* mainsection = script.GetRootSection();
						if(mainsection)
						{
							KVSection* pPair = mainsection->FindSection("modelfilename");

							if(pPair)
							{
								model_path = KV_GetValueString(pPair);
								EqString cmdLine = EqString::Format("egfca.exe -devAddon \"%s\" +filename \"%s\"", devAddonDir, fname.GetData());

								Msg("***Starting egfCa: '%s'\n", cmdLine.ToCString());
								if (system(cmdLine.ToCString()) != 0)
								{
									wxMessageBox(wxString::Format("Failed to run command %s", cmdLine.ToCString()), "Error", wxOK | wxICON_EXCLAMATION, this);
								}
							}
							else
							{
								wxMessageBox("ERROR! 'modelfilename' is not specified in the script!", "Error", wxOK | wxICON_EXCLAMATION, this);
							}
						}
					}

					if(paths.size() == 1 && model_path.Length())
					{
						g_model.SetModel(nullptr);
						FlushCache();

						int cache_index = g_studioModelCache->PrecacheModel( model_path.ToCString() );
						if(cache_index == CACHE_INVALID_MODEL)
							wxMessageBox(wxString::Format("Can't open %s\n", model_path.ToCString()), "Error", wxOK | wxICON_EXCLAMATION, this);

						g_model.SetModel( g_studioModelCache->GetModel(cache_index) );

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
IPhysicsObject* g_pDragable = nullptr;
Vector3D		g_vDragLocalPos(0);
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
				cam_angles.x += move_delta_y * 0.1f;
				cam_angles.y -= move_delta_x * 0.1f;

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

				g_fCamDistance += move_delta_y * 0.05f;

				if (g_fCamDistance < 0.1f)
					g_fCamDistance = 0.1f;

				m_bIsMoving = true;
				bAnyMoveButton = true;

				g_vLastMousePosition = prev_mouse_pos;
			}

			//wxCursor cursor(wxCURSOR_MAGNIFIER);
			//SetCursor(cursor);
		}

		if(event.ButtonIsDown(wxMOUSE_BTN_MIDDLE) && event.Dragging())
		{
			Vector3D right, up;
			AngleVectors(cam_angles, nullptr, &right, &up);

			camera_move_factor *= -1;

			m_bIsMoving = true;
			bAnyMoveButton = true;

			cam_pos += right*move_delta_x * camera_move_factor * g_fCamDistance * 0.01f;
			cam_pos -= up*move_delta_y * camera_move_factor * g_fCamDistance * 0.01f;

			g_vLastMousePosition = prev_mouse_pos;
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

			
			if(g_pDragable == nullptr)
			{
				internaltrace_t tr;
				physics->InternalTraceLine(ray_start, ray_start+ray_dir, COLLISION_GROUP_ALL, &tr);

				//debugoverlay->Line3D(ray_start, tr.traceEnd, ColorRGBA(1,0,0,1), ColorRGBA(0,1,0,1), 10);

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
			g_pDragable = nullptr;
	}

	if(!bAnyMoveButton)
	{
		m_bIsMoving = false;
	}

	if(m_bIsMoving)
	{
		wxPoint pt = ScreenToClient(wxPoint(m_vLastClientCursorPos.x, m_vLastClientCursorPos.y));
		WarpPointer(pt.x, pt.y);
	}

	cam_pos = clamp(cam_pos, Vector3D(-F_INFINITY), Vector3D(F_INFINITY));

	g_camera_rotation = cam_angles;
	g_camera_target = cam_pos;
}

void CEGFViewFrame::OnSize(wxSizeEvent& event)
{
	wxFrame::OnSize( event );

	if(!materials)
		return;

	int w, h;
	m_pRenderPanel->GetSize(&w,&h);
	materials->SetDeviceBackbufferSize(w,h);

	ReDraw();
}

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

void RenderFloor()
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	materials->SetAmbientColor(ColorRGBA(1, 1, 0, 0.15f));

	materials->SetDepthStates(true,true);
	materials->SetRasterizerStates(CULL_FRONT,FILL_SOLID);
	materials->SetBlendingStates(blending);

	MatTextureProxy(materials->FindGlobalMaterialVar(StringToHashConst("basetexture"))).Set(nullptr);

	materials->BindMaterial(materials->GetDefaultMaterial());

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

	meshBuilder.TexturedQuad3(
		Vector3D(-64, 0, -64), 
		Vector3D(64, 0, -64), 
		Vector3D(-64, 0, 64), 
		Vector3D(64, 0, 64), 
		vec2_zero, vec2_zero, vec2_zero, vec2_zero);

	meshBuilder.End();
}

void CEGFViewFrame::ReDraw()
{
	if(!materials)
		return;

	if(!m_bDoRefresh)
		return;

	if(!IsShown())
		return;

	// compute time since last frame
	g_frametime += g_timer.GetTime(true);

	// make 120 FPS
	if (g_frametime < 1.0f / 120.0f)
		return;

	//m_bDoRefresh = false;

	int w, h;
	m_pRenderPanel->GetSize(&w, &h);

	g_pShaderAPI->SetViewport(0, 0, w,h);
	if(materials->BeginFrame(nullptr))
	{
		g_pShaderAPI->Clear(true,true,false, ColorRGBA(0.5,0.5,0.5, 1));

		Vector3D forward, right;
		AngleVectors(g_camera_rotation, &forward, &right);

		g_pCameraParams.SetAngles(g_camera_rotation);
		g_pCameraParams.SetOrigin(g_camera_target - forward * g_fCamDistance);

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

		materials->SetMatrix(MATRIXMODE_WORLD, identity4);

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

		g_model.Update( g_frametime );

		if(g_model.IsSequencePlaying() )
		{
			int nSeq = m_pMotionSelection->GetSelection();
			const gsequence_t& seq = g_model.GetSequence(nSeq);

			float setFrameRate = atoi(m_pAnimFramerate->GetValue());

			if(setFrameRate > 0)
			{
				float framerateScale = setFrameRate / seq.s->framerate;
				g_model.SetPlaybackSpeedScale(framerateScale, 0);
			}

			m_pTimeline->SetValue(g_model.GetCurrentAnimationFrame() );
		}

		// Setup render falgs
		int renderFlags = 0;
		
		if(m_drawPhysModel->IsChecked())
			renderFlags |= RFLAG_PHYSICS;

		if(m_drawBones->IsChecked())
			renderFlags |= RFLAG_BONES;

		if (m_drawAttachments->IsChecked())
			renderFlags |= RFLAG_ATTACHMENTS;

		materials->GetConfiguration().wireframeMode = m_wireframe->IsChecked();

		g_pShaderAPI->ResetCounters();

		// Now we can draw our model
		g_model.Render(renderFlags, g_fCamDistance, m_lodSpin->GetValue(), m_lodOverride->GetValue(), g_frametime);

		debugoverlay->Text(color_white, "polygon count: %d\n", g_pShaderAPI->GetTrianglesCount());

		// reset some values
		materials->SetMatrix(MATRIXMODE_WORLD, identity4);

		// draw floor 1x1 meters
		if(m_drawFloor->IsChecked())
			RenderFloor();

		materials->SetAmbientColor(ColorRGBA(1, 1, 1, 1));

		if (m_drawGrid->IsChecked())
		{
			DrawGrid(1.0f, 8, vec3_zero, ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), true);
			debugoverlay->Line3D(vec3_zero, vec3_right, ColorRGBA(1, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
			debugoverlay->Line3D(vec3_zero, vec3_up, ColorRGBA(0, 1, 0, 1), ColorRGBA(0, 1, 0, 1));
			debugoverlay->Line3D(vec3_zero, vec3_forward, ColorRGBA(0, 0, 1, 1), ColorRGBA(0, 0, 1, 1));
		}

		debugoverlay->Draw(g_mProjMat, g_mViewMat, w,h);

		materials->EndFrame();
		Platform_Sleep(1);
	}

	g_frametime = 0.0f;
}

void CEGFViewFrame::RefreshGUI()
{
	m_enabledBodyParts->Clear();
	m_pMotionSelection->Clear();
	m_pPoseController->Clear();
	g_model.m_bodyGroupFlags = 0xFFFFFFFF;

	// populate all lists
	if(g_model.m_pModel != nullptr)
	{
		const studioHdr_t& studio = g_model.m_pModel->GetStudioHdr();

		for(int i = 0; i < studio.numBodyGroups; i++)
		{
			int idx = m_enabledBodyParts->Append(studio.pBodyGroups(i)->name );
			m_enabledBodyParts->Check(idx, true);
		}

		// sequences
		if(m_pAnimMode->GetSelection() == 0)
		{
			for(int i = 0; i < g_model.GetNumSequences(); i++)
			{
				m_pMotionSelection->Append(g_model.GetSequence(i).s->name );
			}
		}
		else // animations
		{
			/*
			FIXME: UNSUPPORTED for now
			for(int i = 0; i < g_pModel->m_seqList.numElem(); i++)
			{
				m_pMotionSelection->Append( g_pModel->m_seqList[i].name );
			}*/
		}

		for(int i = 0; i < g_model.GetNumPoseControllers(); i++)
		{
			m_pPoseController->Append(g_model.GetPoseController(i).p->name );
		}
	}
}

void CEGFViewFrame::OnComboboxChanged(wxCommandEvent& event)
{
	// TODO: selected slots

	if(event.GetId() == Event_Sequence_Changed)
	{
		int nSeq = m_pMotionSelection->GetSelection();

		if(nSeq != -1)
		{
			g_model.SetSequence( nSeq, 0 );
			g_model.ResetSequenceTime(0);
			g_model.PlaySequence(0);

			const gsequence_t& seq = g_model.GetSequence(nSeq);
			m_pAnimFramerate->SetValue(EqString::Format("%g", seq.s->framerate).ToCString() );

			int maxFrames = g_model.GetCurrentAnimationDurationInFrames();

			m_pTimeline->SetMax( maxFrames );
			g_model.SetPlaybackSpeedScale(1.0f, 0);
		}
	}
	else if(event.GetId() == Event_PoseCont_Changed)
	{
		int nPoseContr = m_pPoseController->GetSelection();

		if(nPoseContr != -1)
		{
			m_pPoseValue->SetMin(g_model.GetPoseController(nPoseContr).p->blendRange[0]*10000 );
			m_pPoseValue->SetMax(g_model.GetPoseController(nPoseContr).p->blendRange[1]*10000 );

			m_pPoseValue->SetValue(g_model.GetPoseControllerValue(nPoseContr)*10000 );
		}
	}
}

void CEGFViewFrame::OnBodyGroupToggled( wxCommandEvent& event )
{
	int itemId = event.GetInt();
	bool isChecked = m_enabledBodyParts->IsChecked(itemId);

	if(isChecked)
		g_model.m_bodyGroupFlags |= (1 << itemId);
	else
		g_model.m_bodyGroupFlags &= ~(1 << itemId);
}

void CEGFViewFrame::OnButtons(wxCommandEvent& event)
{
	// TODO: selected slots

	if(event.GetId() == Event_Sequence_Play)
	{
		g_model.ResetSequenceTime(0);
		g_model.PlaySequence(0);
	}
	else if(event.GetId() == Event_Sequence_Stop)
	{
		g_model.StopSequence(0);
	}
	else if(event.GetId() == Event_Timeline_Set)
	{
		g_model.SetSequenceTime( m_pTimeline->GetValue(), 0 );
	}
	else if(event.GetId() == Event_PoseCont_ValueChanged)
	{
		int nPoseContr = m_pPoseController->GetSelection();

		if(nPoseContr != -1)
			g_model.SetPoseControllerValue(nPoseContr, float(m_pPoseValue->GetValue()) / 10000.0f);
	}
	else if(event.GetId() == Event_Physics_SimulateToggle)
	{
		g_model.TogglePhysicsState();
	}
	else if(event.GetId() == Event_Physics_Reset)
	{
		g_model.ResetPhysics();
	}
	
}

bool InitCore(char *pCmdLine)
{
	// initialize core
	g_eqCore->Init("EGFMan", pCmdLine);

	if(!g_fileSystem->Init(false))
		return false;

	g_eqCore->RegisterInterface(&s_shapeCache);

	g_cmdLine->ExecuteCommandLine();

	return true;
}

IMPLEMENT_APP(CEGFViewApp)

CEGFViewFrame *g_pMainFrame = nullptr;

bool CEGFViewApp::OnInit()
{
#ifdef _DEBUG
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	//flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	flag |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
#endif

#ifdef _WIN32
	InitCore(GetCommandLineA());
#elif __WXGTK__
	InitCore("");
#endif

	setlocale(LC_ALL, "C");

	// first, load matsystem module
	EqString loadErr;
	g_matsysmodule = g_fileSystem->OpenModule("eqMatSystem", &loadErr);

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load eqMatSystem - %s", loadErr.ToCString());
		return -1;
	}

	g_localizer->AddTokensFile("EGFMan");

	g_pMainFrame = new CEGFViewFrame(nullptr, -1, DKLOC("TOKEN_TITLE", "Equilibrium Graphics File viewer 1.0"));
	g_pMainFrame->Centre();
	g_pMainFrame->Show(true);
	SetTopWindow(g_pMainFrame);

	g_pMainFrame->InitializeEq();
	
    return true;
}

int CEGFViewApp::OnExit()
{
	//WriteCfgFile("cfg/editor.cfg");
	
	// shutdown material system
	g_fontCache->Shutdown();
	materials->Shutdown();

	g_fileSystem->CloseModule(g_matsysmodule);
	
	// shutdown core
	g_eqCore->Shutdown();

    return 0;
} 