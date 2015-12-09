//**************** Copyright (C) Parallel Prevision, L.L.C 2011 *****************
//
// Description: Equilibrium Engine Geometry viewer - new with physics support
//
// TODO:	- Standard EGF viewing
//			- Physics model listing with properties
//			- Physics model testing
//			- Full animation support, may be with ik (BaseAnimating.cpp, for game make entity with BaseAnimatingEntity.cpp)
//			- physics model properties saving
//			- sequence properties saving (speed, smoothing, etc.)
//
//*******************************************************************************

#include <windows.h>
#include <commdlg.h>

#include "EditorHeader.h"
#include "EngineModelLoader.h"
#include "DebugOverlay.h"
#include "math/math_util.h"
#include "fs_module.h"

HMODULE*			g_matsysmodule = NULL;
IShaderAPI *		g_pShaderAPI = NULL;
IMaterialSystem *	materials = NULL;

CViewParams		g_pParams(Vector3D(0,0,-100), vec3_zero, 90);
IEngineModel*	g_pLoadedModel = NULL;

float g_fRealtime = 0.0f;
float g_fOldrealtime = 0.0f;
float g_fFrametime = 0.0f;

class CEGFViewApp: public wxApp
{
	bool Initialize(int& argc, wxChar **argv);

    virtual bool OnInit();
	virtual int	 OnExit();

	// TODO: more things
};

IMPLEMENT_APP(CEGFViewApp)

//
// main frame
//

class CEGFViewFrame: public wxFrame
{
public:
    CEGFViewFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

};

enum MenuCommands_e
{
	EVENT_FILE_COMPILE,
	EVENT_FILE_OPEN,
	EVENT_FILE_SAVE,
	EVENT_FILE_EXIT,
	EVENT_VIEW_RESETVIEW,
	EVENT_VIEW_SHOWPHYSICSMODEL,
	EVENT_VIEW_SHOWTBN,
	EVENT_VIEW_SHOWONLYNORMALS,
	EVENT_HELP_ABOUT,
};

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

		FORMAT format = FORMAT_RGB8;

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

		bool materialSystemStatus = materials->Init("materials/", "EqShaderAPID3DX9", materials_config);

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

	materials->LoadShaderLibrary("Shaders_Engine.dll");

	// register all shaders
	for(int i = 0; i < pShaderRegistrators.numElem(); i++)
		materials->RegisterShader( pShaderRegistrators[i].shader_name, pShaderRegistrators[i].dispatcher );

	//viewrenderer->InitializeResources();
}


CEGFViewFrame::CEGFViewFrame(const wxString& title, const wxPoint& pos, const wxSize& size) : wxFrame( NULL, -1, title, pos, size )
{
	wxIcon ico;
	if(ico.LoadFile("IDI_MAINICON"))
	{
		SetIcon(ico);
	}
	else
		MsgError("Can't load icon!\n");

	InitMatSystem((HWND)GetHWND());
}

CEGFViewFrame *g_pMainFrame = NULL;

//
// Application initializer
//

bool CEGFViewApp::Initialize(int& argc, wxChar **argv)
{
	GetCore()->Init("EGFView", argc, argv);

	if(!GetFileSystem()->Init(false))
		return false;

	return wxApp::Initialize(argc, argv);
}

bool CEGFViewApp::OnInit()
{
	// first, load matsystem module
	g_matsysmodule = FS_LoadModule("EqMatSystem.dll");

	if(!g_matsysmodule)
	{
		ErrorMsg("FATAL ERROR! Can't load EqMatSystem!");
		return -1;
	}

	GetLocalizer()->AddTokensFile("egf_view");

	g_pMainFrame = new CEGFViewFrame( DKLOC("TOKEN_TITLE", "Equilibrium Model Viewer") , wxPoint(50, 50), wxSize(1024,768) );
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
	FS_UnloadModule(g_matsysmodule);
	
	// shutdown core
	GetCore()->Shutdown();

    return 0;
} 