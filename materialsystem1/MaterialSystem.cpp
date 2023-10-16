//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material sub-rendering system
//
// Features: - Cached texture loading
//			 - Material management
//			 - Shader management
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/IEqParallelJobs.h"
#include "core/ICommandLine.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "utils/KeyValues.h"
#include "MaterialSystem.h"
#include "imaging/ImageLoader.h"
#include "imaging/PixWriter.h"
#include "material.h"
#include "materialvar.h"
#include "MaterialProxy.h"
#include "TextureLoader.h"
#include "Renderers/IRenderLibrary.h"
#include "materialsystem1/MeshBuilder.h"
#include "materialsystem1/IMaterialCallback.h"


using namespace Threading;
CEqMutex s_matSystemMutex;

DECLARE_INTERNAL_SHADERS()

IShaderAPI* g_renderAPI = nullptr;

// register material system
static CMaterialSystem s_matsystem;
IMaterialSystem* g_matSystem = &s_matsystem;

// standard vertex format used by the material system's dynamic mesh instance
static VertexFormatDesc g_dynMeshVertexFormatDesc[] = {
	{0, 4, VERTEXATTRIB_POSITION,	ATTRIBUTEFORMAT_FLOAT, "position"},
	{0, 4, VERTEXATTRIB_TEXCOORD,	ATTRIBUTEFORMAT_HALF, "texcoord"},
	{0, 4, VERTEXATTRIB_NORMAL,		ATTRIBUTEFORMAT_HALF, "normal"},
	{0, 4, VERTEXATTRIB_COLOR,		ATTRIBUTEFORMAT_UBYTE, "color"},
};

DECLARE_CVAR(r_screen, "0", "Screen count", CV_ARCHIVE);
DECLARE_CVAR(r_clear, "0", "Clear the backbuffer", CV_ARCHIVE);
DECLARE_CVAR(r_lightscale, "1.0f", "Global light scale", CV_ARCHIVE);
DECLARE_CVAR(r_shaderCompilerShowLogs, "0","Show warnings of shader compilation",CV_ARCHIVE);

DECLARE_CVAR_CLAMP(r_loadmiplevel, "0", 0, 3, "Mipmap level to load, needs texture reloading", CV_ARCHIVE);
DECLARE_CVAR_CLAMP(r_anisotropic, "4", 1, 16, "Mipmap anisotropic filtering quality, needs texture reloading", CV_ARCHIVE);

DECLARE_CVAR(r_depthBias, "-0.000001", nullptr, CV_CHEAT);
DECLARE_CVAR(r_slopeDepthBias, "-1.5", nullptr, CV_CHEAT);

DECLARE_CMD(mat_reload, "Reloads all materials",0)
{
	s_matsystem.ReloadAllMaterials();
	s_matsystem.WaitAllMaterialsLoaded();
}

DECLARE_CMD(mat_print, "Print MatSystem info and loaded material list",0)
{
	s_matsystem.PrintLoadedMaterials();
}

DECLARE_CMD(mat_releaseStates, "Releases all render states",0)
{
	s_matsystem.ClearRenderStates();
}

DECLARE_CMD(mat_releaseUnused, "Releases unused materials",0)
{
	s_matsystem.ReleaseUnusedMaterials();
}

//
// Threaded material loader
//

class CEqMatSystemThreadedLoader : public CEqThread
{
public:
	virtual int Run()
	{
		IMaterial* nextMaterial = nullptr;
		{
			CScopedMutex m(s_matSystemMutex);
			auto it = m_newMaterials.begin();
			if (!it.atEnd())
			{
				nextMaterial = it.key();
				m_newMaterials.remove(it);
			}
		}

		if (!nextMaterial)
			return 0;

		if (nextMaterial->Ref_Drop())
		{
			MsgWarning("Material %x is freed before loading\n", nextMaterial);
			return 0;
		}

		// load this material
		// BUG: may be null
		((CMaterial*)nextMaterial)->DoLoadShaderAndTextures();

		if(m_newMaterials.size())
		{
			SignalWork();
		}

		return 0;
	}

	void AddMaterial(IMaterialPtr pMaterial)
	{
		if (g_parallelJobs->IsInitialized() && g_renderAPI->GetProgressiveTextureFrequency() == 0)
		{
			// Wooohoo Blast Processing!
			g_parallelJobs->AddJob(JOB_TYPE_ANY, [pMaterial](void*, int) {
				((CMaterial*)pMaterial.Ptr())->DoLoadShaderAndTextures();
			});

			g_parallelJobs->Submit();
			return;
		}

		{
			CScopedMutex m(s_matSystemMutex);
			if (m_newMaterials.contains(pMaterial))
				return;

			pMaterial->Ref_Grab();
			m_newMaterials.insert(pMaterial);
		}

		if (!IsRunning())
			StartWorkerThread("matSystemLoader");

		SignalWork();
	}

	int GetCount() const
	{
		return m_newMaterials.size();
	}

protected:
	Set<IMaterial*>		m_newMaterials{ PP_SL };
};

static CEqMatSystemThreadedLoader s_threadedMaterialLoader;
static CTextureLoader s_textureLoader;

//---------------------------------------------------------------------------

