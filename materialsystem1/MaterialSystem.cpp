//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material sub-rendering system
//
// Features: - Cached texture loading
//			 - Material management
//			 - Shader management
//////////////////////////////////////////////////////////////////////////////////

#include "core/platform/Platform.h"

#include "core/platform/MessageBox.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IFileSystem.h"

#include "utils/strtools.h"
#include "utils/KeyValues.h"
#include "utils/eqthread.h"
#include "utils/eqtimer.h"

#include "materialsystem1/MeshBuilder.h"
#include "DynamicMesh.h"

#include "MaterialSystem.h"
#include "MaterialProxy.h"
#include "material.h"

#include "Renderers/Shared/IRenderLibrary.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"

using namespace Threading;

DECLARE_INTERNAL_SHADERS()

IShaderAPI*				g_pShaderAPI = NULL;

// register material system
static CMaterialSystem s_matsystem;
IMaterialSystem* materials = &s_matsystem;

ConVar				r_showlightmaps("r_showlightmaps", "0", "Disable diffuse textures to show lighting", CV_CHEAT);
ConVar				r_screen("r_screen", "0", "Screen count", CV_ARCHIVE);
ConVar				r_loadmiplevel("r_loadmiplevel", "0", 0, 3, "Mipmap level to load, needs texture reloading", CV_ARCHIVE );
ConVar				r_anisotropic("r_anisotropic", "4", 1, 16, "Mipmap anisotropic filtering quality, needs texture reloading", CV_ARCHIVE );

ConVar				r_lightscale("r_lightscale", "1.0f", "Global light scale", CV_ARCHIVE);
ConVar				r_shaderCompilerShowLogs("r_shaderCompilerShowLogs", "0","Show warnings of shader compilation",CV_ARCHIVE);

ConVar				r_overdraw("r_overdraw", "0", "Renders all materials in overdraw shader", CV_CHEAT);
ConVar				r_wireframe("r_wireframe","0","Enables wireframe rendering", CV_CHEAT);

ConVar				r_noffp("r_noffp","0","No FFP emulated primitives", CV_CHEAT);

ConVar				r_depthBias("r_depthBias", "-0.000001", NULL, CV_CHEAT);
ConVar				r_slopeDepthBias("r_slopeDepthBias", "-1.5", NULL, CV_CHEAT);

DECLARE_CMD(mat_reload, "Reloads all materials",0)
{
	MsgInfo("Reloading materials...\n\n");
	materials->ReloadAllMaterials();
	materials->Wait();
}

DECLARE_CMD(mat_print, "Print MatSystem info and loaded material list",0)
{
	materials->PrintLoadedMaterials();
}

DECLARE_CMD(mat_releaseStates, "Releases all render states",0)
{
	((CMaterialSystem*)materials)->ClearRenderStates();
}

DECLARE_CMD(mat_releaseUnused, "Releases unused materials",0)
{
	((CMaterialSystem*)materials)->ReleaseUnusedMaterials();
}

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

			if (material)
			{
				// load this material
				((CMaterial*)material)->DoLoadShaderAndTextures();
			}
		}

		// run thread code here
		return 0;
	}

	void AddMaterial(IMaterial* pMaterial)
	{
		CScopedMutex m(m_Mutex);
		m_newMaterials.addUnique( pMaterial );
	}

	int GetCount() const
	{
		return m_newMaterials.numElem();
	}

protected:
	Array<IMaterial*>	m_newMaterials;

	CEqMutex			m_Mutex;
};

CEqMatSystemThreadedLoader g_threadedMaterialLoader;

//---------------------------------------------------------------------------

CMaterialSystem::CMaterialSystem()
{
	m_cullMode = CULL_BACK;
	m_shaderAPI = NULL;

	m_setMaterial = NULL;
	m_overdrawMaterial = NULL;

	m_frame = 0;
	m_paramOverrideMask = 0xFFFFFFFF;

	m_curentLightingModel = MATERIAL_LIGHT_UNLIT;

	m_renderLibrary = NULL;
	m_rendermodule = NULL;

	m_ambColor = color4_white;
	memset(&m_fogInfo,0,sizeof(m_fogInfo));

	m_skinningEnabled = false;
	m_instancingEnabled = false;

	for(int i = 0; i < 4; i++)
		m_matrices[i] = identity4();

	m_whiteTexture = NULL;
	m_pDefaultMaterial = NULL;
	m_currentEnvmapTexture = NULL;

	m_currentLight = NULL;

	m_deviceActiveState = true;

	m_preApplyCallback = NULL;

	m_proxyDeltaTime = 0.0f;
	m_proxyTimer.GetTime(true);

	// register when the DLL is connected
	GetCore()->RegisterInterface(MATSYSTEM_INTERFACE_VERSION, this);

}

CMaterialSystem::~CMaterialSystem()
{
	// unregister when DLL disconnects
	GetCore()->UnregisterInterface(MATSYSTEM_INTERFACE_VERSION);
}

