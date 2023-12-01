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
static VertexLayoutDesc& GetDynamicMeshLayout()
{
	const int stride = sizeof(Vector4D) + sizeof(TVec4D<half>) * 2 + sizeof(uint);
	static VertexLayoutDesc s_levModelDrawVertLayout = Builder<VertexLayoutDesc>()
		.Stride(stride)
		.Attribute(VERTEXATTRIB_POSITION, "position", 0, 0, ATTRIBUTEFORMAT_FLOAT, 4)
		.Attribute(VERTEXATTRIB_TEXCOORD, "texCoord", 1, sizeof(Vector4D), ATTRIBUTEFORMAT_HALF, 4)
		.Attribute(VERTEXATTRIB_NORMAL, "normal", 2, sizeof(Vector4D) + sizeof(TVec4D<half>), ATTRIBUTEFORMAT_HALF, 4)
		.Attribute(VERTEXATTRIB_COLOR, "color", 3, sizeof(Vector4D) + sizeof(TVec4D<half>) * 2, ATTRIBUTEFORMAT_UINT8, 4)
		.End();
	return s_levModelDrawVertLayout;
}

DECLARE_CVAR(r_vSync, "0", "Vertical syncronization", CV_ARCHIVE);
DECLARE_CVAR(r_clear, "0", "Clear the backbuffer", CV_ARCHIVE);
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

	ASSERT(m_shaderAPI == nullptr);

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
		{
			ErrorMsg("Could not init renderer %s\n", rendererName.ToCString());
			return false;
		}
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
			{
				ErrorMsg("Could not init any renderer - perhaps your hardware does not support any\n");
				return false;
			}
		}
		else
		{
			rendererName = "eqGLRHI";

			if(!tryLoadRenderer(rendererName))
			{
				ErrorMsg("Could not init renderer %s\n", rendererName.ToCString());
				return false;
			}
		}
	}

#endif // PLAT_ANDROID

	g_renderAPI = m_shaderAPI;

	const VertexLayoutDesc& dynMeshLayout = GetDynamicMeshLayout();
	if(!m_dynamicMesh.Init(ArrayCRef(&dynMeshLayout, 1)))
	{
		ErrorMsg("Couldn't init DynamicMesh!\n");
		return false;
	}

	// initialize some resources
	REGISTER_INTERNAL_SHADERS();
	CreateWhiteTexture();
	CreateErrorTexture();
	CreateDefaultDepthTexture();
	InitDefaultMaterial();
	InitStandardMaterialProxies();
	
	return true;
}


void CMaterialSystem::Shutdown()
{
	if (!m_renderLibrary || !m_rendermodule || !m_shaderAPI)
		return;

	Msg("MatSystem shutdown...\n");

	// shutdown thread first
	s_threadedMaterialLoader.StopThread(true);

	m_globalMaterialVars.variableMap.clear(true);
	m_globalMaterialVars.variables.clear(true);
		
	m_dynamicMesh.Destroy();

	m_proxyUpdateCmdRecorder = nullptr;
	m_bufferCmdRecorder = nullptr;
	m_transientUniformBuffers = {};
	m_transientVertexBuffers = {};

	m_defaultMaterial = nullptr;
	m_overdrawMaterial = nullptr;
	m_currentEnvmapTexture = nullptr;
	m_defaultDepthTexture = nullptr;
	m_pendingCmdBuffers.clear(true);

	for (int i = 0; i < elementsOf(m_errorTexture); ++i)
		m_errorTexture[i] = nullptr;

	for (int i = 0; i < elementsOf(m_whiteTexture); ++i)
		m_whiteTexture[i] = nullptr;

	FreeMaterials();

	m_shaderOverrideList.clear();
	m_proxyFactoryList.clear();

	m_shaderAPI->Shutdown();
	m_renderLibrary->ExitAPI();

	for(DKMODULE* shaderModule : m_shaderLibs)
		g_fileSystem->CloseModule(shaderModule);

	// shutdown render libraries, all shaders and other
	g_fileSystem->CloseModule( m_rendermodule );
}