CMaterialSystem::CMaterialSystem()
{
	m_proxyTimer.GetTime(true);

	// register when the DLL is connected
	g_eqCore->RegisterInterface(this);
	g_eqCore->RegisterInterface(&s_textureLoader);

}

CMaterialSystem::~CMaterialSystem()
{
	// unregister when DLL disconnects
	g_eqCore->UnregisterInterface<CMaterialSystem>();
	g_eqCore->UnregisterInterface<CTextureLoader>();
}

// Initializes material system
bool CMaterialSystem::Init(const MaterialsInitSettings& config)
{
	Msg(" \n--------- MaterialSystem Init --------- \n");

	const KVSection* matSystemSettings = g_eqCore->GetConfig()->FindSection("MaterialSystem");

	ASSERT(g_renderAPI == nullptr);

	// store the configuration
	m_config = config.renderConfig;

	// setup material and texture folders
	{
		// regular materials
		m_materialsPath = config.materialsPath;
		if(!m_materialsPath.Length())
			m_materialsPath = KV_GetValueString(matSystemSettings ? matSystemSettings->FindSection("MaterialsPath") : nullptr, 0, "materials/");

		m_materialsPath.Path_FixSlashes();

		if (m_materialsPath.ToCString()[m_materialsPath.Length() - 1] != CORRECT_PATH_SEPARATOR)
			m_materialsPath.Append(CORRECT_PATH_SEPARATOR);

		// sources
		m_materialsSRCPath = config.materialsSRCPath;
		if(!m_materialsSRCPath.Length())
			m_materialsSRCPath = KV_GetValueString(matSystemSettings ? matSystemSettings->FindSection("MaterialsSRCPath") : nullptr, 0, "materialsSRC/");

		m_materialsSRCPath.Path_FixSlashes();

		if (m_materialsSRCPath.ToCString()[m_materialsSRCPath.Length() - 1] != CORRECT_PATH_SEPARATOR)
			m_materialsSRCPath.Append(CORRECT_PATH_SEPARATOR);

		// copy default path
		EqString texturePath = config.texturePath;
		EqString textureSRCPath = config.textureSRCPath;

		if (texturePath[0] == 0)
			texturePath = m_materialsPath.GetData();

		if (textureSRCPath[0] == 0)
			textureSRCPath = m_materialsSRCPath.GetData();

		s_textureLoader.Initialize(texturePath, textureSRCPath);
	}

	auto tryLoadRenderer = [this, &config](const char* rendererName)
	{
		EqString loadErr;
		DKMODULE* renderModule = g_fileSystem->OpenModule(rendererName, &loadErr);
		IRenderManager* renderMng = nullptr;
		IRenderLibrary* renderLib = nullptr;
		IShaderAPI* shaderAPI = nullptr;
		
		defer 
		{
			if(shaderAPI)
			{
				m_renderLibrary = renderLib;
				m_rendermodule = renderModule;
				m_shaderAPI = shaderAPI;
			}
			else
			{
				g_fileSystem->CloseModule(renderModule);
			}
		};

		if(!renderModule)
		{
			DevMsg(DEVMSG_MATSYSTEM, "Can't load renderer '%s' - %s", rendererName, loadErr.ToCString());
			return false;
		}

		renderMng = g_eqCore->GetInterface<IRenderManager>();
		if(!renderMng)
		{
			ErrorMsg("MatSystem Error: %s does not provide render manager interface", rendererName);
			return false;
		}

		renderLib = renderMng->CreateRenderer(config.shaderApiParams);
		if(!renderLib)
		{
			ErrorMsg("MatSystem Error: %s failed to create renderer interface", rendererName);
			return false;
		}

		// render library initialization
		if( !renderLib->InitCaps() )
			return false;

		if(!renderLib->InitAPI(config.shaderApiParams))
			return false;

		// we created all we've need
		shaderAPI = renderLib->GetRenderer();

		if(!shaderAPI)
		{
			ErrorMsg("Fatal error! Couldn't create ShaderAPI interface!\n");
			return false;
		}

		// init new created shader api with this parameters
		shaderAPI->Init(config.shaderApiParams);

		return true;
	};

#ifdef PLAT_ANDROID
	EqString rendererName = "libeqGLESRHI";

	if(!tryLoadRenderer(rendererName))
		return false;
#else
	// try explicitly set renderer
	EqString rendererName = config.rendererName;

	const int rendererCmdLine = g_cmdLine->FindArgument("-renderer");
	if (rendererCmdLine != -1)
		rendererName = g_cmdLine->GetArgumentsOf(rendererCmdLine);
	else if(g_cmdLine->FindArgument("-norender") != -1)
		rendererName = "eqNullRHI";

	if (rendererName.Length())
	{
		if(!tryLoadRenderer(rendererName))
			return false;
	}
	else
	{
		// try first working renderer from EQ.CONFIG
		const KVSection* rendererKey = matSystemSettings->FindSection("Renderer");
		if(rendererKey)
		{
			for(int i = 0; i < rendererKey->ValueCount(); ++i)
			{
				if(tryLoadRenderer(KV_GetValueString(rendererKey, i)))
				{
					break;
				}
			}

			if(!m_shaderAPI)
				return false;
		}
		else
		{
			rendererName = "eqGLRHI";

			if(!tryLoadRenderer(rendererName))
				return false;
		}
	}

#endif // PLAT_ANDROID

	g_renderAPI = m_shaderAPI;

	if(!m_dynamicMesh.Init(g_dynMeshVertexFormatDesc, elementsOf(g_dynMeshVertexFormatDesc)))
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
	if (!m_renderLibrary || !m_rendermodule || !g_renderAPI)
		return;

	Msg("MatSystem shutdown...\n");

	// shutdown thread first
	s_threadedMaterialLoader.StopThread(true);

	m_globalMaterialVars.variableMap.clear(true);
	m_globalMaterialVars.variables.clear(true);
		
	ClearRenderStates();
	m_dynamicMesh.Destroy();

	m_setMaterial = nullptr;
	m_defaultMaterial = nullptr;
	m_overdrawMaterial = nullptr;
	SetEnvironmentMapTexture(nullptr);

	m_whiteTexture = nullptr;
	m_luxelTestTexture = nullptr;

	FreeMaterials();

	m_shaderOverrideList.clear();
	m_proxyFactoryList.clear();

	m_renderLibrary->ReleaseSwapChains();
	g_renderAPI->Shutdown();
	m_renderLibrary->ExitAPI();

	for(DKMODULE* shaderModule : m_shaderLibs)
		g_fileSystem->CloseModule(shaderModule);

	// shutdown render libraries, all shaders and other
	g_fileSystem->CloseModule( m_rendermodule );
}