// Initializes material system
bool CMaterialSystem::Init(const matsystem_init_config_t& config)
{
	Msg(" \n--------- MaterialSystem Init --------- \n");

	kvkeybase_t* matSystemSettings = GetCore()->GetConfig()->FindKeyBase("MaterialSystem");

	ASSERT(g_pShaderAPI == NULL);

	// store the configuration
	m_config = config.renderConfig;

#ifdef PLAT_ANDROID
	EqString rendererName = "libeqGLESRHI";
#else
	EqString rendererName = config.rendererName;

	if (!rendererName.Length())
	{
		// try using default from Eq config
		rendererName = matSystemSettings ? KV_GetValueString(matSystemSettings->FindKeyBase("Renderer"), 0, NULL) : "eqGLRHI";
	}
#endif // _WIN329

	if (g_cmdLine->FindArgument("-norender") != -1)
	{
		rendererName = "eqNullRHI";
	}

	// Load Shader API from here...
	m_rendermodule = g_fileSystem->LoadModule(rendererName);
	if(m_rendermodule)
	{
		m_renderLibrary = (IRenderLibrary*)GetCore()->GetInterface( RENDERER_INTERFACE_VERSION );
		if(!m_renderLibrary)
		{
			ErrorMsg("MatSystem Error: Failed to initialize rendering library using %s!!!", rendererName.ToCString());
			g_fileSystem->FreeModule(m_rendermodule);
			return false;
		}
	}
	else
	{
		ErrorMsg("MatSystem Error: Cannot load library '%s'!!!", rendererName.ToCString());
		return false;
	}

	{
		// regular materials
		m_materialsPath = config.materialsPath;
		if(!m_materialsPath.Length())
			m_materialsPath = matSystemSettings ? KV_GetValueString(matSystemSettings->FindKeyBase("MaterialsPath"), 0, NULL) : "materials/";

		m_materialsPath.Path_FixSlashes();

		if (m_materialsPath.ToCString()[m_materialsPath.Length() - 1] != CORRECT_PATH_SEPARATOR)
			m_materialsPath.Append(CORRECT_PATH_SEPARATOR);

		// sources
		m_materialsSRCPath = config.materialsSRCPath;
		if(!m_materialsSRCPath.Length())
			m_materialsSRCPath = matSystemSettings ? KV_GetValueString(matSystemSettings->FindKeyBase("MaterialsSRCPath"), 0, NULL) : "materialsSRC/";

		m_materialsSRCPath.Path_FixSlashes();

		if (m_materialsSRCPath.ToCString()[m_materialsSRCPath.Length() - 1] != CORRECT_PATH_SEPARATOR)
			m_materialsSRCPath.Append(CORRECT_PATH_SEPARATOR);
	}

	// render library initialization
	// collect all needed data for use in shaderapi initialization

	if( m_renderLibrary->InitCaps() )
	{
		if(!m_renderLibrary->InitAPI(config.shaderApiParams))
			return false;

		// we created all we've need
		m_shaderAPI = m_renderLibrary->GetRenderer();

		if(!m_shaderAPI)
		{
			ErrorMsg("Fatal error! Couldn't create ShaderAPI interface!\n\nPlease update or reinstall the game\n");
			return false;
		}

		// copy default path
		shaderAPIParams_t sapiParams = config.shaderApiParams;
		if (sapiParams.texturePath[0] == 0)
			sapiParams.texturePath = m_materialsPath.GetData();

		if (sapiParams.textureSRCPath[0] == 0)
			sapiParams.textureSRCPath = m_materialsSRCPath.GetData();

		// init new created shader api with this parameters
		m_shaderAPI->Init(sapiParams);

        // test to be sure that it was initialized correctly
		m_renderLibrary->BeginFrame();
		m_shaderAPI->Clear(true, true, false);
		m_renderLibrary->EndFrame();
	}
	else
		return false;

	g_pShaderAPI = m_shaderAPI;

	if(!m_dynamicMesh.Init( g_standardVertexFormatDesc, elementsOf(g_standardVertexFormatDesc)))
	{
		ErrorMsg("Couldn't init DynamicMesh!\n");
		return false;
	}

	// initialize some resources
	CreateWhiteTexture();

	InitStandardMaterialProxies();
	REGISTER_INTERNAL_SHADERS();

	return true;
}


void CMaterialSystem::Shutdown()
{
	if(m_renderLibrary && m_rendermodule && g_pShaderAPI)
	{
		Msg("MatSystem shutdown...\n");

		// shutdown thread first
		g_threadedMaterialLoader.StopThread(true);
		
		ClearRenderStates();
		m_dynamicMesh.Destroy();

		FreeMaterials();

		if(m_whiteTexture)
			g_pShaderAPI->FreeTexture(m_whiteTexture);

		if(m_luxelTestTexture)
			g_pShaderAPI->FreeTexture(m_luxelTestTexture);

		for(int i = 0; i < m_shaderOverrideList.numElem(); i++)
			delete [] m_shaderOverrideList[i].shadername;

		m_shaderOverrideList.clear();

		for(int i = 0; i < m_proxyFactoryList.numElem(); i++)
			delete [] m_proxyFactoryList[i].name;

		m_proxyFactoryList.clear();

		m_renderLibrary->ReleaseSwapChains();
		g_pShaderAPI->Shutdown();
		m_renderLibrary->ExitAPI();

		// shutdown render libraries, all shaders and other
		g_fileSystem->FreeModule( m_rendermodule );
	}
}