void CMaterialSystem::CreateWhiteTexture()
{
	CImagePtr img = CRefPtr_new(CImage);
	const int nWidth = 4;
	const int nHeight = 4;

	ubyte* texData = img->Create(FORMAT_RGBA8, nWidth, nHeight, IMAGE_DEPTH_CUBEMAP, 1);
	const int dataSize = img->GetMipMappedSize(0, 1) * img->GetArraySize();
	memset(texData, 0xffffffff, dataSize);

	FixedArray<CImagePtr, 1> images;
	images.append(img);

	img->SetDepth(IMAGE_DEPTH_CUBEMAP);
	img->SetName("_matsys_white_cb");
	ASSERT(img->GetImageType() == IMAGE_TYPE_CUBE);
	m_whiteTexture[TEXDIMENSION_CUBE] = m_shaderAPI->CreateTexture(images, SamplerStateParams(TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP), TEXFLAG_NOQUALITYLOD);

	img->SetDepth(1);
	img->SetName("_matsys_white");
	ASSERT(img->GetImageType() == IMAGE_TYPE_2D);
	m_whiteTexture[TEXDIMENSION_2D] = m_shaderAPI->CreateTexture(images, SamplerStateParams(TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP), TEXFLAG_NOQUALITYLOD);

	images.clear();
}

void CMaterialSystem::CreateErrorTexture()
{
	bool justCreated = false;
	m_errorTexture[TEXDIMENSION_2D] = m_shaderAPI->FindOrCreateTexture("error", justCreated);
	m_errorTexture[TEXDIMENSION_2D]->GenerateErrorTexture();

	m_errorTexture[TEXDIMENSION_CUBE] = m_shaderAPI->FindOrCreateTexture("error_cube", justCreated);
	m_errorTexture[TEXDIMENSION_CUBE]->GenerateErrorTexture(TEXFLAG_CUBEMAP);
}

void CMaterialSystem::CreateDefaultDepthTexture()
{
	m_defaultDepthTexture = m_shaderAPI->CreateRenderTarget("_matSys_depthBuffer", 800, 600, FORMAT_D24, TEXFILTER_NEAREST, TEXADDRESS_CLAMP);

	ASSERT_MSG(m_defaultDepthTexture, "Unable to create default depth texture");
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
		pMaterial->Init(m_shaderAPI, params);
	else
		pMaterial->Init(m_shaderAPI);
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
		QueueLoading(IMaterialPtr(it.value()));
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
			material->Init(m_shaderAPI, nullptr);
		else
			material->Init(m_shaderAPI);

		const int framesDiff = (material->m_frameBound - m_frame);

		// preload material if it was ever used before
		if(framesDiff >= -1)
			loadingList.append(IMaterialPtr(material));
	}

	// issue loading after all materials were freed
	// - this is a guarantee to shader recompilation
	for(int i = 0; i < loadingList.numElem(); i++)
		QueueLoading( loadingList[i] );
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

IMatSystemShader* CMaterialSystem::CreateShaderInstance(const char* szShaderName)
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

// loads material or sends it to loader thread
void CMaterialSystem::QueueLoading(const IMaterialPtr& pMaterial)
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
void CMaterialSystem::GetMatrix(EMatrixMode mode, Matrix4x4 &matrix) const
{
	matrix = m_matrices[(int)mode];
}

void CMaterialSystem::GetViewProjection(Matrix4x4& matrix) const
{
	const int viewProjBits = (1 << MATRIXMODE_PROJECTION) | (1 << MATRIXMODE_VIEW);

	// cache view projection
	if (m_matrixDirty & viewProjBits)
		m_viewProjMatrix = matrix = m_matrices[MATRIXMODE_PROJECTION] * m_matrices[MATRIXMODE_VIEW];
	else
		matrix = m_viewProjMatrix;
}