void CMaterialSystem::CreateWhiteTexture()
{
	CImage img;
	img.Ref_Grab();	// by grabbing ref we make sure it won't be deleted

	img.SetName("_matsys_white");

	const int nWidth = 4;
	const int nHeight = 4;

	ubyte* pLightmapData = img.Create(FORMAT_RGBA8, nWidth,nHeight,1,1);

	PixelWriter pixw(FORMAT_RGBA8, pLightmapData, 0);

	for (int y = 0; y < nHeight; ++y)
	{
		pixw.Seek( 0, y );
		for (int x = 0; x < nWidth; ++x)
		{
			pixw.WritePixel( 255, 255, 255, 255 );
		}
	}

	SamplerStateParams texSamplerParams;
	SamplerStateParams_Make(texSamplerParams, g_renderAPI->GetCaps(), TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP);

	FixedArray<CRefPtr<CImage>, 1> images;
	images.append(CRefPtr(&img));

	m_whiteTexture = g_renderAPI->CreateTexture(images, texSamplerParams, TEXFLAG_NOQUALITYLOD);

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

	texSamplerParams = g_renderAPI->MakeSamplerState(TEXFILTER_NEAREST,TEXADDRESS_CLAMP,TEXADDRESS_CLAMP,TEXADDRESS_CLAMP);

	m_luxelTestTexture = g_renderAPI->CreateTexture(images, texSamplerParams, TEXFLAG_NOQUALITYLOD);

	*/
}

void CMaterialSystem::InitDefaultMaterial()
{
	if(!m_defaultMaterial)
	{
		KVSection defaultParams;
		defaultParams.SetName("Default"); // set shader 'Default'
		defaultParams.SetKey("BaseTexture", "$basetexture");

		IMaterialPtr pMaterial = CreateMaterial("_default", &defaultParams);
		pMaterial->LoadShaderAndTextures();

		m_defaultMaterial = pMaterial;
	}

	if(!m_overdrawMaterial)
	{
		KVSection overdrawParams;
		overdrawParams.SetName("BaseUnlit"); // set shader 'BaseUnlit'
		overdrawParams.SetKey("BaseTexture", "_matsys_white");
		overdrawParams.SetKey("Color", "[0.045 0.02 0.02 1.0]");
		overdrawParams.SetKey("Additive", "1");
		overdrawParams.SetKey("ztest", "0");
		overdrawParams.SetKey("zwrite", "0");
	
		IMaterialPtr pMaterial = CreateMaterial("_overdraw", &overdrawParams);
		pMaterial->LoadShaderAndTextures();

		m_overdrawMaterial = pMaterial;
	}
}

MaterialsRenderSettings& CMaterialSystem::GetConfiguration()
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
	EqString loadErr;
	DKMODULE* shaderlib = g_fileSystem->OpenModule( libname );
	if(!shaderlib)
	{
		MsgError("LoadShaderLibrary Error: Failed to load %s - %s\n", libname, loadErr.ToCString());
		return false;
	}

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

bool CMaterialSystem::IsMaterialExist(const char* szMaterialName) const
{
	EqString mat_path(m_materialsPath + szMaterialName + _Es(".mat"));
	return g_fileSystem->FileExist(mat_path.GetData());
}

IMaterialPtr CMaterialSystem::CreateMaterial(const char* szMaterialName, KVSection* params)
{
	// must have names
	ASSERT_MSG(strlen(szMaterialName) > 0, "CreateMaterial - name is empty!");

	CRefPtr<CMaterial> material = CRefPtr_new(CMaterial, szMaterialName, false);

	{
		CScopedMutex m(s_matSystemMutex);

		const int nameHash = material->m_nameHash;

		// add to list
		ASSERT_MSG(m_loadedMaterials.find(nameHash) == m_loadedMaterials.end(), "Material %s was already created under that name", szMaterialName);
		m_loadedMaterials.insert(nameHash, material);
	}

	CreateMaterialInternal(material, params);

	return IMaterialPtr(material);
}