void CMaterialSystem::CreateWhiteTexture()
{
	// don't worry, it will be removed
	CImage* img = PPNew CImage();

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

	SamplerStateParam_t texSamplerParams = g_pShaderAPI->MakeSamplerState(TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,TEXADDRESS_CLAMP,TEXADDRESS_CLAMP);

	Array<CImage*> images;
	images.append(img);

	m_whiteTexture = g_pShaderAPI->CreateTexture(images, texSamplerParams, TEXFLAG_NOQUALITYLOD);

	if(m_whiteTexture)
		m_whiteTexture->Ref_Grab();

	images.clear();

	//--------------------------------------------------------------------
	/*
	// don't worry, it will be removed
	img = PPNew CImage();

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

	texSamplerParams = g_pShaderAPI->MakeSamplerState(TEXFILTER_NEAREST,TEXADDRESS_CLAMP,TEXADDRESS_CLAMP,TEXADDRESS_CLAMP);

	m_luxelTestTexture = g_pShaderAPI->CreateTexture(images, texSamplerParams, TEXFLAG_NOQUALITYLOD);

	if(m_luxelTestTexture)
		m_luxelTestTexture->Ref_Grab();
	*/
}

void CMaterialSystem::InitDefaultMaterial()
{
	if(!m_pDefaultMaterial)
	{
		kvkeybase_t defaultParams;
		defaultParams.SetName("Default"); // set shader 'Default'
		defaultParams.SetKey("BaseTexture", "$basetexture");

		CMaterial* pMaterial = (CMaterial*)CreateMaterial("_default", &defaultParams);
		pMaterial->Ref_Grab();
		pMaterial->LoadShaderAndTextures();

		m_pDefaultMaterial = pMaterial;
	}

	if(!m_overdrawMaterial)
	{
		kvkeybase_t overdrawParams;
		overdrawParams.SetName("BaseUnlit"); // set shader 'BaseUnlit'
		overdrawParams.SetKey("BaseTexture", "_matsys_white");
		overdrawParams.SetKey("Color", "[0.045 0.02 0.02 1.0]");
		overdrawParams.SetKey("Additive", "1");
		overdrawParams.SetKey("ztest", "0");
		overdrawParams.SetKey("zwrite", "0");
		

		CMaterial* pMaterial = (CMaterial*)CreateMaterial("_overdraw", &overdrawParams);
		pMaterial->Ref_Grab();
		pMaterial->LoadShaderAndTextures();

		m_overdrawMaterial = pMaterial;
	}
}

bool CMaterialSystem::IsInStubMode()
{
	return m_config.stubMode;
}

matsystem_render_config_t& CMaterialSystem::GetConfiguration()
{
	return m_config;
}

// returns material path
const char* CMaterialSystem::GetMaterialPath() const
{
	return m_materialsPath.ToCString();
}

const char* CMaterialSystem::GetMaterialSRCPath() const
{
	return m_materialsSRCPath.ToCString();
}

// Adds new shader library
bool CMaterialSystem::LoadShaderLibrary(const char* libname)
{
	DKMODULE* shaderlib = g_fileSystem->LoadModule( libname );

	if(!shaderlib)
		return false;

	typedef int (*InitShaderLibraryFunction)(IMaterialSystem* pMatSystem);

	InitShaderLibraryFunction functionPtr = (InitShaderLibraryFunction)g_fileSystem->GetProcedureAddress(shaderlib, "InitShaderLibrary");

	if(!functionPtr)
	{
		MsgError("Invalid shader library '%s' - can't get InitShaderLibrary function!\n",libname);
		return false;
	}

	(functionPtr)(this);

	m_shaderLibs.append(shaderlib);

	return true;
}

//------------------------------------------------------------------------------------------------

bool CMaterialSystem::IsMaterialExist(const char* szMaterialName)
{
	EqString mat_path(m_materialsPath + szMaterialName + _Es(".mat"));

	return g_fileSystem->FileExist(mat_path.GetData());
}

IMaterial* CMaterialSystem::CreateMaterial(const char* szMaterialName, kvkeybase_t* params)
{
	// must have names
	ASSERT(strlen(szMaterialName) > 0);
	const int nameHash = StringToHash(szMaterialName, true);

	return CreateMaterialInternal(szMaterialName, nameHash, params);
}

// creates new material with defined parameters
IMaterial* CMaterialSystem::CreateMaterialInternal(const char* szMaterialName, int nameHash, kvkeybase_t* params)
{
	CScopedMutex m(m_Mutex);

	// create new material
	CMaterial* pMaterial = PPNew CMaterial(m_ProxyMutex[m_loadedMaterials.size() % 4]);

	// add to list
	ASSERTMSG(m_loadedMaterials.find(nameHash) == m_loadedMaterials.end(), "Material %s was already created under that name", szMaterialName);
	m_loadedMaterials.insert(nameHash, pMaterial);

	// if no params, we can load it a usual way
	if(params)
		pMaterial->Init(szMaterialName, params);
	else
		pMaterial->Init(szMaterialName);

	return pMaterial;
}

IMaterial* CMaterialSystem::GetMaterial(const char* szMaterialName)
{
	// Don't load null materials
	if( strlen(szMaterialName) == 0 )
		return NULL;

	EqString materialName = szMaterialName;
	materialName = materialName.LowerCase();
	materialName.Path_FixSlashes();

	if (materialName.ToCString()[0] == CORRECT_PATH_SEPARATOR)
		materialName = materialName.ToCString() + 1;

	const int nameHash = StringToHash(materialName.ToCString(), true);

	// find the material with existing name
	CScopedMutex m(m_Mutex);

	auto it = m_loadedMaterials.find(nameHash);
	if (it != m_loadedMaterials.end())
		return *it;

	return CreateMaterialInternal(materialName.ToCString(), nameHash, nullptr);
}

