//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech material sub-rendering system
//
// Features: - Cached texture loading
//			 - Material management
//			 - Shader management
//////////////////////////////////////////////////////////////////////////////////

#include "platform/Platform.h"
#include "ConVar.h"
#include "ConCommand.h"

#include "MaterialSystem.h"
#include "IDebugOverlay.h"

#include "utils/strtools.h"

#include "Renderers/IRenderLibrary.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"

#pragma fixme("add push/pop renderstates and make regression tests?")

IShaderAPI*				g_pShaderAPI = NULL;
IDebugOverlay*			debugoverlay = NULL;

// register material system
static CMaterialSystem s_matsystem;
IMaterialSystem* materials = &s_matsystem;

// this is an engine functions.
void null_func() {}

RESOURCELOADCALLBACK g_pLoadBeginCallback = null_func;
RESOURCELOADCALLBACK g_pLoadEndCallback = null_func;

ConVar				r_overdraw("r_overdraw", "0", "Renders all materials in overdraw shader", CV_ARCHIVE);

ConVar				r_showlightmaps("r_showlightmaps", "0", "Disable diffuse textures to show lighting", CV_CHEAT);
ConVar				r_screen("r_screen", "0", "Screen count", CV_ARCHIVE);
ConVar				r_loadmiplevel("r_loadmiplevel", "0", 0, 3, "Mipmap level to load, needs texture reloading", CV_ARCHIVE );
ConVar				r_textureanisotrophy("r_textureanisotrophy", "4", "Mipmap anisotropic filtering quality, needs texture reloading", CV_ARCHIVE );

ConVar				r_lightscale("r_lightscale", "1.0f", "Global light scale", CV_ARCHIVE);
ConVar				r_shaderCompilerShowLogs("r_shaderCompilerShowLogs", "0","Show warnings of shader compilation",CV_ARCHIVE);
ConVar				r_wireframe("r_wireframe","0","Enables wireframe rendering",0);

//
// Threaded material loader
//

class CEqMatSystemThreadedLoader : public CEqThread
{
public:
	virtual int Run()
	{
		int counter = 0;

		while(true)
		{
			m_Mutex.Lock();
				// out of bounds
				if(counter >= m_newMaterials.numElem())
				{
					m_newMaterials.setNum(0);
					m_Mutex.Unlock();
					break;
				}

				IMaterial* material = m_newMaterials[counter];
				counter++;
			m_Mutex.Unlock();

			// load this material
			material->LoadShaderAndTextures();

			if(!material)
				continue;
		}

		// done here
		g_pShaderAPI->EndAsyncOperation();

		// run thread code here
		return 0;
	}

	void AddMaterial(IMaterial* pMaterial)
	{
		m_Mutex.Lock();
		m_newMaterials.addUnique( pMaterial );
		m_Mutex.Unlock();
	}

	int GetCount()
	{
		CScopedMutex m(m_Mutex);
		return m_newMaterials.numElem();
	}

protected:
	DkList<IMaterial*>	m_newMaterials;

	CEqMutex			m_Mutex;
};

CEqMatSystemThreadedLoader g_threadedMaterialLoader;

CMaterialSystem::CMaterialSystem()
{
	m_nCurrentCullMode = CULL_BACK;
	m_pShaderAPI = NULL;

	m_bPreloadingMarker = false;

	m_fCurrFrameTime = 0.0f;

	m_pCurrentMaterial = NULL;
	m_pOverdrawMaterial = NULL;

	m_nMaterialChanges = 0;
	m_frame = 0;

	m_nCurrentLightingShaderModel = MATERIAL_LIGHT_UNLIT;

	m_pRenderLib = NULL;
	m_rendermodule = NULL;

	m_ambColor = color4_white;
	memset(&m_fogInfo,0,sizeof(m_fogInfo));

	m_bIsSkinningEnabled = false;
	m_instancingEnabled = false;

	for(int i = 0; i < 3; i++)
		m_matrices[i] = identity4();

	m_pWhiteTexture = NULL;
	m_pDefaultMaterial = NULL;
	m_pEnvmapTexture = NULL;

	m_pCurrentLight = NULL;

	m_bDeviceState = true;

	m_preApplyCallback = NULL;

	// register when the DLL is connected
	GetCore()->RegisterInterface(MATSYSTEM_INTERFACE_VERSION, this);
}

CMaterialSystem::~CMaterialSystem()
{
	// unregister when DLL disconnects
	GetCore()->UnregisterInterface(MATSYSTEM_INTERFACE_VERSION);
}