// creates new material with defined parameters
void CMaterialSystem::CreateMaterialInternal(CRefPtr<CMaterial> material, KVSection* params)
{
	PROF_EVENT("MatSystem Load Material");

	// create new material
	CMaterial* pMaterial = (CMaterial*)material.Ptr();

	// if no params, we can load it a usual way
	if (params)
		pMaterial->Init(params);
	else
		pMaterial->Init();
}

IMaterialPtr CMaterialSystem::GetMaterial(const char* szMaterialName)
{
	// Don't load null materials
	if( strlen(szMaterialName) == 0 )
		return nullptr;

	EqString materialName = szMaterialName;
	materialName = materialName.LowerCase();
	materialName.Path_FixSlashes();

	if (materialName.ToCString()[0] == CORRECT_PATH_SEPARATOR)
		materialName = materialName.ToCString() + 1;

	const int nameHash = StringToHash(materialName.ToCString(), true);

	CRefPtr<CMaterial> newMaterial;

	// find the material with existing name
	// it could be a material that was not been loaded from disk
	{
		CScopedMutex m(s_matSystemMutex);

		auto it = m_loadedMaterials.find(nameHash);
		if (!it.atEnd())
			return IMaterialPtr(*it);

		// by default try to load material file from disk
		newMaterial = CRefPtr_new(CMaterial, materialName, true);
		m_loadedMaterials.insert(nameHash, newMaterial);
	}

	CreateMaterialInternal(newMaterial, nullptr);
	return IMaterialPtr(newMaterial);
}

// If we have unliaded material, just load it
void CMaterialSystem::PreloadNewMaterials()
{
	CScopedMutex m(s_matSystemMutex);
	
	for (auto it = m_loadedMaterials.begin(); !it.atEnd(); ++it)
	{
		PutMaterialToLoadingQueue(IMaterialPtr(it.value()));
	}
}

// releases non-used materials
void CMaterialSystem::ReleaseUnusedMaterials()
{
	CScopedMutex m(s_matSystemMutex);

	for (auto it = m_loadedMaterials.begin(); !it.atEnd(); ++it)
	{
		CMaterial* material = (CMaterial*)*it;

		// don't unload default material
		if(!stricmp(material->GetName(), "Default"))
			continue;

		const int framesDiff = (material->m_frameBound - m_frame);

		// Flush materials
		if(framesDiff >= m_config.materialFlushThresh - 1)
			material->Cleanup(false, true);
	}
}

// Reloads materials
void CMaterialSystem::ReloadAllMaterials()
{
	CScopedMutex m(s_matSystemMutex);

	MsgInfo("Reloading all materials...\n");
	Array<IMaterialPtr> loadingList(PP_SL);

	for (auto it = m_loadedMaterials.begin(); !it.atEnd(); ++it)
	{
		CMaterial* material = (CMaterial*)*it;

		// don't unload default material
		if(!stricmp(material->GetName(), "Default"))
			continue;

		const bool loadedFromDisk = material->m_loadFromDisk;
		material->Cleanup(loadedFromDisk, true);

		// don't reload materials which are not from disk
		if(!loadedFromDisk)
			material->Init(nullptr);
		else
			material->Init();

		const int framesDiff = (material->m_frameBound - m_frame);

		// preload material if it was ever used before
		if(framesDiff >= -1)
			loadingList.append(IMaterialPtr(material));
	}

	// issue loading after all materials were freed
	// - this is a guarantee to shader recompilation
	for(int i = 0; i < loadingList.numElem(); i++)
		PutMaterialToLoadingQueue( loadingList[i] );
}

// frees all materials
void CMaterialSystem::FreeMaterials()
{
	CScopedMutex m(s_matSystemMutex);

	for (auto it = m_loadedMaterials.begin(); !it.atEnd(); ++it)
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
	for (auto i = m_blendStates.begin(); !i.atEnd(); ++i)
		g_renderAPI->DestroyRenderState(*i);
	m_blendStates.clear();
	
	for (auto i = m_depthStates.begin(); !i.atEnd(); ++i)
		g_renderAPI->DestroyRenderState(*i);
	m_depthStates.clear();
	
	for (auto i = m_rasterStates.begin(); !i.atEnd(); ++i)
		g_renderAPI->DestroyRenderState(*i);
	m_rasterStates.clear();
}

// frees single material
void CMaterialSystem::FreeMaterial(IMaterial* pMaterial)
{
	if(pMaterial == nullptr)
		return;

	ASSERT_MSG(pMaterial->Ref_Count() == 0, "Material %s refcount = %d", pMaterial->GetName(), pMaterial->Ref_Count());

	DevMsg(DEVMSG_MATSYSTEM, "freeing material %s\n", pMaterial->GetName());
	CMaterial* material = (CMaterial*)pMaterial;
	{
		CScopedMutex m(s_matSystemMutex);
		m_loadedMaterials.remove(material->m_nameHash);
	}
}