// retunrs multiplied matrix
void CMaterialSystem::GetWorldViewProjection(Matrix4x4 &matrix) const
{
	const int viewProjBits = (1 << MATRIXMODE_PROJECTION) | (1 << MATRIXMODE_VIEW);

	const int wvpBits = viewProjBits | (1 << MATRIXMODE_WORLD) | (1 << MATRIXMODE_WORLD2);
	Matrix4x4 vproj = m_viewProjMatrix;
	Matrix4x4 wvpMatrix = m_wvpMatrix;

	if (m_matrixDirty & viewProjBits)
		GetViewProjection(vproj);

	if(m_matrixDirty & wvpBits)
		m_wvpMatrix = wvpMatrix = vproj * (m_matrices[MATRIXMODE_WORLD2] * m_matrices[MATRIXMODE_WORLD]);

	m_matrixDirty = 0;

	matrix = wvpMatrix;
}

void CMaterialSystem::GetCameraParams(MatSysCamera& cameraParams, bool worldViewProj) const
{
	FogInfo fog;
	Matrix4x4 wvp_matrix, view, proj;
	if (worldViewProj)
		GetWorldViewProjection(cameraParams.viewProj);
	else
		GetViewProjection(cameraParams.viewProj);
	GetMatrix(MATRIXMODE_VIEW, cameraParams.view);
	GetMatrix(MATRIXMODE_PROJECTION, cameraParams.proj);

	cameraParams.invViewProj = !cameraParams.viewProj;

	// TODO: viewport parameters in Matsystem
	cameraParams.viewport.near = 0.0f;
	cameraParams.viewport.far = 0.0f;
	cameraParams.viewport.invWidth = 1.0f;
	cameraParams.viewport.invHeight = 1.0f;

	GetFogInfo(fog);
	cameraParams.position = fog.viewPos;

	// can use either fixed array or CMemoryStream with on-stack storage
	if (fog.enableFog)
	{
		cameraParams.fog.factor = fog.enableFog ? 1.0f : 0.0f;
		cameraParams.fog.near = fog.fognear;
		cameraParams.fog.far = fog.fogfar;
		cameraParams.fog.scale = 1.0f / (fog.fogfar - fog.fognear);
		cameraParams.fog.color = fog.fogColor;
	}
	else
	{
		cameraParams.fog.factor = 0.0f;
		cameraParams.fog.near = 1000000.0f;
		cameraParams.fog.far = 1000000.0f;
		cameraParams.fog.scale = 1.0f;
		cameraParams.fog.color = color_white;
	}
}

const IMaterialPtr& CMaterialSystem::GetDefaultMaterial() const
{
	return m_defaultMaterial;
}

const ITexturePtr& CMaterialSystem::GetWhiteTexture(ETextureDimension texDimension) const
{
	return m_whiteTexture[texDimension];
}

const ITexturePtr& CMaterialSystem::GetErrorCheckerboardTexture(ETextureDimension texDimension) const
{
	return m_errorTexture[texDimension];
}
//-----------------------------
// Frame operations
//-----------------------------

// tells 3d device to begin frame
bool CMaterialSystem::BeginFrame(ISwapChain* swapChain)
{
	if(!m_shaderAPI)
		return false;

	bool state, oldState = m_deviceActiveState;
	m_deviceActiveState = state = m_shaderAPI->IsDeviceActive();

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

	m_renderLibrary->SetVSync(r_vSync.GetBool());
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

	m_shaderAPI->SetViewport(IAARectangle(0, 0, backbufferSize.x, backbufferSize.y));
	m_shaderAPI->SetScissorRectangle(IAARectangle(0, 0, backbufferSize.x, backbufferSize.y));

#ifdef PLAT_ANDROID
	// always clear all on Android
	m_shaderAPI->Clear(true, true, true, clearColor);
#else
	m_shaderAPI->Clear(clearBackBuffer, true, false, clearColor);
#endif

	m_proxyUpdateCmdRecorder = g_renderAPI->CreateCommandRecorder("ProxyUpdate");

	return true;
}

// tells 3d device to end and present frame
bool CMaterialSystem::EndFrame()
{
	if (!m_shaderAPI)
		return false;

	// issue the rendering of anything
	m_shaderAPI->Flush();
	m_shaderAPI->ResetCounters();

	m_pendingCmdBuffers.append(m_proxyUpdateCmdRecorder->End());
	SubmitQueuedCommands();

	m_renderLibrary->EndFrame();

	m_frame++;
	m_proxyDeltaTime = m_proxyTimer.GetTime(true);

	return true;
}