// Initializes material system
bool CMaterialSystem::Init(const char* materialsDirectory, const char* szShaderAPI, matsystem_render_config_t &config)
{
	Msg(" \n--------- InitMaterialSystem --------- \n");

	ASSERT(g_pShaderAPI == NULL);

	// store the configuration
	m_config = config;

	const char* pszShaderAPILibName = szShaderAPI;

	if(!pszShaderAPILibName)
		pszShaderAPILibName = "EqD3D9RHI";

	// Load Shader API from here...
	m_rendermodule = g_fileSystem->LoadModule(pszShaderAPILibName);
	if(m_rendermodule)
	{
		m_pRenderLib = (IRenderLibrary*)GetCore()->GetInterface( RENDERER_INTERFACE_VERSION );
		if(!m_pRenderLib)
		{
			ErrorMsg("MatSystem Error: Failed to initialize rendering library!!!");
			g_fileSystem->FreeModule(m_rendermodule);
			return false;
		}
	}
	else
	{
		ErrorMsg("MatSystem Error: Cannot load library '%s'!!!", pszShaderAPILibName);
		return false;
	}

	// initialize debug overlay interface
	debugoverlay = (IDebugOverlay*)GetCore()->GetInterface(DBGOVERLAY_INTERFACE_VERSION);

	m_szMaterialsdir = materialsDirectory;
	m_nMaterialChanges = 0;

	// render library initialization
	// collect all needed data for use in shaderapi initialization

	if(m_pRenderLib->InitCaps())
	{
		if(!m_pRenderLib->InitAPI( m_config.shaderapi_params ))
			return false;

		// we created all we've need
		m_pShaderAPI = m_pRenderLib->GetRenderer();

		if(!m_pShaderAPI)
		{
			ErrorMsg("Fatal error! Couldn't create ShaderAPI interface!\n\nPlease update or reinstall the game\n");
			return false;
		}

		// copy default path
		if(m_config.shaderapi_params.textures_path[0] == 0)
			strcpy(m_config.shaderapi_params.textures_path, m_szMaterialsdir.GetData());

		// init new created shader api with this parameters
		m_pShaderAPI->Init( m_config.shaderapi_params );

        // test to be sure that it was initialized correctly
		m_pRenderLib->BeginFrame();
		m_pShaderAPI->Clear(true, true, false);
		m_pRenderLib->EndFrame();
	}
	else
		return false;

	g_pShaderAPI = m_pShaderAPI;

	if(debugoverlay)
		debugoverlay->Graph_AddBucket("Material change count per update", ColorRGBA(1,1,0,1), 100, 0.25f);

	CreateWhiteTexture();

	return true;
}

void CMaterialSystem::CreateWhiteTexture()
{
	// don't worry, it will be removed
	CImage* img = new CImage();

	img->SetName("_matsys_white");

	int nWidth = 4;
	int nHeight = 4;

	ubyte* pLightmapData = img->Create(FORMAT_RGBA8, nWidth,nHeight,1,1);

	PixelWriter pixw;
	pixw.SetPixelMemory(FORMAT_RGBA8, pLightmapData, 0);

	for (int y = 0; y < nHeight; ++y)
	{
		pixw.Seek( 0, y );
		for (int x = 0; x < nWidth; ++x)
		{
			pixw.WritePixel( 255, 255, 255, 255 );
		}
	}

	SamplerStateParam_t texSamplerParams = g_pShaderAPI->MakeSamplerState(TEXFILTER_TRILINEAR_ANISO,ADDRESSMODE_CLAMP,ADDRESSMODE_CLAMP,ADDRESSMODE_CLAMP);

	DkList<CImage*> images;
	images.append(img);

	m_pWhiteTexture = g_pShaderAPI->CreateTexture(images, texSamplerParams, TEXFLAG_NOQUALITYLOD);

	if(m_pWhiteTexture)
		m_pWhiteTexture->Ref_Grab();

	images.clear();

	//--------------------------------------------------------------------
	/*
	// don't worry, it will be removed
	img = new CImage();

	img->SetName("_matsys_luxeltest");

	nWidth = 1024;
	nHeight = 1024;

	pLightmapData = img->Create(FORMAT_RGBA8, nWidth,nHeight,1,1);

	pixw.SetPixelMemory(FORMAT_RGBA8, pLightmapData, 0);

	for (int y = 0; y < nHeight; ++y)
	{
		pixw.Seek( 0, y );

		for (int x = 0; x < nWidth; ++x)
		{
			if ((x & (1 << 0)) ^ (y & (1 << 0)))
				pixw.WritePixel( 0, 0, 255, 255 );
			else
				pixw.WritePixel( 255, 255, 255, 0 );
		}
	}

	images.append(img);

	texSamplerParams = g_pShaderAPI->MakeSamplerState(TEXFILTER_NEAREST,ADDRESSMODE_CLAMP,ADDRESSMODE_CLAMP,ADDRESSMODE_CLAMP);

	m_pLuxelTestTexture = g_pShaderAPI->CreateTexture(images, texSamplerParams, TEXFLAG_NOQUALITYLOD);

	if(m_pLuxelTestTexture)
		m_pLuxelTestTexture->Ref_Grab();
	*/
}

void CMaterialSystem::InitDefaultMaterial()
{
	if(!m_pDefaultMaterial)
	{
		CMaterial* pMaterial = (CMaterial*)FindMaterial("engine/matsys_default");
		pMaterial->Ref_Grab();
		pMaterial->LoadShaderAndTextures();

		m_pDefaultMaterial = pMaterial;
	}

	if(!m_pOverdrawMaterial)
	{
		CMaterial* pMaterial = (CMaterial*)FindMaterial("engine/matsys_overdraw");
		pMaterial->Ref_Grab();
		pMaterial->LoadShaderAndTextures();

		m_pOverdrawMaterial = pMaterial;
	}
}

bool CMaterialSystem::IsInStubMode()
{
	return m_config.stubMode;
}