void CMaterialSystem::RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName)
{
	ShaderProxyFactory factory;
	factory.name = pszName;
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

	return nullptr;
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

	ShaderFactory newShader;

	newShader.dispatcher = dispatcher_creation;
	newShader.shader_name = (char*)pszShaderName;

	m_shaderFactoryList.append(newShader);
}

// registers overrider for shaders
void CMaterialSystem::RegisterShaderOverrideFunction(const char* shaderName, DISPATCH_OVERRIDE_SHADER check_function)
{
	ShaderOverride new_override;
	new_override.shadername = shaderName;
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

	return nullptr;
}

//------------------------------------------------------------------------------------------------

typedef bool (*PFNMATERIALBINDCALLBACK)(IMaterial* pMaterial, uint paramMask);

static void BindFFPMaterial(IMaterial* pMaterial, int paramMask)
{
	g_renderAPI->SetShader(nullptr);

	g_renderAPI->SetBlendingState(nullptr);
	g_renderAPI->SetRasterizerState(nullptr);
	g_renderAPI->SetDepthStencilState(nullptr);
}

static bool Callback_BindErrorTextureFFPMaterial(IMaterial* pMaterial, uint paramMask)
{
	BindFFPMaterial(pMaterial, paramMask);
	g_renderAPI->SetTexture(StringToHashConst("BaseTextureSampler"), g_renderAPI->GetErrorTexture());

	return false;
}

static bool Callback_BindNormalMaterial(IMaterial* pMaterial, uint paramMask)
{
	((CMaterial*)pMaterial)->Setup(paramMask);

	return true;
}

PFNMATERIALBINDCALLBACK materialstate_callbacks[] =
{
	Callback_BindErrorTextureFFPMaterial,	// binds ffp shader with error texture, error shader only
	Callback_BindNormalMaterial				// binds normal material.
};

enum EMaterialRenderSubroutine
{
	MATERIAL_SUBROUTINE_ERROR = 0,
	MATERIAL_SUBROUTINE_NORMAL,
};

// loads material or sends it to loader thread
void CMaterialSystem::PutMaterialToLoadingQueue(const IMaterialPtr& pMaterial)
{
	CMaterial* material = (CMaterial*)pMaterial.Ptr();
	if(Atomic::CompareExchange(material->m_state, MATERIAL_LOAD_NEED_LOAD, MATERIAL_LOAD_INQUEUE) != MATERIAL_LOAD_NEED_LOAD)
	{
		return;
	}

	if( m_config.threadedloader )
	{
		s_threadedMaterialLoader.AddMaterial(pMaterial);
	}
	else
	{
		material->DoLoadShaderAndTextures();
	}
}

int CMaterialSystem::GetLoadingQueue() const
{
	return s_threadedMaterialLoader.GetCount();
}

void CMaterialSystem::SetShaderParameterOverriden(int param, bool set)
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

MatVarProxyUnk CMaterialSystem::FindGlobalMaterialVar(int nameHash) const
{
	CScopedMutex m(s_matSystemMutex);
	auto it = m_globalMaterialVars.variableMap.find(nameHash);
	if (it.atEnd())
		return MatVarProxyUnk();

	return MatVarProxyUnk(*it, *const_cast<MaterialVarBlock*>(&m_globalMaterialVars));
}

MatVarProxyUnk CMaterialSystem::FindGlobalMaterialVarByName(const char* pszVarName) const
{
	const int nameHash = StringToHash(pszVarName);
	return FindGlobalMaterialVar(nameHash);
}

MatVarProxyUnk	CMaterialSystem::GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue)
{
	CScopedMutex m(s_matSystemMutex);

	const int nameHash = StringToHash(pszVarName);

	auto it = m_globalMaterialVars.variableMap.find(nameHash);
	if (!it.atEnd())
		return MatVarProxyUnk(*it, m_globalMaterialVars);

	const int varId = m_globalMaterialVars.variables.numElem();
	MatVarData& newVar = m_globalMaterialVars.variables.append();
	MatVarHelper::Init(newVar, defaultValue);

	m_globalMaterialVars.variableMap.insert(nameHash, varId);

	return MatVarProxyUnk(varId, m_globalMaterialVars);
}