// If we have unliaded material, just load it
void CMaterialSystem::PreloadNewMaterials()
{
	if(m_config.stubMode)
		return;

	{
		CScopedMutex m(m_Mutex);
	
		for (auto it = m_loadedMaterials.begin(); it != m_loadedMaterials.end(); ++it)
		{
			if(it.value()->GetState() != MATERIAL_LOAD_NEED_LOAD)
				continue;

			PutMaterialToLoadingQueue(it.value());
		}
	}
}

// releases non-used materials
void CMaterialSystem::ReleaseUnusedMaterials()
{
	CScopedMutex m(m_Mutex);

	for (auto it = m_loadedMaterials.begin(); it != m_loadedMaterials.end(); ++it)
	{
		CMaterial* material = (CMaterial*)*it;

		// don't unload default material
		if(!stricmp(material->GetName(), "Default"))
			continue;

		int framesDiff = (material->m_frameBound - m_frame);

		// Flush materials
		if(framesDiff >= m_config.flushThresh - 1)
			material->Cleanup(false, true);
	}
}

// Reloads materials
void CMaterialSystem::ReloadAllMaterials()
{
	CScopedMutex m(m_Mutex);

	Array<IMaterial*> loadingList;

	for (auto it = m_loadedMaterials.begin(); it != m_loadedMaterials.end(); ++it)
	{
		CMaterial* material = (CMaterial*)*it;

		// don't unload default material
		if(!stricmp(material->GetName(), "Default"))
			continue;

		// don't drop variables, just reload shader
		material->Cleanup(false, true);

		// don't load materials which are not from disk
		if(!material->m_loadFromDisk)
		{
			material->InitShader();
			continue;
		}

		material->Init( NULL );

		const int framesDiff = (material->m_frameBound - m_frame);

		// preload material if it was ever used before
		if(framesDiff >= -1)
			loadingList.append(material);
	}

	// issue loading after all materials were freed
	// - this is a guarantee to shader recompilation
	for(int i = 0; i < loadingList.numElem(); i++)
		PutMaterialToLoadingQueue( loadingList[i] );
}

// frees all materials
void CMaterialSystem::FreeMaterials()
{
	CScopedMutex m(m_Mutex);

	for (auto it = m_loadedMaterials.begin(); it != m_loadedMaterials.end(); ++it)
	{
		CMaterial* pMaterial = (CMaterial*)*it;
		DevMsg(DEVMSG_MATSYSTEM, "freeing material %s\n", pMaterial->GetName());
		pMaterial->Cleanup();
		delete pMaterial;
	}

	m_loadedMaterials.clear();
}

void CMaterialSystem::ClearRenderStates()
{
	for (auto i = m_blendStates.begin(); i != m_blendStates.end(); ++i)
		g_pShaderAPI->DestroyRenderState(*i);
	m_blendStates.clear();
	
	for (auto i = m_depthStates.begin(); i != m_depthStates.end(); ++i)
		g_pShaderAPI->DestroyRenderState(*i);
	m_depthStates.clear();
	
	for (auto i = m_rasterStates.begin(); i != m_rasterStates.end(); ++i)
		g_pShaderAPI->DestroyRenderState(*i);
	m_rasterStates.clear();
}

// frees single material
void CMaterialSystem::FreeMaterial(IMaterial *pMaterial)
{
	if(pMaterial == NULL)
		return;

	CScopedMutex m(m_Mutex);

	pMaterial->Ref_Drop();

	if(pMaterial->Ref_Count() <= 0)
	{
		CMaterial* material = (CMaterial*)pMaterial;

		auto it = m_loadedMaterials.find(material->m_nameHash);
		if(it != m_loadedMaterials.end())
		{
			DevMsg(DEVMSG_MATSYSTEM,"freeing %s\n", material->GetName());
			material->Cleanup();
			delete material;

			m_loadedMaterials.remove(it);
		}
	}
}

void CMaterialSystem::RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName)
{
	proxyfactory_t factory;
	factory.name = xstrdup(pszName);
	factory.disp = dispfunc;

	m_proxyFactoryList.append(factory);
}

IMaterialProxy* CMaterialSystem::CreateProxyByName(const char* pszName)
{
	for(int i = 0; i < m_proxyFactoryList.numElem(); i++)
	{
		if(!stricmp(m_proxyFactoryList[i].name, pszName))
		{
			return (m_proxyFactoryList[i].disp)();
		}
	}

	return NULL;
}

void CMaterialSystem::RegisterShader(const char* pszShaderName, DISPATCH_CREATE_SHADER dispatcher_creation)
{
	for(int i = 0; i < m_shaderFactoryList.numElem(); i++)
	{
		if(!stricmp(m_shaderFactoryList[i].shader_name,pszShaderName))
		{
			ErrorMsg("Programming Error! The shader '%s' is already exist!\n",pszShaderName);
			exit(-1);
		}
	}

	DevMsg(DEVMSG_MATSYSTEM, "Registering shader '%s'\n", pszShaderName);

	shaderfactory_t newShader;

	newShader.dispatcher = dispatcher_creation;
	newShader.shader_name = (char*)pszShaderName;

	m_shaderFactoryList.append(newShader);
}

// registers overrider for shaders
void CMaterialSystem::RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function)
{
	shaderoverride_t new_override;
	new_override.shadername = xstrdup(shaderName);
	new_override.function = check_function;

	// this is a higher priority
	m_shaderOverrideList.insert(new_override, 0);
}