void CMaterialSystem::Shutdown()
{
	if(m_pRenderLib && m_rendermodule && g_pShaderAPI)
	{
		Msg("MatSystem shutdown...\n");

		for (blendStateMap_t::iterator i = m_blendStates.begin(); i != m_blendStates.end(); ++i)
			g_pShaderAPI->DestroyRenderState(i->second);

		for (depthStateMap_t::iterator i = m_depthStates.begin(); i != m_depthStates.end(); ++i)
			g_pShaderAPI->DestroyRenderState(i->second);

		for (rasterStateMap_t::iterator i = m_rasterStates.begin(); i != m_rasterStates.end(); ++i)
			g_pShaderAPI->DestroyRenderState(i->second);

		// shutdown thread first
		g_threadedMaterialLoader.StopThread(true);

		if(m_pWhiteTexture)
			g_pShaderAPI->FreeTexture(m_pWhiteTexture);

		if(m_pLuxelTestTexture)
			g_pShaderAPI->FreeTexture(m_pLuxelTestTexture);

		FreeMaterials(true);

		m_pRenderLib->ReleaseSwapChains();
		g_pShaderAPI->Shutdown();
		m_pRenderLib->ExitAPI();

		// shutdown render libraries, all shaders and other
		g_fileSystem->FreeModule( m_rendermodule );
	}
}

matsystem_render_config_t& CMaterialSystem::GetConfiguration()
{
	return m_config;
}

void CMaterialSystem::SetResourceBeginEndLoadCallback(RESOURCELOADCALLBACK begin, RESOURCELOADCALLBACK end)
{
	g_pLoadBeginCallback = begin;
	g_pLoadEndCallback = end;
}

typedef int (*InitShaderLibraryFunction)(IMaterialSystem* pMatSystem);

// Adds new shader library
bool CMaterialSystem::LoadShaderLibrary(const char* libname)
{
	DKMODULE* shaderlib = g_fileSystem->LoadModule( libname );

	if(!shaderlib)
		return false;

	InitShaderLibraryFunction functionPtr = (InitShaderLibraryFunction)g_fileSystem->GetProcedureAddress(shaderlib, "InitShaderLibrary");

	if(!functionPtr)
	{
		MsgError("Invalid shader library '%s' - can't get InitShaderLibrary function!\n",libname);
		return false;
	}

	(functionPtr)(this);

	m_hShaderLibraries.append(shaderlib);

	return true;
}

bool CMaterialSystem::IsMaterialExist(const char* szMaterialName)
{
	EqString mat_path(m_szMaterialsdir + szMaterialName + _Es(".mat"));

	return g_fileSystem->FileExist(mat_path.GetData());
}

IMaterial* CMaterialSystem::FindMaterial(const char* szMaterialName,bool findExisting/* = false*/)
{
	// Don't load null materials
	if( strlen(szMaterialName) == 0 )
		return NULL;

	CScopedMutex m(m_Mutex);

	g_pLoadBeginCallback();

	if(findExisting)
	{
		EqString search_string;

		if( szMaterialName[0] == '/' || szMaterialName[0] == '\\' )
			search_string = szMaterialName+1;
		else
			search_string = szMaterialName;

		search_string.Path_FixSlashes();

		for(int i = 0; i < m_pLoadedMaterials.numElem(); i++)
		{
			if(m_pLoadedMaterials[i] != NULL)
			{
				if(!search_string.CompareCaseIns( m_pLoadedMaterials[i]->GetName() ))
				{
					g_pLoadEndCallback();
					return m_pLoadedMaterials[i];
				}
			}
		}
	}

	// create new material
	CMaterial *pMaterial = new CMaterial;

	// initialize it
	pMaterial->Init( szMaterialName );

	if( m_bPreloadingMarker )
		PutMaterialToLoadingQueue( pMaterial );

	// add to list
	m_pLoadedMaterials.append(pMaterial);

	g_pLoadEndCallback();

	return pMaterial;
}

// If we have unliaded material, just load it
void CMaterialSystem::PreloadNewMaterials()
{
	if(m_config.stubMode)
		return;

	CScopedMutex m(m_Mutex);

	g_pLoadBeginCallback();

	for(int i = 0; i < m_pLoadedMaterials.numElem(); i++)
	{
		if(m_pLoadedMaterials[i] != NULL)
		{
			if(m_pLoadedMaterials[i]->GetState() != MATERIAL_LOAD_NEED_LOAD)
				continue;

			PutMaterialToLoadingQueue(m_pLoadedMaterials[i]);
		}
	}

	g_pLoadEndCallback();
}

// begins preloading zone of materials when FindMaterial calls
void CMaterialSystem::BeginPreloadMarker()
{
	if(!m_bPreloadingMarker)
	{
		g_pLoadBeginCallback();
		m_bPreloadingMarker = true;
	}
}

// ends preloading zone of materials when FindMaterial calls
void CMaterialSystem::EndPreloadMarker()
{
	if(m_bPreloadingMarker)
	{
		g_pLoadEndCallback();
		m_bPreloadingMarker = false;
	}
}

// Reloads materials
void CMaterialSystem::ReloadAllTextures()
{
	ReloadAllMaterials(true,false);
}