bool CMaterialSystem::BindMaterial(IMaterial* pMaterial, int flags)
{
	if(!pMaterial)
	{
		InitDefaultMaterial();
		pMaterial = m_defaultMaterial;
	}

	CMaterial* pSetupMaterial = (CMaterial*)pMaterial;

	// proxy update is dirty if material was not bound to this frame
	if (pSetupMaterial->m_frameBound != m_frame)
		pSetupMaterial->UpdateProxy(m_proxyDeltaTime);

	pSetupMaterial->m_frameBound = m_frame;

	// it's now a more critical section to the material
	pSetupMaterial->DoLoadShaderAndTextures();

	// set the current material
	IMaterial* setMaterial = pMaterial;

	// try overriding material
	if (m_preApplyCallback)
		setMaterial = m_preApplyCallback->OnPreBindMaterial(setMaterial);
	
	EMaterialRenderSubroutine subRoutineId = MATERIAL_SUBROUTINE_NORMAL;

	if( pMaterial->GetState() == MATERIAL_LOAD_ERROR || pMaterial->GetState() == MATERIAL_LOAD_NEED_LOAD )
		subRoutineId = MATERIAL_SUBROUTINE_ERROR;

	// if material is still loading, use the default material for a while
	if( pMaterial->GetState() == MATERIAL_LOAD_INQUEUE )
	{
		InitDefaultMaterial();
		setMaterial = m_defaultMaterial;
		subRoutineId = MATERIAL_SUBROUTINE_NORMAL;
	}

	bool success;

	if( m_config.overdrawMode )
	{
		InitDefaultMaterial();
		m_ambColor = color_white;
		success = (*materialstate_callbacks[subRoutineId])(m_overdrawMaterial, 0xFFFFFFFF);
	}
	else
		success = (*materialstate_callbacks[subRoutineId])(setMaterial, m_paramOverrideMask);

	m_setMaterial = IMaterialPtr(setMaterial);

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
		g_renderAPI->Apply();
		return;
	}

	// callback before applying
	if(m_preApplyCallback)
		m_preApplyCallback->OnPreApplyMaterial(setMaterial);

	g_renderAPI->Apply();
}

// returns bound material
IMaterialPtr CMaterialSystem::GetBoundMaterial() const
{
	return m_setMaterial;
}

// waits for material loader thread is finished
void CMaterialSystem::WaitAllMaterialsLoaded()
{
	if (m_config.threadedloader && s_threadedMaterialLoader.IsRunning())
		s_threadedMaterialLoader.WaitForThread();
}

// transform operations
// sets up a matrix, projection, view, and world
void CMaterialSystem::SetMatrix(EMatrixMode mode, const Matrix4x4 &matrix)
{
	m_matrices[(int)mode] = matrix;
	m_matrixDirty |= (1 << mode);
}

// returns a typed matrix
void CMaterialSystem::GetMatrix(EMatrixMode mode, Matrix4x4 &matrix)
{
	matrix = m_matrices[(int)mode];
}

// retunrs multiplied matrix
void CMaterialSystem::GetWorldViewProjection(Matrix4x4 &matrix)
{
	Matrix4x4 vproj = m_viewProjMatrix;
	Matrix4x4 wvpMatrix = m_wvpMatrix;

	const int viewProjBits = (1 << MATRIXMODE_PROJECTION) | (1 << MATRIXMODE_VIEW);
	const int wvpBits = viewProjBits | (1 << MATRIXMODE_WORLD) | (1 << MATRIXMODE_WORLD2);

	// cache view projection
	if(m_matrixDirty & viewProjBits)
		m_viewProjMatrix = vproj = m_matrices[MATRIXMODE_PROJECTION] * m_matrices[MATRIXMODE_VIEW];

	if(m_matrixDirty & wvpBits)
		m_wvpMatrix = wvpMatrix = vproj * (m_matrices[MATRIXMODE_WORLD2] * m_matrices[MATRIXMODE_WORLD]);

	m_matrixDirty = 0;

	matrix = wvpMatrix;
}

// sets an ambient light
void CMaterialSystem::SetAmbientColor(const ColorRGBA &color)
{
	m_ambColor = color;
}

ColorRGBA CMaterialSystem::GetAmbientColor() const
{
	return m_ambColor;
}

const IMaterialPtr& CMaterialSystem::GetDefaultMaterial() const
{
	return m_defaultMaterial;
}

const ITexturePtr& CMaterialSystem::GetWhiteTexture() const
{
	return m_whiteTexture;
}

const ITexturePtr& CMaterialSystem::GetLuxelTestTexture() const
{
	return m_luxelTestTexture;
}

//-----------------------------
// Frame operations
//-----------------------------

// tells 3d device to begin frame
bool CMaterialSystem::BeginFrame(IEqSwapChain* swapChain)
{
	if(!m_shaderAPI)
		return false;

	bool state, oldState = m_deviceActiveState;
	m_deviceActiveState = state = g_renderAPI->IsDeviceActive();

	if(!state && state != oldState)
	{
		for(int i = 0; i < m_lostDeviceCb.numElem(); i++)
		{
			if (!m_lostDeviceCb[i])
				continue;

			if(!m_lostDeviceCb[i]())
				return false;
		}
	}

	if(s_threadedMaterialLoader.GetCount())
		s_threadedMaterialLoader.SignalWork();

	m_renderLibrary->BeginFrame(swapChain);

	if(state && state != oldState)
	{
		for(int i = 0; i < m_restoreDeviceCb.numElem(); i++)
		{
			if (!m_restoreDeviceCb[i])
				continue;

			if(!m_restoreDeviceCb[i]())
				return false;
		}
	}

	const bool clearBackBuffer = m_config.overdrawMode || r_clear.GetBool();
	const MColor clearColor = m_config.overdrawMode ? color_black : MColor(0.1f, 0.1f, 0.1f, 1.0f);

	// reset viewport and scissor
	IVector2D backbufferSize = m_backbufferSize;
	if (swapChain)
		swapChain->GetBackbufferSize(backbufferSize.x, backbufferSize.y);

	g_renderAPI->SetViewport(0, 0, backbufferSize.x, backbufferSize.y);
	g_renderAPI->SetScissorRectangle(IAARectangle(0, 0, backbufferSize.x, backbufferSize.y));

#ifdef PLAT_ANDROID
	// always clear all on Android
	g_renderAPI->Clear(true, true, true, clearColor);
#else
	g_renderAPI->Clear(clearBackBuffer, true, false, clearColor);
#endif

	return true;
}