IMaterialSystemShader* CMaterialSystem::CreateShaderInstance(const char* szShaderName)
{
	EqString shaderName( szShaderName );

	// check the override table
	for(int i = 0; i < m_shaderOverrideList.numElem(); i++)
	{
		if(!stricmp(szShaderName,m_shaderOverrideList[i].shadername))
		{
			shaderName = m_shaderOverrideList[i].function();

			if(shaderName.Length() > 0) // only if we have shader name
				break;
		}
	}

	// now find the factory and dispatch
	for(int i = 0; i < m_shaderFactoryList.numElem(); i++)
	{
		if(!shaderName.CompareCaseIns( m_shaderFactoryList[i].shader_name ))
			return (m_shaderFactoryList[i].dispatcher)();
	}

	return NULL;
}

//------------------------------------------------------------------------------------------------

typedef bool (*PFNMATERIALBINDCALLBACK)(IMaterial* pMaterial, uint paramMask);

void BindFFPMaterial(IMaterial* pMaterial, int paramMask)
{
	g_pShaderAPI->SetShader(NULL);

	g_pShaderAPI->SetBlendingState(NULL);
	g_pShaderAPI->SetRasterizerState(NULL);
	g_pShaderAPI->SetDepthStencilState(NULL);
}

bool Callback_BindErrorTextureFFPMaterial(IMaterial* pMaterial, uint paramMask)
{
	BindFFPMaterial(pMaterial, paramMask);
	g_pShaderAPI->SetTexture(g_pShaderAPI->GetErrorTexture());

	return false;
}

bool Callback_BindFFPMaterial(IMaterial* pMaterial, uint paramMask)
{
	BindFFPMaterial(pMaterial, paramMask);

	// bind same, but with base texture
	g_pShaderAPI->SetTexture(pMaterial->GetBaseTexture());

	return false;
}

bool Callback_BindNormalMaterial(IMaterial* pMaterial, uint paramMask)
{
	((CMaterial*)pMaterial)->Setup(paramMask);

	return true;
}

PFNMATERIALBINDCALLBACK materialstate_callbacks[] =
{
	Callback_BindErrorTextureFFPMaterial,	// binds ffp shader with error texture, error shader only
	Callback_BindFFPMaterial,				// binds ffp shader with material textures, only FFP mode
	Callback_BindNormalMaterial				// binds normal material.
};

enum EMaterialRenderSubroutine
{
	MATERIAL_SUBROUTINE_ERROR = 0,
	MATERIAL_SUBROUTINE_FFPMATERIAL,
	MATERIAL_SUBROUTINE_NORMAL,
};

// loads material or sends it to loader thread
void CMaterialSystem::PutMaterialToLoadingQueue(IMaterial* pMaterial)
{
	if(pMaterial->GetState() != MATERIAL_LOAD_NEED_LOAD)
		return;

	CMaterial* material = (CMaterial*)pMaterial;
	{
		CScopedMutex m(m_Mutex);
		material->m_state = MATERIAL_LOAD_INQUEUE;
	}

	if( m_config.threadedloader )
	{
		g_threadedMaterialLoader.AddMaterial(pMaterial);

		if(!g_threadedMaterialLoader.IsRunning())
			g_threadedMaterialLoader.StartWorkerThread("matSystemLoader");
	}
	else
	{
		material->DoLoadShaderAndTextures();
	}
}

int CMaterialSystem::GetLoadingQueue() const
{
	return g_threadedMaterialLoader.GetCount();
}

void CMaterialSystem::SetShaderParameterOverriden(ShaderDefaultParams_e param, bool set)
{
	if(set)
		m_paramOverrideMask &= ~(1 << (uint)param);
	else
		m_paramOverrideMask |= (1 << (uint)param);
}

void CMaterialSystem::SetProxyDeltaTime(float deltaTime)
{
	m_proxyDeltaTime = deltaTime;
}

bool CMaterialSystem::BindMaterial(IMaterial* pMaterial, int flags)
{
	if(!pMaterial)
	{
		InitDefaultMaterial();
		pMaterial = m_pDefaultMaterial;
	}

	CMaterial* pSetupMaterial = (CMaterial*)pMaterial;

	// proxy update is dirty if material was not bound to this frame
	if (pSetupMaterial->m_frameBound != m_frame)
		pSetupMaterial->UpdateProxy(m_proxyDeltaTime);

	pSetupMaterial->m_frameBound = m_frame;

	// it's now a more critical section to the material
	PutMaterialToLoadingQueue( pMaterial );

	// set the current material
	IMaterial* setMaterial = pMaterial;

	// try overriding material
	if (m_preApplyCallback)
		setMaterial = m_preApplyCallback->OnPreBindMaterial(setMaterial);
	
	EMaterialRenderSubroutine subRoutineId = MATERIAL_SUBROUTINE_NORMAL;

	if(m_config.ffpMode)
		subRoutineId = MATERIAL_SUBROUTINE_FFPMATERIAL;

	if( pMaterial->GetState() == MATERIAL_LOAD_ERROR || pMaterial->GetState() == MATERIAL_LOAD_NEED_LOAD )
		subRoutineId = MATERIAL_SUBROUTINE_ERROR;

	// if material is still loading, use the default material for a while
	if( pMaterial->GetState() == MATERIAL_LOAD_INQUEUE )
	{
		InitDefaultMaterial();
		setMaterial = m_pDefaultMaterial;
		subRoutineId = MATERIAL_SUBROUTINE_NORMAL;
	}

	bool success;

	if( m_config.overdrawMode )
	{
		InitDefaultMaterial();
		materials->SetAmbientColor(ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f));
		success = (*materialstate_callbacks[subRoutineId])(m_overdrawMaterial, 0xFFFFFFFF);
	}
	else
		success = (*materialstate_callbacks[subRoutineId])(setMaterial, m_paramOverrideMask);

	m_setMaterial = setMaterial;

	if (!(flags & MATERIAL_BIND_KEEPOVERRIDE))
		m_paramOverrideMask = 0xFFFFFFFF; // reset override mask shortly after we bind material

	if(flags & MATERIAL_BIND_PREAPPLY)
		Apply();

	return success;
}