// Frees all textures
void CMaterialSystem::FreeAllTextures()
{
	CScopedMutex m(m_Mutex);

	for(int i = 0; i < m_pLoadedMaterials.numElem(); i++)
	{
		if(m_pLoadedMaterials[i] != NULL)
		{
			EqString old_name(m_pLoadedMaterials[i]->GetName());

			((CMaterial*)m_pLoadedMaterials[i])->Cleanup(false, true);
		}
	}
}

// Reloads materials
void CMaterialSystem::ReloadAllMaterials(bool bTouchTextures,bool bTouchShaders, bool wait)
{
	CScopedMutex m(m_Mutex);

	g_pLoadBeginCallback();

	for(int i = 0; i < m_pLoadedMaterials.numElem(); i++)
	{
		EqString old_name(m_pLoadedMaterials[i]->GetName());

		CMaterial* mat = (CMaterial*)m_pLoadedMaterials[i];

		// Flush materials
		mat->Cleanup(bTouchShaders, bTouchTextures, false);
		mat->Init( old_name.GetData(), false );

		// preload material if it was visible
		if(mat->m_frameBound == m_frame-1 || mat->m_frameBound == m_frame)
			PutMaterialToLoadingQueue(mat);
	}

	PreloadNewMaterials();

	if( wait )
		Wait();

	g_pLoadEndCallback();
}

// frees all materials (if bFreeAll is positive, will free locked)
void CMaterialSystem::FreeMaterials(bool bFreeAll)
{
	CScopedMutex m(m_Mutex);

	for(int i = 0; i < m_pLoadedMaterials.numElem(); i++)
	{
		DevMsg(DEVMSG_MATSYSTEM, "freeing %s\n", m_pLoadedMaterials[i]->GetName());

		((CMaterial*)m_pLoadedMaterials[i])->Cleanup();
		CMaterial* pMaterial = (CMaterial*)m_pLoadedMaterials[i];
		delete ((CMaterial*)pMaterial);
	}
}

// frees single material
void CMaterialSystem::FreeMaterial(IMaterial *pMaterial)
{
	// if already freed.
	if(pMaterial == NULL)
		return;

	CScopedMutex m(m_Mutex);

	pMaterial->Ref_Drop();

	if(pMaterial->Ref_Count() <= 0)
	{
		((CMaterial*)pMaterial)->Cleanup();
		DevMsg(DEVMSG_MATSYSTEM,"freeing %s\n",pMaterial->GetName());

		m_pLoadedMaterials.fastRemove(pMaterial);
		delete ((CMaterial*)pMaterial);
	}
}

void CMaterialSystem::RegisterShader(const char* pszShaderName, DISPATCH_CREATE_SHADER dispatcher_creation)
{
	for(int i = 0; i < m_hShaders.numElem(); i++)
	{
		if(!stricmp(m_hShaders[i].shader_name,pszShaderName))
		{
			ErrorMsg("Programming Error! The shader '%s' is already exist!\n",pszShaderName);
			exit(-1);
		}
	}

	DevMsg(DEVMSG_MATSYSTEM, "Registering shader '%s'\n", pszShaderName);

	shaderfactory_t newShader;

	newShader.dispatcher = dispatcher_creation;
	newShader.shader_name = (char*)pszShaderName;

	m_hShaders.append(newShader);
}

// registers overrider for shaders
void CMaterialSystem::RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function)
{
	shaderoverride_t new_override;
	new_override.shadername = xstrdup(shaderName);
	new_override.function = check_function;

	// this is a higher priority
	m_ShaderOverrideList.insert(new_override, 0);
}

IMaterialSystemShader* CMaterialSystem::CreateShaderInstance(const char* szShaderName)
{
	EqString shaderName( szShaderName );

	// check the override table
	for(int i = 0; i < m_ShaderOverrideList.numElem(); i++)
	{
		if(!stricmp(szShaderName,m_ShaderOverrideList[i].shadername))
		{
			shaderName = m_ShaderOverrideList[i].function();
			break;
		}
	}

	// now find the factory and dispatch
	for(int i = 0; i < m_hShaders.numElem(); i++)
	{
		if(!shaderName.CompareCaseIns( m_hShaders[i].shader_name ))
		{
			IMaterialSystemShader* pShader = (m_hShaders[i].dispatcher)();

			ASSERT(pShader);

			return pShader;
		}
	}

	return NULL;
}

typedef bool (*PFNMATERIALBINDCALLBACK)(IMaterial* pMaterial);

void BindFFPMaterial(IMaterial* pMaterial)
{
	g_pShaderAPI->SetShader(NULL);

	g_pShaderAPI->SetBlendingState(NULL);
	g_pShaderAPI->SetRasterizerState(NULL);
	g_pShaderAPI->SetDepthStencilState(NULL);
}

bool Callback_BindErrorTextureFFPMaterial(IMaterial* pMaterial)
{
	BindFFPMaterial(pMaterial);
	g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());

	return false;
}

bool Callback_BindFFPMaterial(IMaterial* pMaterial)
{
	BindFFPMaterial(pMaterial);

	// bind same, but with base texture
	g_pShaderAPI->SetTexture(pMaterial->GetBaseTexture());

	return false;
}

bool Callback_BindNormalMaterial(IMaterial* pMaterial)
{
	((CMaterial*)pMaterial)->Setup();

	return true;
}