// tells 3d device to end and present frame
bool CMaterialSystem::EndFrame()
{
	if(m_renderLibrary)
		m_renderLibrary->EndFrame();

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
	m_backbufferSize = IVector2D(wide, tall);
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

	return nullptr;
}

void CMaterialSystem::DestroySwapChain(IEqSwapChain* swapChain)
{
	if(m_renderLibrary)
		m_renderLibrary->DestroySwapChain(swapChain);
}

// fullscreen mode changing
bool CMaterialSystem::SetWindowed(bool enable)
{
	const bool changeMode = (m_renderLibrary->IsWindowed() != enable);
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
IShaderAPI* CMaterialSystem::GetShaderAPI() const
{
	return m_shaderAPI;
}

//--------------------------------------------------------------------------------------
// Shader dynamic states

// sets a fog info
void CMaterialSystem::SetFogInfo(const FogInfo &info)
{
	m_fogInfo = info;
}

// returns fog info
void CMaterialSystem::GetFogInfo(FogInfo &info) const
{
	if( m_config.overdrawMode)
	{
		static FogInfo nofog;
		info = nofog;
		return;
	}

	info = m_fogInfo;
}

// sets pre-apply callback
void CMaterialSystem::SetRenderCallbacks( IMatSysRenderCallbacks* callback )
{
	m_preApplyCallback = callback;
}

// returns current pre-apply callback
IMatSysRenderCallbacks* CMaterialSystem::GetRenderCallbacks() const
{
	return m_preApplyCallback;
}

// sets pre-apply callback
void CMaterialSystem::SetEnvironmentMapTexture(const ITexturePtr& pEnvMapTexture)
{
	m_currentEnvmapTexture = pEnvMapTexture;
}

// returns current pre-apply callback
const ITexturePtr& CMaterialSystem::GetEnvironmentMapTexture() const
{
	return m_currentEnvmapTexture;
}

ECullMode CMaterialSystem::GetCurrentCullMode() const
{
	return m_cullMode;
}

void CMaterialSystem::SetCullMode(ECullMode cullMode)
{
	m_cullMode = cullMode;
}

// skinning mode
void CMaterialSystem::SetSkinningEnabled( bool bEnable )
{
	m_skinningEnabled = bEnable;
}

bool CMaterialSystem::IsSkinningEnabled() const
{
	return m_skinningEnabled;
}

// instancing mode
void CMaterialSystem::SetInstancingEnabled( bool bEnable )
{
	m_instancingEnabled = bEnable;
}

bool CMaterialSystem::IsInstancingEnabled() const
{
	return m_instancingEnabled;
}

// sets up a 2D mode
void CMaterialSystem::Setup2D(float wide, float tall)
{
	SetMatrix(MATRIXMODE_PROJECTION, projection2DScreen(wide,tall));
	SetMatrix(MATRIXMODE_VIEW, identity4);
	SetMatrix(MATRIXMODE_WORLD, identity4);
	SetMatrix(MATRIXMODE_WORLD2, identity4);
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

void CMaterialSystem::DrawDefaultUP(EPrimTopology type, int vertFVF, const void* verts, int numVerts,
	const ITexturePtr& texture, const MColor& color,
	BlendStateParams* blendParams, DepthStencilStateParams* depthParams,
	RasterizerStateParams* rasterParams)
{
	ASSERT_MSG(vertFVF & VERTEX_FVF_XYZ, "DrawDefaultUP must have FVF_XYZ in vertex flags");

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

	ColorRGBA oldAmbColor = m_ambColor;
	m_ambColor = color;

	IMaterialSystem::FindGlobalMaterialVar<MatTextureProxy>(StringToHashConst("basetexture")).Set(texture);
	BindMaterial(GetDefaultMaterial());

	CMeshBuilder meshBuilder(&m_dynamicMesh);
	meshBuilder.Begin(type);

	const ubyte* vertPtr = reinterpret_cast<const ubyte*>(verts);
	for(int i = 0; i < numVerts; ++i)
	{
		if (vertFVF & VERTEX_FVF_XYZ)
		{
			meshBuilder.Position3fv(*reinterpret_cast<const Vector3D*>(vertPtr));
			vertPtr += sizeof(Vector3D);
		}
		if (vertFVF & VERTEX_FVF_UVS)
		{
			meshBuilder.TexCoord2fv(*reinterpret_cast<const Vector2D*>(vertPtr));
			vertPtr += sizeof(Vector2D);
		}
		if (vertFVF & VERTEX_FVF_NORMAL)
		{
			meshBuilder.Normal3fv(*reinterpret_cast<const Vector3D*>(vertPtr));
			vertPtr += sizeof(Vector3D);
		}
		if (vertFVF & VERTEX_FVF_COLOR)
		{
			meshBuilder.Color4(*reinterpret_cast<const uint*>(vertPtr));
			vertPtr += sizeof(uint);
		}
		meshBuilder.AdvanceVertex();
	}
	meshBuilder.End();

	m_ambColor = oldAmbColor;
}

//-----------------------------
// RHI render states setup
//-----------------------------

// sets blending
void CMaterialSystem::SetBlendingStates(const BlendStateParams& blend)
{
	SetBlendingStates(blend.srcFactor, blend.dstFactor, blend.blendFunc, blend.mask);
}

// sets depth stencil state
void CMaterialSystem::SetDepthStates(const DepthStencilStateParams& depth)
{
	SetDepthStates(depth.depthTest, depth.depthWrite, depth.depthFunc);
}

// sets rasterizer extended mode
void CMaterialSystem::SetRasterizerStates(const RasterizerStateParams& raster)
{
	SetRasterizerStates(raster.cullMode, raster.fillMode, raster.multiSample, raster.scissor, raster.useDepthBias);
}

// pack blending function to ushort
struct blendStateIndex_t
{
	blendStateIndex_t( EBlendFactor nSrcFactor, EBlendFactor nDestFactor, EBlendFunction nBlendingFunc, int colormask )
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
void CMaterialSystem::SetBlendingStates(EBlendFactor nSrcFactor, EBlendFactor nDestFactor, EBlendFunction nBlendingFunc, int colormask)
{
	blendStateIndex_t idx(nSrcFactor, nDestFactor, nBlendingFunc, colormask);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	auto blendState = m_blendStates.find(stateIndex);

	if(blendState.atEnd())
	{
		BlendStateParams desc;
		// no sense to enable blending when no visual effects...
		desc.blendEnable = !(nSrcFactor == BLENDFACTOR_ONE && nDestFactor == BLENDFACTOR_ZERO && nBlendingFunc == BLENDFUNC_ADD);
		desc.srcFactor = nSrcFactor;
		desc.dstFactor = nDestFactor;
		desc.blendFunc = nBlendingFunc;
		desc.mask = colormask;

		state = g_renderAPI->CreateBlendingState(desc);
		m_blendStates.insert(stateIndex, state);
	}
	else
		state = *blendState;

	g_renderAPI->SetBlendingState( state );
}

// pack depth states to ubyte
struct depthStateIndex_t
{
	depthStateIndex_t( bool bDoDepthTest, bool bDoDepthWrite, ECompareFunc depthCompFunc )
		: doDepthTest(bDoDepthTest), doDepthWrite(bDoDepthWrite), compFunc(depthCompFunc)
	{
	}

	ubyte doDepthTest : 1;
	ubyte doDepthWrite : 1;
	ubyte compFunc : 3;
	ubyte pad{ 0 };
};

assert_sizeof(depthStateIndex_t,2);

// sets depth stencil state
void CMaterialSystem::SetDepthStates(bool bDoDepthTest, bool bDoDepthWrite, ECompareFunc depthCompFunc)
{
	depthStateIndex_t idx(bDoDepthTest, bDoDepthWrite, depthCompFunc);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	auto depthState = m_depthStates.find(stateIndex);

	if(depthState.atEnd())
	{
		DepthStencilStateParams desc;
		desc.depthWrite = bDoDepthWrite;
		desc.depthTest = bDoDepthTest;
		desc.depthFunc = depthCompFunc;
		desc.doStencilTest = false;

		state = g_renderAPI->CreateDepthStencilState(desc);
		m_depthStates.insert(stateIndex, state);
	}
	else
		state = *depthState;

	g_renderAPI->SetDepthStencilState( state );
}

// pack blending function to ushort
struct rasterStateIndex_t
{
	rasterStateIndex_t( ECullMode nCullMode, EFillMode nFillMode, bool bMultiSample,bool bScissor, bool bPolyOffset )
		: cullMode(nCullMode), fillMode(nFillMode), multisample(bMultiSample), scissor(bScissor), polyoffset(bPolyOffset)
	{
	}

	ubyte cullMode : 2;
	ubyte fillMode : 2;
	ubyte multisample : 1;
	ubyte scissor : 1;
	ubyte polyoffset: 1;
	ubyte pad{ 0 };
};

assert_sizeof(rasterStateIndex_t,2);

// sets rasterizer extended mode
void CMaterialSystem::SetRasterizerStates(ECullMode nCullMode, EFillMode nFillMode,bool bMultiSample,bool bScissor,bool bPolyOffset)
{
	rasterStateIndex_t idx(nCullMode, nFillMode, bMultiSample, bScissor, bPolyOffset);
	ushort stateIndex = *(ushort*)&idx;

	IRenderState* state = nullptr;

	auto rasterState = m_rasterStates.find(stateIndex);

	if(rasterState.atEnd())
	{
		RasterizerStateParams desc;
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

		state = g_renderAPI->CreateRasterizerState(desc);
		m_rasterStates.insert(stateIndex, state);
	}
	else
		state = *rasterState;

	g_renderAPI->SetRasterizerState( state );
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
	CScopedMutex m(s_matSystemMutex);

	Msg("*** Material list begin ***\n");
	for (auto it = m_loadedMaterials.begin(); !it.atEnd(); ++it)
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