// Applies current material
void CMaterialSystem::Apply()
{
	IMaterial* setMaterial = m_setMaterial;

	if(!setMaterial)
	{
		g_pShaderAPI->Apply();
		return;
	}

	// callback before applying
	if(m_preApplyCallback)
		m_preApplyCallback->OnPreApplyMaterial(setMaterial);

	g_pShaderAPI->Apply();
}

// returns bound material
IMaterial* CMaterialSystem::GetBoundMaterial()
{
	return m_setMaterial;
}

// waits for material loader thread is finished
void CMaterialSystem::Wait()
{
	if (m_config.threadedloader)
		g_threadedMaterialLoader.WaitForThread();
}

// transform operations
// sets up a matrix, projection, view, and world
void CMaterialSystem::SetMatrix(ER_MatrixMode mode, const Matrix4x4 &matrix)
{
	m_matrices[(int)mode] = matrix;

	g_pShaderAPI->SetMatrixMode(mode);

	g_pShaderAPI->PopMatrix();
	g_pShaderAPI->LoadMatrix(matrix);
	g_pShaderAPI->PushMatrix();
}

// returns a typed matrix
void CMaterialSystem::GetMatrix(ER_MatrixMode mode, Matrix4x4 &matrix)
{
	matrix = m_matrices[(int)mode];
}

// retunrs multiplied matrix
void CMaterialSystem::GetWorldViewProjection(Matrix4x4 &matrix)
{
	matrix = identity4() * m_matrices[MATRIXMODE_PROJECTION] * (m_matrices[MATRIXMODE_VIEW] * (m_matrices[MATRIXMODE_WORLD2] * m_matrices[MATRIXMODE_WORLD]));
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
	m_currentLight = pLight;
}

dlight_t* CMaterialSystem::GetLight()
{
	return m_currentLight;
}

IMaterial* CMaterialSystem::GetDefaultMaterial() const
{
	return m_pDefaultMaterial;
}

ITexture* CMaterialSystem::GetWhiteTexture() const
{
	return m_whiteTexture;
}

ITexture* CMaterialSystem::GetLuxelTestTexture() const
{
	return m_luxelTestTexture;
}

//-----------------------------
// Frame operations
//-----------------------------

// tells 3d device to begin frame
bool CMaterialSystem::BeginFrame()
{
	if(m_config.stubMode || !m_shaderAPI)
		return false;

	bool state, oldState = m_deviceActiveState;
	m_deviceActiveState = state = g_pShaderAPI->IsDeviceActive();

	if(!state && state != oldState)
	{
		for(int i = 0; i < m_lostDeviceCb.numElem(); i++)
		{
			if(!m_lostDeviceCb[i]())
				return false;
		}
	}

	if(g_threadedMaterialLoader.GetCount())
		g_threadedMaterialLoader.SignalWork();

	m_renderLibrary->BeginFrame();

	if(state && state != oldState)
	{
		for(int i = 0; i < m_lostDeviceCb.numElem(); i++)
		{
			if(!m_restoreDeviceCb[i]())
				return false;
		}
	}

	if(m_config.overdrawMode)
		g_pShaderAPI->Clear(true, false, false, ColorRGBA(0, 0, 0, 0));

	return true;
}

// tells 3d device to end and present frame
bool CMaterialSystem::EndFrame(IEqSwapChain* swapChain)
{
	if(m_renderLibrary)
		m_renderLibrary->EndFrame(swapChain);

	m_frame++;
	m_proxyDeltaTime = m_proxyTimer.GetTime(true);

	return true;
}

// captures screenshot to CImage data
bool CMaterialSystem::CaptureScreenshot(CImage &img)
{
	return m_renderLibrary->CaptureScreenshot( img );
}

// resizes device back buffer. Must be called if window resized
void CMaterialSystem::SetDeviceBackbufferSize(int wide, int tall)
{
	if(m_renderLibrary)
		m_renderLibrary->SetBackbufferSize(wide, tall);
}

// reports device focus mode
void CMaterialSystem::SetDeviceFocused(bool inFocus)
{
	if (m_renderLibrary)
		m_renderLibrary->SetFocused(inFocus);
}

IEqSwapChain* CMaterialSystem::CreateSwapChain(void* windowHandle)
{
	if(m_renderLibrary)
		return m_renderLibrary->CreateSwapChain(windowHandle);

	return NULL;
}

void CMaterialSystem::DestroySwapChain(IEqSwapChain* swapChain)
{
	if(m_renderLibrary)
		m_renderLibrary->DestroySwapChain(swapChain);
}