PFNMATERIALBINDCALLBACK materialstate_callbacks[] =
{
	Callback_BindErrorTextureFFPMaterial,	// binds ffp shader with error texture, error shader only
	Callback_BindFFPMaterial,				// binds ffp shader with material textures, only FFP mode
	Callback_BindNormalMaterial				// binds normal material.
};

// loads material or sends it to loader thread
void CMaterialSystem::PutMaterialToLoadingQueue(IMaterial* pMaterial)
{
	if(pMaterial->GetState() != MATERIAL_LOAD_NEED_LOAD)
		return;

	if( m_config.threadedloader )
	{
		g_threadedMaterialLoader.AddMaterial( pMaterial );

		if(!g_threadedMaterialLoader.IsRunning())
			g_threadedMaterialLoader.StartWorkerThread("matSystemLoader");

		// make async ops happen
		g_pShaderAPI->BeginAsyncOperation( g_threadedMaterialLoader.GetThreadID() );

		g_threadedMaterialLoader.SignalWork();
	}
	else
	{
		pMaterial->LoadShaderAndTextures();
	}
}

int CMaterialSystem::GetLoadingQueue() const
{
	return g_threadedMaterialLoader.GetCount();
}

bool CMaterialSystem::BindMaterial(IMaterial *pMaterial, bool doApply/* = true*/)
{
	if(!pMaterial)
		return false;

	CMaterial* pSetupMaterial = (CMaterial*)pMaterial;

	// set proxy is dirty
	pSetupMaterial->m_proxyIsDirty = (pSetupMaterial->m_frameBound != m_frame);

	// set bound frame
	pSetupMaterial->m_frameBound = m_frame;

	// preload shader and textures
	PutMaterialToLoadingQueue( pMaterial );

	// set the current material
	m_pCurrentMaterial = pMaterial;
	m_nMaterialChanges++;

	int state = 2;

	if(m_config.ffp_mode)
		state = 1;

	if( pMaterial->GetState() == MATERIAL_LOAD_ERROR )
		state = 0;

	// if material is still loading, use the default material for a while
	if( pMaterial->GetState() == MATERIAL_LOAD_INQUEUE )
	{
		// first do that
		InitDefaultMaterial();

		// set to default material first, wait for loading
		m_pCurrentMaterial = m_pDefaultMaterial;

		state = 2;
	}

	bool success = false;

	if( r_overdraw.GetBool() )
	{
		// do that too
		InitDefaultMaterial();

		materials->SetAmbientColor(ColorRGBA(0.045, 0.02, 0.02, 1.0));
		success = (materialstate_callbacks[state])(m_pOverdrawMaterial);
	}
	else
		success = (materialstate_callbacks[state])(m_pCurrentMaterial);

	if(doApply)
		Apply();

	return success;
}

// Applies current material
void CMaterialSystem::Apply()
{
	if(m_preApplyCallback)
		m_preApplyCallback->OnPreApplyMaterial( m_pCurrentMaterial );

	g_pShaderAPI->Apply();
}

// returns bound material
IMaterial* CMaterialSystem::GetBoundMaterial()
{
	return m_pCurrentMaterial;
}

// captures screenshot to CImage data
bool CMaterialSystem::CaptureScreenshot(CImage &img)
{
	return m_pRenderLib->CaptureScreenshot( img );
}

// update all materials and proxies
void CMaterialSystem::Update(float dt)
{
	if(debugoverlay)
		debugoverlay->Graph_AddValue(0, m_nMaterialChanges);

	m_fCurrFrameTime = dt;

	m_nMaterialChanges = 0;
}

// waits for material loader thread is finished
void CMaterialSystem::Wait()
{
	if(m_config.threadedloader)
		g_threadedMaterialLoader.WaitForThread();
}

// transform operations
// sets up a matrix, projection, view, and world
void CMaterialSystem::SetMatrix(MatrixMode_e mode, const Matrix4x4 &matrix)
{
	m_matrices[(int)mode] = matrix;

	g_pShaderAPI->SetMatrixMode(mode);

	g_pShaderAPI->PopMatrix();
	g_pShaderAPI->LoadMatrix(matrix);
	g_pShaderAPI->PushMatrix();
}

// returns a typed matrix
void CMaterialSystem::GetMatrix(MatrixMode_e mode, Matrix4x4 &matrix)
{
	matrix = m_matrices[(int)mode];
}

// retunrs multiplied matrix
void CMaterialSystem::GetWorldViewProjection(Matrix4x4 &matrix)
{
	matrix = identity4() * m_matrices[MATRIXMODE_PROJECTION] * (m_matrices[MATRIXMODE_VIEW] * m_matrices[MATRIXMODE_WORLD]);
}

// sets an ambient light
void CMaterialSystem::SetAmbientColor(const ColorRGBA &color)
{
	m_ambColor = color;
}

ColorRGBA CMaterialSystem::GetAmbientColor()
{
	return m_ambColor;
}

// sets current light for processing in shaders
void CMaterialSystem::SetLight(dlight_t* pLight)
{
	m_pCurrentLight = pLight;
}

dlight_t* CMaterialSystem::GetLight()
{
	return m_pCurrentLight;
}

ITexture* CMaterialSystem::GetWhiteTexture()
{
	return m_pWhiteTexture;
}