void CMaterialSystem::SubmitQueuedCommands()
{
	m_shaderAPI->SubmitCommandBuffers(m_pendingCmdBuffers);
	m_pendingCmdBuffers.clear();
}

ITexturePtr CMaterialSystem::GetCurrentBackbuffer() const
{
	return m_renderLibrary->GetCurrentBackbuffer();
}

ITexturePtr CMaterialSystem::GetDefaultDepthBuffer() const
{
	return m_defaultDepthTexture;
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
	if (!m_renderLibrary)
		return;

	m_renderLibrary->SetBackbufferSize(wide, tall);
	m_shaderAPI->ResizeRenderTarget(m_defaultDepthTexture, wide, tall);
}

// reports device focus mode
void CMaterialSystem::SetDeviceFocused(bool inFocus)
{
	if (!m_renderLibrary)
		return;
	m_renderLibrary->SetFocused(inFocus);
}

ISwapChain* CMaterialSystem::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	if (!m_renderLibrary)
		return nullptr;

	return m_renderLibrary->CreateSwapChain(windowInfo);
}

void CMaterialSystem::DestroySwapChain(ISwapChain* swapChain)
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

	for (int i = 0; i < m_restoreDeviceCb.numElem(); i++)
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

void CMaterialSystem::GetPolyOffsetSettings(float& depthBias, float& depthBiasSlopeScale) const
{
	depthBias = r_depthBias.GetFloat();
	depthBiasSlopeScale = r_slopeDepthBias.GetFloat();
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

// skinning mode
void CMaterialSystem::SetSkinningEnabled( bool bEnable )
{
	m_skinningEnabled = bEnable;
}

bool CMaterialSystem::IsSkinningEnabled() const
{
	return m_skinningEnabled;
}

// TODO: per instance
void CMaterialSystem::SetSkinningBones(ArrayCRef<RenderBoneTransform> bones)
{
	m_boneTransforms.clear();
	m_skinningEnabled = bones.numElem() > 0;
	m_boneTransforms.reserve(bones.numElem());
	for (const RenderBoneTransform& bone : bones)
		m_boneTransforms.append(bone);
}

void CMaterialSystem::GetSkinningBones(ArrayCRef<RenderBoneTransform>& outBones) const
{
	outBones = m_boneTransforms;
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
// Helper rendering operations
//-----------------------------

IDynamicMesh* CMaterialSystem::GetDynamicMesh() const
{
	return (IDynamicMesh*)&m_dynamicMesh;
}

// returns temp buffer with data written. SubmitCommandBuffers uploads it to GPU
GPUBufferPtrView CMaterialSystem::GetTransientUniformBuffer(const void* data, int64 size)
{
	const int bufferAlignment = m_shaderAPI->GetCaps().minUniformBufferOffsetAlignment;
	constexpr int64 maxTransientBufferSize = 128 * 1024;

	TransientBufferCollection& collection = m_transientUniformBuffers;

	if (!m_bufferCmdRecorder)
		m_bufferCmdRecorder = g_renderAPI->CreateCommandRecorder("BufferSubmit");

	const int bufferIndex = collection.bufferIdx;
	IGPUBufferPtr& buffer = collection.buffers[bufferIndex];
	if (!buffer)
		buffer = m_shaderAPI->CreateBuffer(BufferInfo(1, maxTransientBufferSize), BUFFERUSAGE_UNIFORM, "TransientUniformBuffer");
	
	const int64 writeOffset = NextBufferOffset(size, collection.bufferOffsets[bufferIndex], maxTransientBufferSize, bufferAlignment);
	m_bufferCmdRecorder->WriteBuffer(buffer, data, size, writeOffset);

	return GPUBufferPtrView(buffer, writeOffset, size);
}

GPUBufferPtrView CMaterialSystem::GetTransientVertexBuffer(const void* data, int64 size)
{
	const int bufferAlignment = 4; // vertex buffers are aligned always 4 bytes
	constexpr int64 maxTransientBufferSize = 128 * 1024;

	TransientBufferCollection& collection = m_transientVertexBuffers;

	if (!m_bufferCmdRecorder)
		m_bufferCmdRecorder = g_renderAPI->CreateCommandRecorder("BufferSubmit");

	const int bufferIndex = collection.bufferIdx;
	IGPUBufferPtr& buffer = collection.buffers[bufferIndex];
	if (!buffer)
		buffer = m_shaderAPI->CreateBuffer(BufferInfo(1, maxTransientBufferSize), BUFFERUSAGE_VERTEX, "TransientVertexBuffer");

	const int64 writeOffset = NextBufferOffset(size, collection.bufferOffsets[bufferIndex], maxTransientBufferSize, bufferAlignment);
	m_bufferCmdRecorder->WriteBuffer(buffer, data, size, writeOffset);

	return GPUBufferPtrView(buffer, writeOffset, size);
}

void CMaterialSystem::QueueCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers)
{
	Array<IGPUCommandBufferPtr>& buffers = m_pendingCmdBuffers;

	buffers.append(m_dynamicMesh.GetSubmitBuffer());
	if (m_bufferCmdRecorder)
	{
		buffers.append(m_bufferCmdRecorder->End());
		m_bufferCmdRecorder = nullptr;

		{
			TransientBufferCollection& collection = m_transientUniformBuffers;
			collection.bufferOffsets[collection.bufferIdx] = 0;
			collection.bufferIdx = (collection.bufferIdx + 1) % elementsOf(collection.buffers);
		}

		{
			TransientBufferCollection& collection = m_transientVertexBuffers;
			collection.bufferOffsets[collection.bufferIdx] = 0;
			collection.bufferIdx = (collection.bufferIdx + 1) % elementsOf(collection.buffers);
		}
	}

	for (const IGPUCommandBufferPtr& buffer : cmdBuffers)
		buffers.append(buffer);
}

void CMaterialSystem::QueueCommandBuffer(const IGPUCommandBuffer* cmdBuffer)
{
	Array<IGPUCommandBufferPtr>& buffers = m_pendingCmdBuffers;
	
	buffers.append(m_dynamicMesh.GetSubmitBuffer());
	if (m_bufferCmdRecorder)
	{
		buffers.append(m_bufferCmdRecorder->End());
		m_bufferCmdRecorder = nullptr;

		{
			TransientBufferCollection& collection = m_transientUniformBuffers;
			collection.bufferOffsets[collection.bufferIdx] = 0;
			collection.bufferIdx = (collection.bufferIdx + 1) % elementsOf(collection.buffers);
		}

		{
			TransientBufferCollection& collection = m_transientVertexBuffers;
			collection.bufferOffsets[collection.bufferIdx] = 0;
			collection.bufferIdx = (collection.bufferIdx + 1) % elementsOf(collection.buffers);
		}
	}

	buffers.append(IGPUCommandBufferPtr(const_cast<IGPUCommandBuffer*>(cmdBuffer)));
}

void CMaterialSystem::Draw(const RenderDrawCmd& drawCmd)
{
	// no material means no pipeline state
	if (!drawCmd.material)
		return;

	IShaderAPI* renderAPI = m_shaderAPI;

	// TODO: render pass description should come as argument
	RenderPassDesc renderPassDesc = Builder<RenderPassDesc>()
		.ColorTarget(m_renderLibrary->GetCurrentBackbuffer())
		//.DepthStencilTarget(g_matSystem->GetDefaultDepthBuffer())
		.End();

	IGPURenderPassRecorderPtr rendPassRecorder = renderAPI->BeginRenderPass(renderPassDesc);

	SetupDrawCommand(drawCmd, rendPassRecorder);

	QueueCommandBuffer(rendPassRecorder->End());
}

void CMaterialSystem::SetupDrawCommand(const RenderDrawCmd& drawCmd, IGPURenderPassRecorder* rendPassRecorder)
{
	if (!drawCmd.material)
		return;

	const RenderInstanceInfo& instInfo = drawCmd.instanceInfo;
	const MeshInstanceFormatRef& instFormat = instInfo.instFormat;
	const MeshInstanceData& instData = instInfo.instData;
	const RenderMeshInfo& meshInfo = drawCmd.meshInfo;

	const int instanceCount = instData.buffer ? instData.count : 1;

	int firstEmptyVb = MAX_VERTEXSTREAM;
	int vertexLayoutBits = 0;
	for (int i = 0; i < instFormat.layout.numElem(); ++i)
	{
		if (instData.buffer && instFormat.layout[i].stepMode == VERTEX_STEPMODE_INSTANCE)
		{
			firstEmptyVb = i;
			vertexLayoutBits |= (1 << i);
		}
		else
			vertexLayoutBits |= instInfo.streamBuffers[i] ? (1 << i) : 0;
	}

	SetupMaterialPipeline(drawCmd.material, meshInfo.primTopology, instFormat, vertexLayoutBits, drawCmd.userData, rendPassRecorder);

	if (instFormat.layout.numElem())
	{
		CMaterial* material = static_cast<CMaterial*>(drawCmd.material);
		IMatSystemShader* matShader = material->m_shader;

		ASSERT_MSG(matShader->IsSupportInstanceFormat(instFormat.nameHash), "Shader '%s' used by %s does not support vertex format '%s'", drawCmd.material->GetShaderName(), drawCmd.material->GetName(), instInfo.instFormat.name);

		for (int i = 0; i < instInfo.streamBuffers.numElem(); ++i)
			rendPassRecorder->SetVertexBuffer(i, instInfo.streamBuffers[i], instInfo.streamOffsets[i], instInfo.streamSizes[i]);

		// bind instance
		if (instData.buffer)
		{
			ASSERT_MSG(firstEmptyVb != MAX_VERTEXSTREAM, "No free slots for instance buffer");
			rendPassRecorder->SetVertexBuffer(firstEmptyVb, instData.buffer, instData.offset, instData.stride * instData.count);
		}

		rendPassRecorder->SetIndexBuffer(instInfo.indexBuffer, instInfo.indexFormat, instInfo.indexBufOffset, instInfo.indexBufSize);
	}

	if (meshInfo.firstIndex < 0 && meshInfo.numIndices == 0)
		rendPassRecorder->Draw(meshInfo.numVertices, meshInfo.firstVertex, instanceCount, instData.first);
	else
		rendPassRecorder->DrawIndexed(meshInfo.numIndices, meshInfo.firstIndex, instanceCount, meshInfo.baseVertex, instData.first);
}

void CMaterialSystem::SetupMaterialPipeline(IMaterial* material, EPrimTopology primTopology, const MeshInstanceFormatRef& meshInstFormat, int vertexLayoutBits, const void* userData, IGPURenderPassRecorder* rendPassRecorder)
{
	IShaderAPI* renderAPI = m_shaderAPI;

	if (m_preApplyCallback)
		material = m_preApplyCallback->OnPreBindMaterial(material);

	// force load shader and textures if not already
	material->LoadShaderAndTextures();

	CMaterial* matSysMaterial = static_cast<CMaterial*>(material);
	IMatSystemShader* matShader = matSysMaterial->m_shader;

	const uint proxyFrame = m_frame;
	if (matSysMaterial->m_frameBound != proxyFrame)
	{
		matSysMaterial->UpdateProxy(m_proxyDeltaTime, m_proxyUpdateCmdRecorder);
		matSysMaterial->m_frameBound = proxyFrame;
	}

	// TODO: overdraw material. Or maybe debug property in shader?

	matShader->SetupRenderPass(renderAPI, rendPassRecorder, meshInstFormat, vertexLayoutBits, primTopology, userData);
}

bool CMaterialSystem::SetupDrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts, IGPURenderPassRecorder* rendPassRecorder)
{
	ASSERT_MSG(vertFVF & VERTEX_FVF_XYZ, "DrawDefaultUP must have FVF_XYZ in vertex flags");

	CMeshBuilder meshBuilder(&m_dynamicMesh);
	meshBuilder.Begin(primTopology);

	const ubyte* vertPtr = reinterpret_cast<const ubyte*>(verts);
	for (int i = 0; i < numVerts; ++i)
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
		if (vertFVF & VERTEX_FVF_COLOR_DW)
		{
			meshBuilder.Color4(*reinterpret_cast<const uint*>(vertPtr));
			vertPtr += sizeof(uint);
		}
		if (vertFVF & VERTEX_FVF_COLOR_VEC)
		{
			meshBuilder.Color4(*reinterpret_cast<const Vector4D*>(vertPtr));
			vertPtr += sizeof(Vector4D);
		}
		meshBuilder.AdvanceVertex();
	}

	RenderDrawCmd drawCmd;
	if (!meshBuilder.End(drawCmd))
		return false;

	IShaderAPI* renderAPI = m_shaderAPI;

	const CMaterial* material = static_cast<CMaterial*>(GetDefaultMaterial().Ptr());
	IMatSystemShader* matShader = material->m_shader;

	const RenderInstanceInfo& instInfo = drawCmd.instanceInfo;
	const MeshInstanceData& instData = instInfo.instData;
	const MeshInstanceFormatRef& instFormat = instInfo.instFormat;
	const RenderMeshInfo& meshInfo = drawCmd.meshInfo;

	const int instanceCount = instData.buffer ? instData.count : 1;

	// material must support correct vertex layout state
	if (instFormat.layout.numElem())
	{
		ASSERT_MSG(matShader->IsSupportInstanceFormat(instFormat.nameHash), "Shader '%s' used by %s does not support vertex format '%s'", drawCmd.material->GetShaderName(), drawCmd.material->GetName(), instFormat.name);

		int vertexLayoutBits = 0;
		for (int i = 0; i < instFormat.layout.numElem(); ++i)
		{
			if (instData.buffer && instFormat.layout[i].stepMode == VERTEX_STEPMODE_INSTANCE)
			{
				ASSERT_FAIL("DrawDefaultUP does not support instancing yet");
			}
			else
				vertexLayoutBits |= instInfo.streamBuffers[i] ? (1 << i) : 0;
		}

		matShader->SetupRenderPass(renderAPI, rendPassRecorder, instFormat, vertexLayoutBits, primTopology, &rendPassInfo);

		for (int i = 0; i < instInfo.streamBuffers.numElem(); ++i)
			rendPassRecorder->SetVertexBuffer(i, instInfo.streamBuffers[i], instInfo.streamOffsets[i], instInfo.streamSizes	[i]);
	}

	if (rendPassInfo.scissorRectangle.leftTop != IVector2D(-1, -1) && rendPassInfo.scissorRectangle.rightBottom != IVector2D(-1, -1))
		rendPassRecorder->SetScissorRectangle(rendPassInfo.scissorRectangle);

	if (meshInfo.firstIndex < 0 && meshInfo.numIndices == 0)
		rendPassRecorder->Draw(meshInfo.numVertices, meshInfo.firstVertex, instanceCount, instData.first);
	else
		rendPassRecorder->DrawIndexed(meshInfo.numIndices, meshInfo.firstIndex, instanceCount, meshInfo.baseVertex, instData.first);

	return true;
}

void CMaterialSystem::DrawDefaultUP(const MatSysDefaultRenderPass& rendPassInfo, EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts)
{
	// Draw to default view
	RenderPassDesc renderPassDesc = Builder<RenderPassDesc>()
		.ColorTarget(m_renderLibrary->GetCurrentBackbuffer())
		//.DepthStencilTarget(g_matSystem->GetDefaultDepthBuffer())
		.End();

	IShaderAPI* renderAPI = m_shaderAPI;
	IGPURenderPassRecorderPtr rendPassRecorder = renderAPI->BeginRenderPass(renderPassDesc);

	if (!SetupDrawDefaultUP(rendPassInfo, primTopology, vertFVF, verts, numVerts, rendPassRecorder))
		return;

	QueueCommandBuffer(rendPassRecorder->End());
}

// use this if you have objects that must be destroyed when device is lost
void CMaterialSystem::AddDestroyLostCallbacks(DEVLICELOSTRESTORE destroy, DEVLICELOSTRESTORE restore)
{
	m_lostDeviceCb.addUnique(destroy);
	m_restoreDeviceCb.addUnique(restore);
}

// prints loaded materials to console
void CMaterialSystem::PrintLoadedMaterials() const
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