// fullscreen mode changing
bool CMaterialSystem::SetWindowed(bool enable)
{
	bool changeMode = (m_renderLibrary->IsWindowed() != enable);

	if(!changeMode)
		return true;

	for (int i = 0; i < m_lostDeviceCb.numElem(); i++)
	{
		if (!m_lostDeviceCb[i]())
			return false;
	}

	bool result = true;

	if (m_renderLibrary)
		result = m_renderLibrary->SetWindowed(enable);

	for (int i = 0; i < m_lostDeviceCb.numElem(); i++)
	{
		if (!m_restoreDeviceCb[i]())
			return false;
	}

	return result;
}

bool CMaterialSystem::IsWindowed() const
{
	return m_renderLibrary->IsWindowed();
}

// returns RHI device interface
IShaderAPI* CMaterialSystem::GetShaderAPI()
{
	return m_shaderAPI;
}

//--------------------------------------------------------------------------------------
// Shader dynamic states

// sets a fog info
void CMaterialSystem::SetFogInfo(const FogInfo_t &info)
{
	m_fogInfo = info;
}

// returns fog info
void CMaterialSystem::GetFogInfo(FogInfo_t &info)
{
	if( m_config.overdrawMode)
	{
		static FogInfo_t nofog;

		info = nofog;

		return;
	}

	info = m_fogInfo;
}

// sets current lighting model as state
void CMaterialSystem::SetCurrentLightingModel(EMaterialLightingMode lightingModel)
{
	m_curentLightingModel = lightingModel;
}

EMaterialLightingMode CMaterialSystem::GetCurrentLightingModel()
{
	return m_curentLightingModel;
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
	if (m_currentEnvmapTexture)
		m_currentEnvmapTexture->Ref_Drop();

	m_currentEnvmapTexture = pEnvMapTexture;

	if (m_currentEnvmapTexture)
		m_currentEnvmapTexture->Ref_Grab();
}

// returns current pre-apply callback
ITexture* CMaterialSystem::GetEnvironmentMapTexture()
{
	return m_currentEnvmapTexture;
}

ER_CullMode CMaterialSystem::GetCurrentCullMode()
{
	return m_cullMode;
}

void CMaterialSystem::SetCullMode(ER_CullMode cullMode)
{
	m_cullMode = cullMode;
}

// skinning mode
void CMaterialSystem::SetSkinningEnabled( bool bEnable )
{
	m_skinningEnabled = bEnable;
}