ITexture* CMaterialSystem::GetLuxelTestTexture()
{
	return m_pLuxelTestTexture;
}

//-----------------------------
// Frame operations
//-----------------------------

// tells 3d device to begin frame
bool CMaterialSystem::BeginFrame()
{
	if(m_config.stubMode || !m_pShaderAPI)
		return false;

	bool oldState = m_bDeviceState;
	m_bDeviceState = g_pShaderAPI->IsDeviceActive();

	if(!m_bDeviceState && m_bDeviceState != oldState)
	{
		for(int i = 0; i < m_pDeviceLostCb.numElem(); i++)
		{
			if(!m_pDeviceLostCb[i]())
				return false;
		}
	}

	m_pRenderLib->BeginFrame();

	if(m_bDeviceState && m_bDeviceState != oldState)
	{
		for(int i = 0; i < m_pDeviceLostCb.numElem(); i++)
		{
			if(!m_pDeviceRestoreCb[i]())
				return false;
		}
	}

	if( r_overdraw.GetBool() )
		g_pShaderAPI->Clear(true, false, false, ColorRGBA(0, 0, 0, 0));

	return true;
}

// tells 3d device to end and present frame
bool CMaterialSystem::EndFrame(IEqSwapChain* swapChain)
{
	if(m_pRenderLib)
		m_pRenderLib->EndFrame(swapChain);

	m_frame++;;

	return true;
}

// resizes device back buffer. Must be called if window resized
void CMaterialSystem::SetDeviceBackbufferSize(int wide, int tall)
{
	if(m_pRenderLib)
		m_pRenderLib->SetBackbufferSize(wide, tall);
}

IEqSwapChain* CMaterialSystem::CreateSwapChain(void* windowHandle)
{
	if(m_pRenderLib)
		return m_pRenderLib->CreateSwapChain(windowHandle, m_config.shaderapi_params.bIsWindowed);

	return NULL;
}

void CMaterialSystem::DestroySwapChain(IEqSwapChain* swapChain)
{
	if(m_pRenderLib)
		m_pRenderLib->DestroySwapChain(swapChain);
}

// sets a fog info
void CMaterialSystem::SetFogInfo(const FogInfo_t &info)
{
	m_fogInfo = info;
}

// returns fog info
void CMaterialSystem::GetFogInfo(FogInfo_t &info)
{
	if( r_overdraw.GetBool() )
	{
		static FogInfo_t nofog;

		info = nofog;

		return;
	}

	info = m_fogInfo;
}

// sets current lighting model as state
void CMaterialSystem::SetCurrentLightingModel(MaterialLightingMode_e lightingModel)
{
	m_nCurrentLightingShaderModel = lightingModel;
}

// Lighting model (e.g shadow maps or light maps)
void CMaterialSystem::SetLightingModel(MaterialLightingMode_e lightingModel)
{
	m_config.lighting_model = lightingModel;
}

MaterialLightingMode_e CMaterialSystem::GetCurrentLightingModel()
{
	return m_nCurrentLightingShaderModel;
}

// Lighting model (e.g shadow maps or light maps)
MaterialLightingMode_e CMaterialSystem::GetLightingModel()
{
	return m_config.lighting_model;
}

// sets pre-apply callback
void CMaterialSystem::SetMaterialRenderParamCallback( IMaterialRenderParamCallbacks* callback )
{
	m_preApplyCallback = callback;
}

// returns current pre-apply callback
IMaterialRenderParamCallbacks* CMaterialSystem::GetMaterialRenderParamCallback()
{
	return m_preApplyCallback;
}

// sets pre-apply callback
void CMaterialSystem::SetEnvironmentMapTexture( ITexture* pEnvMapTexture )
{
	if (m_pEnvmapTexture)
		m_pEnvmapTexture->Ref_Drop();

	m_pEnvmapTexture = pEnvMapTexture;

	if (m_pEnvmapTexture)
		m_pEnvmapTexture->Ref_Grab();
}

// returns current pre-apply callback
ITexture* CMaterialSystem::GetEnvironmentMapTexture()
{
	return m_pEnvmapTexture;
}

// skinning mode
void CMaterialSystem::SetSkinningEnabled( bool bEnable )
{
	m_bIsSkinningEnabled = bEnable;
}

bool CMaterialSystem::IsSkinningEnabled()
{
	return m_bIsSkinningEnabled;
}

// instancing mode
void CMaterialSystem::SetInstancingEnabled( bool bEnable )
{
	m_instancingEnabled = bEnable;
}

bool CMaterialSystem::IsInstancingEnabled()
{
	return m_instancingEnabled;
}

//-----------------------------
// Helper operations
//-----------------------------

// sets up a 2D mode
void CMaterialSystem::Setup2D(float wide, float tall)
{
	SetMatrix(MATRIXMODE_PROJECTION, projection2DScreen(wide,tall));
	SetMatrix(MATRIXMODE_VIEW, identity4());
	SetMatrix(MATRIXMODE_WORLD, identity4());
}

// sets up 3D mode, projection
void CMaterialSystem::SetupProjection(float wide, float tall, float fFOV, float zNear, float zFar)
{
	SetMatrix(MATRIXMODE_PROJECTION, perspectiveMatrixY(DEG2RAD(fFOV), wide, tall, zNear, zFar));
}

// sets up 3D mode, orthogonal
void CMaterialSystem::SetupOrtho(float left, float right, float top, float bottom, float zNear, float zFar)
{
	// using orthoMatrixR, because renderer fucks with Z-far plane
	SetMatrix(MATRIXMODE_PROJECTION, orthoMatrixR(left,right,top,bottom,zNear,zFar));
}

//-----------------------------
// Helper rendering operations (warning, slow)
//-----------------------------

ConVar r_noffp("r_noffp","0","No FFP emulated primitives", CV_CHEAT);

// draws 2D primitives
void CMaterialSystem::DrawPrimitivesFFP(PrimitiveType_e type, Vertex3D_t *pVerts, int nVerts,
										ITexture* pTexture, const ColorRGBA &color,
										BlendStateParam_t* blendParams, DepthStencilStateParams_t* depthParams,
										RasterizerStateParams_t* rasterParams)
{
	if(r_noffp.GetBool())
		return;

	g_pShaderAPI->Reset();

	g_pShaderAPI->SetTexture(pTexture, NULL, 0);

	if(!blendParams)
		SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA,BLENDFUNC_ADD, COLORMASK_ALL);
	else
		SetBlendingStates(*blendParams);

	if(!rasterParams)
		SetRasterizerStates(CULL_NONE,FILL_SOLID);
	else
		SetRasterizerStates(*rasterParams);

	if(!depthParams)
		SetDepthStates(false,false);
	else
		SetDepthStates(*depthParams);

	g_pShaderAPI->Apply();

	IMeshBuilder* pMB = m_pShaderAPI->CreateMeshBuilder();

	if(pMB)
	{
		pMB->Begin(type);

		for(int i = 0; i < nVerts; i++)
		{
			pMB->Color4fv(pVerts[i].m_vColor * color);
			pMB->TexCoord2fv(pVerts[i].m_vTexCoord);
			pMB->Position3fv(pVerts[i].m_vPosition);

			pMB->AdvanceVertex();
		}

		pMB->End();
	}

	m_pShaderAPI->DestroyMeshBuilder(pMB);

	g_pShaderAPI->SetBlendingState(NULL);
	g_pShaderAPI->SetRasterizerState(NULL);
	g_pShaderAPI->SetDepthStencilState(NULL);
}

void CMaterialSystem::DrawPrimitives2DFFP(	PrimitiveType_e type, Vertex2D_t *pVerts, int nVerts,
											ITexture* pTexture, const ColorRGBA &color,
											BlendStateParam_t* blendParams, DepthStencilStateParams_t* depthParams,
											RasterizerStateParams_t* rasterParams)
{
	if(r_noffp.GetBool())
		return;

	g_pShaderAPI->Reset();

	g_pShaderAPI->SetTexture(pTexture,0);

	if(!blendParams)
		SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO,BLENDFUNC_ADD);
	else
		SetBlendingStates(*blendParams);

	if(!rasterParams)
		SetRasterizerStates(CULL_NONE,FILL_SOLID);
	else
		SetRasterizerStates(*rasterParams);

	if(!depthParams)
		SetDepthStates(false,false);
	else
		SetDepthStates(*depthParams);

	g_pShaderAPI->Apply();

	IMeshBuilder* pMB = m_pShaderAPI->CreateMeshBuilder();

	if(pMB)
	{
		pMB->Begin(type);

		for(int i = 0; i < nVerts; i++)
		{
			pMB->Color4fv(pVerts[i].m_vColor * color);
			pMB->TexCoord2f(pVerts[i].m_vTexCoord.x,pVerts[i].m_vTexCoord.y);
			pMB->Position3f(pVerts[i].m_vPosition.x, pVerts[i].m_vPosition.y, 0);

			pMB->AdvanceVertex();
		}

		pMB->End();
	}

	m_pShaderAPI->DestroyMeshBuilder(pMB);

	g_pShaderAPI->SetBlendingState(NULL);
	g_pShaderAPI->SetRasterizerState(NULL);
	g_pShaderAPI->SetDepthStencilState(NULL);
}


//-----------------------------
// State setup
//-----------------------------

// sets blending
void CMaterialSystem::SetBlendingStates(const BlendStateParam_t& blend)
{
	SetBlendingStates(blend.srcFactor, blend.dstFactor, blend.blendFunc, blend.mask);
}

// sets depth stencil state
void CMaterialSystem::SetDepthStates(const DepthStencilStateParams_t& depth)
{
	SetDepthStates(depth.depthTest, depth.depthWrite, depth.depthFunc);
}

// sets rasterizer extended mode
void CMaterialSystem::SetRasterizerStates(const RasterizerStateParams_t& raster)
{
	SetRasterizerStates(raster.cullMode, raster.fillMode, raster.multiSample, raster.scissor);
}

// pack blending function to ushort
struct blendStateIndex_t
{
	blendStateIndex_t( BlendingFactor_e nSrcFactor, BlendingFactor_e nDestFactor, BlendingFunction_e nBlendingFunc, int colormask )
		: srcFactor(nSrcFactor), destFactor(nDestFactor), colMask(colormask), blendFunc(nBlendingFunc)
	{
	}