bool CMaterialSystem::IsSkinningEnabled()
{
	return m_skinningEnabled;
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

// sets up a 2D mode
void CMaterialSystem::Setup2D(float wide, float tall)
{
	SetMatrix(MATRIXMODE_PROJECTION, projection2DScreen(wide,tall));
	SetMatrix(MATRIXMODE_VIEW, identity4());
	SetMatrix(MATRIXMODE_WORLD, identity4());
	SetMatrix(MATRIXMODE_WORLD2, identity4());
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

IDynamicMesh* CMaterialSystem::GetDynamicMesh() const
{
	return (IDynamicMesh*)&m_dynamicMesh;
}

// draws 2D primitives
void CMaterialSystem::DrawPrimitivesFFP(ER_PrimitiveType type, Vertex3D_t *pVerts, int nVerts,
										ITexture* pTexture, const ColorRGBA &color,
										BlendStateParam_t* blendParams, DepthStencilStateParams_t* depthParams,
										RasterizerStateParams_t* rasterParams)
{
	if(r_noffp.GetBool())
		return;

	if(!blendParams)
		SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA, BLENDFUNC_ADD);
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

	g_pShaderAPI->SetTexture(pTexture, NULL, 0);
	BindMaterial(GetDefaultMaterial());

	CMeshBuilder meshBuilder(&m_dynamicMesh);

	meshBuilder.Begin(type);

	for(int i = 0; i < nVerts; i++)
	{
		meshBuilder.Color4fv(pVerts[i].color * color);
		meshBuilder.TexCoord2fv(pVerts[i].texCoord);
		meshBuilder.Position3fv(pVerts[i].position);

		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
}

void CMaterialSystem::DrawPrimitives2DFFP(	ER_PrimitiveType type, Vertex2D_t *pVerts, int nVerts,
											ITexture* pTexture, const ColorRGBA &color,
											BlendStateParam_t* blendParams, DepthStencilStateParams_t* depthParams,
											RasterizerStateParams_t* rasterParams)
{
	if(r_noffp.GetBool())
		return;

	if(!blendParams)
		SetBlendingStates(BLENDFACTOR_ONE, BLENDFACTOR_ZERO, BLENDFUNC_ADD);
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

	g_pShaderAPI->SetTexture(pTexture, NULL, 0);
	BindMaterial(GetDefaultMaterial());

	CMeshBuilder meshBuilder(&m_dynamicMesh);

	meshBuilder.Begin(type);

	for(int i = 0; i < nVerts; i++)
	{
		meshBuilder.Color4fv(pVerts[i].color * color);
		meshBuilder.TexCoord2f(pVerts[i].texCoord.x,pVerts[i].texCoord.y);
		meshBuilder.Position3f(pVerts[i].position.x, pVerts[i].position.y, 0);

		meshBuilder.AdvanceVertex();
	}

	meshBuilder.End();
}


//-----------------------------
// RHI render states setup
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
	SetRasterizerStates(raster.cullMode, raster.fillMode, raster.multiSample, raster.scissor, raster.useDepthBias);
}

// pack blending function to ushort
struct blendStateIndex_t
{
	blendStateIndex_t( ER_BlendFactor nSrcFactor, ER_BlendFactor nDestFactor, ER_BlendFunction nBlendingFunc, int colormask )
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
void CMaterialSystem::SetBlendingStates(ER_BlendFactor nSrcFactor, ER_BlendFactor nDestFactor, ER_BlendFunction nBlendingFunc, int colormask)
{
	blendStateIndex_t idx(nSrcFactor, nDestFactor, nBlendingFunc, colormask);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	auto blendState = m_blendStates.find(stateIndex);

	if(blendState == m_blendStates.end())
	{
		BlendStateParam_t desc;
		// no sense to enable blending when no visual effects...
		desc.blendEnable = !(nSrcFactor == BLENDFACTOR_ONE && nDestFactor == BLENDFACTOR_ZERO && nBlendingFunc == BLENDFUNC_ADD);
		desc.srcFactor = nSrcFactor;
		desc.dstFactor = nDestFactor;
		desc.blendFunc = nBlendingFunc;
		desc.mask = colormask;

		state = g_pShaderAPI->CreateBlendingState(desc);
		m_blendStates.insert(stateIndex, state);
	}
	else
		state = *blendState;

	g_pShaderAPI->SetBlendingState( state );
}

// pack depth states to ubyte
struct depthStateIndex_t
{
	depthStateIndex_t( bool bDoDepthTest, bool bDoDepthWrite, ER_CompareFunc depthCompFunc )
		: doDepthTest(bDoDepthTest), doDepthWrite(bDoDepthWrite), compFunc(depthCompFunc)
	{
	}

	ubyte doDepthTest : 1;
	ubyte doDepthWrite : 1;
	ubyte compFunc : 3;
};

assert_sizeof(depthStateIndex_t,1);

// sets depth stencil state
void CMaterialSystem::SetDepthStates(bool bDoDepthTest, bool bDoDepthWrite, ER_CompareFunc depthCompFunc)
{
	depthStateIndex_t idx(bDoDepthTest, bDoDepthWrite, depthCompFunc);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	auto depthState = m_depthStates.find(stateIndex);

	if(depthState == m_depthStates.end())
	{
		DepthStencilStateParams_t desc;
		desc.depthWrite = bDoDepthWrite;
		desc.depthTest = bDoDepthTest;
		desc.depthFunc = depthCompFunc;
		desc.doStencilTest = false;

		state = g_pShaderAPI->CreateDepthStencilState(desc);
		m_depthStates.insert(stateIndex, state);
	}
	else
		state = *depthState;

	g_pShaderAPI->SetDepthStencilState( state );
}

// pack blending function to ushort
struct rasterStateIndex_t
{
	rasterStateIndex_t( ER_CullMode nCullMode, ER_FillMode nFillMode, bool bMultiSample,bool bScissor, bool bPolyOffset )
		: cullMode(nCullMode), fillMode(nFillMode), multisample(bMultiSample), scissor(bScissor), polyoffset(bPolyOffset)
	{
	}

	ubyte cullMode : 2;
	ubyte fillMode : 2;
	ubyte multisample : 1;
	ubyte scissor : 1;
	ubyte polyoffset: 1;
};

assert_sizeof(rasterStateIndex_t,1);

// sets rasterizer extended mode
void CMaterialSystem::SetRasterizerStates(ER_CullMode nCullMode, ER_FillMode nFillMode,bool bMultiSample,bool bScissor,bool bPolyOffset)
{
	rasterStateIndex_t idx(nCullMode, nFillMode, bMultiSample, bScissor, bPolyOffset);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	auto rasterState = m_rasterStates.find(stateIndex);

	if(rasterState == m_rasterStates.end())
	{
		RasterizerStateParams_t desc;
		desc.cullMode = nCullMode;
		desc.fillMode = nFillMode;
		desc.multiSample = bMultiSample;
		desc.scissor = bScissor;
		desc.useDepthBias = bPolyOffset;

		if(desc.useDepthBias)
		{
			desc.depthBias = r_depthBias.GetFloat();
			desc.slopeDepthBias = r_slopeDepthBias.GetFloat();
		}

		state = g_pShaderAPI->CreateRasterizerState(desc);
		m_rasterStates.insert(stateIndex, state);
	}
	else
		state = *rasterState;

	g_pShaderAPI->SetRasterizerState( state );
}

// use this if you have objects that must be destroyed when device is lost
void CMaterialSystem::AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore)
{
	m_lostDeviceCb.addUnique(destroy);
	m_restoreDeviceCb.addUnique(restore);
}

// prints loaded materials to console
void CMaterialSystem::PrintLoadedMaterials()
{
	CScopedMutex m(m_Mutex);

	Msg("*** Material list begin ***\n");
	for (auto it = m_loadedMaterials.begin(); it != m_loadedMaterials.end(); ++it)
	{
		CMaterial* material = (CMaterial*)*it;

		MsgInfo("%s - %s (%d refs) %s\n", material->GetShaderName(), material->GetName(), material->Ref_Count(), (material->m_state == MATERIAL_LOAD_NEED_LOAD) ? "(not loaded)" : "");
	}
	Msg("Total loaded materials: %d\n", m_loadedMaterials.size());
}

// removes callbacks from list
void CMaterialSystem::RemoveLostRestoreCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore)
{
	m_lostDeviceCb.remove(destroy);
	m_restoreDeviceCb.remove(restore);
}