	ushort srcFactor : 4;
	ushort destFactor : 4;
	ushort colMask : 4;
	ushort blendFunc : 4;
};

assert_sizeof(blendStateIndex_t,2);

// sets blending
void CMaterialSystem::SetBlendingStates(BlendingFactor_e nSrcFactor, BlendingFactor_e nDestFactor, BlendingFunction_e nBlendingFunc, int colormask)
{
	blendStateIndex_t idx(nSrcFactor, nDestFactor, nBlendingFunc, colormask);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	if( !m_blendStates.count(stateIndex) )
	{
		BlendStateParam_t desc;
		// no sense to enable blending when no visual effects...
		desc.blendEnable = !(nSrcFactor == BLENDFACTOR_ONE && nDestFactor == BLENDFACTOR_ZERO && nBlendingFunc == BLENDFUNC_ADD);
		desc.srcFactor = nSrcFactor;
		desc.dstFactor = nDestFactor;
		desc.blendFunc = nBlendingFunc;
		desc.mask = colormask;

		state = g_pShaderAPI->CreateBlendingState(desc);
		m_blendStates[stateIndex] = state;
	}
	else
		state = m_blendStates[stateIndex];

	g_pShaderAPI->SetBlendingState( state );
}

// pack depth states to ubyte
struct depthStateIndex_t
{
	depthStateIndex_t( bool bDoDepthTest, bool bDoDepthWrite, CompareFunc_e depthCompFunc )
		: doDepthTest(bDoDepthTest), doDepthWrite(bDoDepthWrite), compFunc(depthCompFunc)
	{
	}

	ubyte doDepthTest : 1;
	ubyte doDepthWrite : 1;
	ubyte compFunc : 3;
};

assert_sizeof(depthStateIndex_t,1);

// sets depth stencil state
void CMaterialSystem::SetDepthStates(bool bDoDepthTest, bool bDoDepthWrite, CompareFunc_e depthCompFunc)
{
	depthStateIndex_t idx(bDoDepthTest, bDoDepthWrite, depthCompFunc);
	ubyte stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	if( !m_depthStates.count(stateIndex) )
	{
		DepthStencilStateParams_t desc;
		desc.depthWrite = bDoDepthWrite;
		desc.depthTest = bDoDepthTest;
		desc.depthFunc = depthCompFunc;
		desc.doStencilTest = false;

		state = g_pShaderAPI->CreateDepthStencilState(desc);
		m_depthStates[stateIndex] = state;
	}
	else
		state = m_depthStates[stateIndex];

	g_pShaderAPI->SetDepthStencilState( state );
}

// pack blending function to ushort
struct rasterStateIndex_t
{
	rasterStateIndex_t( CullMode_e nCullMode, FillMode_e nFillMode, bool bMultiSample,bool bScissor )
		: cullMode(nCullMode), fillMode(nFillMode), multisample(bMultiSample), scissor(bScissor)
	{
	}

	ubyte cullMode : 2;
	ubyte fillMode : 2;
	ubyte multisample : 1;
	ubyte scissor : 1;
};

assert_sizeof(rasterStateIndex_t,1);

// sets rasterizer extended mode
void CMaterialSystem::SetRasterizerStates(CullMode_e nCullMode, FillMode_e nFillMode,bool bMultiSample,bool bScissor)
{
	rasterStateIndex_t idx(nCullMode, nFillMode, bMultiSample, bScissor);
	ubyte stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	if( !m_rasterStates.count(stateIndex) )
	{
		RasterizerStateParams_t desc;
		desc.cullMode = nCullMode;
		desc.fillMode = nFillMode;
		desc.multiSample = bMultiSample;
		desc.scissor = bScissor;

		state = g_pShaderAPI->CreateRasterizerState(desc);
		m_rasterStates[stateIndex] = state;
	}
	else
		state = m_rasterStates[stateIndex];

	g_pShaderAPI->SetRasterizerState( state );
}

// use this if you have objects that must be destroyed when device is lost
void CMaterialSystem::AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore)
{
	m_pDeviceLostCb.append(destroy);
	m_pDeviceRestoreCb.append(restore);
}

// prints loaded materials to console
void CMaterialSystem::PrintLoadedMaterials()
{
	CScopedMutex m(m_Mutex);

	Msg("---MATERIALS---\n");
	for(int i = 0; i < m_pLoadedMaterials.numElem(); i++)
	{
		if(m_pLoadedMaterials[i])
			Msg("%s (%d)\n", m_pLoadedMaterials[i]->GetName(), m_pLoadedMaterials[i]->Ref_Count());
	}
	Msg("---END MATERIALS---\n");
}

// removes callbacks from list
void CMaterialSystem::RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore)
{
	m_pDeviceLostCb.remove(destroy);
	m_pDeviceRestoreCb.remove(restore);
}

DECLARE_CMD(r_reloadallmaterials,"Reloads all materials",0)
{
	MsgInfo("*** Reloading materials...\n \n");
	materials->ReloadAllMaterials();
}

DECLARE_CMD(r_unloadmaterialtextures,"Frees all textures",0)
{
	MsgInfo("*** Unloading textures...\n \n");
	materials->FreeAllTextures();
}

DECLARE_CMD(r_materialsInfo,"Matsystem info",0)
{
	materials->PrintLoadedMaterials();
}
