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


using namespace Threading;
static CEqMutex s_matSystemMutex;
static CEqMutex s_matSystemPipelineMutex;
static CEqMutex s_matSystemBufferMutex;

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
DECLARE_CVAR(r_showPipelineCacheMisses, "0", "Show warning messages about shader pipeline cache misses", CV_ARCHIVE);

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

		CMaterial* matSysMaterial = static_cast<CMaterial*>(nextMaterial);

		// check if loader is only one owning the material
		if (matSysMaterial->Ref_Count() == 1)
		{
			MsgWarning("Material %s is freed before loading\n", matSysMaterial->GetName());

			// switch state back
			Atomic::CompareExchange(matSysMaterial->m_state, MATERIAL_LOAD_INQUEUE, MATERIAL_LOAD_NEED_LOAD);
			matSysMaterial->Ref_Drop();
			return 0;
		}

		matSysMaterial->Ref_Drop();

		// load this material
		matSysMaterial->DoLoadShaderAndTextures();

		// try load text
		if(m_newMaterials.size())
			SignalWork();

		return 0;
	}

	void AddMaterial(IMaterialPtr pMaterial)
	{
		if (!pMaterial)
			return;

		ASSERT(pMaterial->GetState() == MATERIAL_LOAD_INQUEUE);

		if (g_parallelJobs->IsInitialized())
		{
			// Wooohoo Blast Processing!
			FunctionJob* job = PPNew FunctionJob("LoadMaterialJob", [pMaterial](void*, int) {
				((CMaterial*)pMaterial.Ptr())->DoLoadShaderAndTextures();
			});
			job->DeleteOnFinish();
			g_parallelJobs->GetJobMng()->InitStartJob(job);

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

		fnmPathFixSeparators(m_materialsPath);

		if (m_materialsPath.ToCString()[m_materialsPath.Length() - 1] != CORRECT_PATH_SEPARATOR)
			m_materialsPath.Append(CORRECT_PATH_SEPARATOR);

		// sources
		m_materialsSRCPath = config.materialsSRCPath;
		if(!m_materialsSRCPath.Length())
			m_materialsSRCPath = KV_GetValueString(matSystemSettings ? matSystemSettings->FindSection("MaterialsSRCPath") : nullptr, 0, "materialsSRC/");

		fnmPathFixSeparators(m_materialsSRCPath);

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

		DevMsg(DEVMSG_MATSYSTEM, "Trying renderer %s\n", rendererName);
		
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
			MsgError("Can't load renderer '%s' - %s\n", rendererName, loadErr.ToCString());
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
	EqString rendererName = "libeqWGPURHI";

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
		// try first working renderer from E2.CONFIG
		const KVSection* rendererKey = matSystemSettings ? matSystemSettings->FindSection("Renderer") : nullptr;
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
			rendererName = "eqWGPURHI";

			if(!tryLoadRenderer(rendererName))
			{
				ErrorMsg("Could not init renderer %s\n", rendererName.ToCString());
				return false;
			}
		}
	}

#endif // PLAT_ANDROID

	m_renderLibrary->SetBackbufferSize(m_backbufferSize.x, m_backbufferSize.y);
	g_renderAPI = m_shaderAPI;

	// initialize some resources
	REGISTER_INTERNAL_SHADERS();
	CreateWhiteTexture();
	CreateErrorTexture();
	CreateDefaultDepthTexture();
	InitDefaultMaterial();
	InitStandardMaterialProxies();

	const VertexLayoutDesc& dynMeshLayout = GetDynamicMeshLayout();
	m_dynamicMeshVertexFormat = g_renderAPI->CreateVertexFormat("DynMeshVertex", ArrayCRef(&dynMeshLayout, 1));
	
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

	m_proxyUpdateCmdRecorder = nullptr;
	m_bufferCmdRecorder = nullptr;
	m_transientUniformBuffers = {};
	m_transientVertexBuffers = {};

	m_defaultMaterial = nullptr;
	m_gridMaterial = nullptr;
	m_overdrawMaterial = nullptr;
	m_currentEnvmapTexture = nullptr;
	m_defaultDepthTexture = nullptr;
	m_pendingCmdBuffers.clear(true);
	m_dynamicMeshes.clear(true);
	m_freeDynamicMeshes.clear(true);

	g_renderAPI->DestroyVertexFormat(m_dynamicMeshVertexFormat);
	m_dynamicMeshVertexFormat = nullptr;

	for (int i = 0; i < elementsOf(m_errorTexture); ++i)
		m_errorTexture[i] = nullptr;

	for (int i = 0; i < elementsOf(m_whiteTexture); ++i)
		m_whiteTexture[i] = nullptr;

	FreeMaterials();

	m_renderPipelineCache.clear();
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

	FixedArray<CImagePtr, 1> images;
	images.append(img);

	{
		ubyte* texData = img->Create(FORMAT_RGBA8, nWidth, nHeight, IMAGE_DEPTH_CUBEMAP, 1);
		const int dataSize = img->GetMipMappedSize(0, 1) * img->GetArraySize();
		memset(texData, 0xffffffff, dataSize);

		img->SetName("_matsys_white_cb");

		ASSERT(img->GetImageType() == IMAGE_TYPE_CUBE);
		m_whiteTexture[TEXDIMENSION_CUBE] = m_shaderAPI->CreateTexture(images, SamplerStateParams(TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP), TEXFLAG_IGNORE_QUALITY);
	}

	{
		ubyte* texData = img->Create(FORMAT_RGBA8, nWidth, nHeight, 1, 1);
		const int dataSize = img->GetMipMappedSize(0, 1) * img->GetArraySize();
		memset(texData, 0xffffffff, dataSize);

		img->SetName("_matsys_white");

		ASSERT(img->GetImageType() == IMAGE_TYPE_2D);
		m_whiteTexture[TEXDIMENSION_2D] = m_shaderAPI->CreateTexture(images, SamplerStateParams(TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP), TEXFLAG_IGNORE_QUALITY);
	}

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
	m_defaultDepthTexture = m_shaderAPI->CreateRenderTarget(
		Builder<TextureDesc>()
		.Name("_matSys_depthBuffer")
		.Format(FORMAT_D24)
		.Size(800, 600)
		.Sampler(SamplerStateParams(TEXFILTER_NEAREST, TEXADDRESS_CLAMP))
		.End()
	);

	ASSERT_MSG(m_defaultDepthTexture, "Unable to create default depth texture");
}

void CMaterialSystem::InitDefaultMaterial()
{
	if(!m_defaultMaterial)
	{
		KVSection defaultParams;
		defaultParams.SetName("Default"); // set shader 'Default'
		defaultParams.SetKey("BaseTexture", "$basetexture");

		m_defaultMaterial = CreateMaterial("_default", &defaultParams);
		m_defaultMaterial->LoadShaderAndTextures();
	}

	if (!m_gridMaterial)
	{
		KVSection gridParams;
		gridParams.SetName("PristineGrid");
		gridParams
			.SetKey("lineWidth", 0.05f)
			.SetKey("lineSpacing", 1.0f);
		m_gridMaterial = g_matSystem->CreateMaterial("MatSysPristineGrid", &gridParams);
	}

	/*if (!m_overdrawMaterial)
	{
		KVSection overdrawParams;
		overdrawParams.SetName("BaseUnlit"); // set shader 'BaseUnlit'
		overdrawParams.SetKey("BaseTexture", "_matsys_white");
		overdrawParams.SetKey("Color", "[0.045 0.02 0.02 1.0]");
		overdrawParams.SetKey("Additive", "1");
		overdrawParams.SetKey("ztest", "0");
		overdrawParams.SetKey("zwrite", "0");
	
		IMaterialPtr pMaterial = CreateMaterial("_overdraw", &overdrawParams);
		//pMaterial->LoadShaderAndTextures();

		m_overdrawMaterial = pMaterial;
	}*/
}

MatSysRenderSettings& CMaterialSystem::GetConfiguration()
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

// creates new material with defined parameters
void CMaterialSystem::CreateMaterialInternal(CRefPtr<CMaterial> material, const KVSection* params)
{
	PROF_EVENT("MatSystem Load Material");

	// create new material
	CMaterial* pMaterial = static_cast<CMaterial*>(material.Ptr());

	// if no params, we can load it a usual way
	if (params)
		pMaterial->Init(m_shaderAPI, params);
	else
		pMaterial->Init(m_shaderAPI);
}

IMaterialPtr CMaterialSystem::CreateMaterial(const char* szMaterialName, const KVSection* params, int instanceFormatId)
{
	// must have names
	ASSERT_MSG(strlen(szMaterialName) > 0, "CreateMaterial - name is empty!");

	CRefPtr<CMaterial> material = CRefPtr_new(CMaterial, szMaterialName, instanceFormatId, false);

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

IMaterialPtr CMaterialSystem::GetMaterial(const char* szMaterialName, int instanceFormatId)
{
	if(*szMaterialName == 0)
		return nullptr;

	EqString materialName = EqStringRef(szMaterialName).LowerCase();
	fnmPathFixSeparators(materialName);

	if (materialName[0] == CORRECT_PATH_SEPARATOR)
		materialName = materialName.ToCString() + 1;

	const int nameHash = StringToHash(materialName, true);

	CRefPtr<CMaterial> newMaterial;

	// find the material with existing name
	// it could be a material that was not been loaded from disk
	{
		CScopedMutex m(s_matSystemMutex);

		auto it = m_loadedMaterials.find(nameHash);
		if (!it.atEnd())
			return IMaterialPtr(*it);

		// by default try to load material file from disk
		newMaterial = CRefPtr_new(CMaterial, materialName, instanceFormatId, true);
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
		if(!CString::CompareCaseIns(material->GetName(), "Default"))
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
		if(!CString::CompareCaseIns(material->GetName(), "Default"))
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

void CMaterialSystem::RegisterProxy(PROXY_FACTORY_CB dispfunc, const char* pszName)
{
	ShaderProxyFactory factory;
	factory.name = pszName;
	factory.func = dispfunc;

	m_proxyFactoryList.append(factory);
}

IMaterialProxy* CMaterialSystem::CreateProxyByName(const char* pszName)
{
	for(const ShaderProxyFactory& factory : m_proxyFactoryList)
	{
		if(!factory.name.CompareCaseIns(pszName))
			return (factory.func)();
	}

	return nullptr;
}

void CMaterialSystem::RegisterShader(const ShaderFactory& factory)
{
	const int nameHash = StringToHash(factory.shaderName, true);
	auto it = m_shaderFactoryList.find(nameHash);
	if (!it.atEnd())
	{
		ASSERT_FAIL("MatSys shader '%s' already exists!\n", factory.shaderName);
		return;
	}

	DevMsg(DEVMSG_MATSYSTEM, "Registering shader '%s'\n", factory.shaderName);
	m_shaderFactoryList.insert(nameHash, factory);
}

// registers overrider for shaders
void CMaterialSystem::RegisterShaderOverride(const char* shaderName, OVERRIDE_SHADER_CB func)
{
	ASSERT(func);

	ShaderOverride new_override;
	new_override.shaderName = shaderName;
	new_override.func = func;

	// this is a higher priority
	m_shaderOverrideList.insert(new_override, 0);
}

MatSysShaderPipelineCache& CMaterialSystem::GetRenderPipelineCache(int shaderNameHash)
{
	CScopedMutex m(s_matSystemPipelineMutex);
	return m_renderPipelineCache[shaderNameHash];
}

const ShaderFactory* CMaterialSystem::GetShaderFactory(const char* szShaderName, int instanceFormatId)
{
	EqString shaderName( szShaderName );

	// check the override table
	for(const ShaderOverride& override : m_shaderOverrideList)
	{
		if(!override.shaderName.CompareCaseIns(szShaderName))
		{
			const char* overrideShaderName = override.func(instanceFormatId);
			if(overrideShaderName && *overrideShaderName) // only if we have shader name
			{
				shaderName = overrideShaderName;
				break;
			}
		}
	}

	// now find the factory and dispatch
	const int nameHash = StringToHash(shaderName, true);
	auto it = m_shaderFactoryList.find(nameHash);
	if (it.atEnd())
		return nullptr;
	
	return &it.value();
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

MatVarProxyUnk CMaterialSystem::GetGlobalMaterialVarByName(const char* pszVarName, const char* defaultValue)
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
	++m_cameraChangeId;
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

void CMaterialSystem::GetCameraParams(MatSysCamera& cameraParams, const Matrix4x4& proj, const Matrix4x4& view, const Matrix4x4& world) const
{
	cameraParams.proj = proj;
	cameraParams.view = view;

	cameraParams.viewProj = proj * view * world;
	cameraParams.invViewProj = !cameraParams.viewProj;

	// TODO: viewport parameters in Matsystem
	cameraParams.viewport.near = 0.0f;
	cameraParams.viewport.far = 0.0f;
	cameraParams.viewport.invWidth = 1.0f;
	cameraParams.viewport.invHeight = 1.0f;

	// TODO: this is hacky wacky way, need to use CViewParams
	FogInfo fog;
	GetFogInfo(fog);
	cameraParams.position = fog.viewPos;

	// can use either fixed array or CMemoryStream with on-stack storage
	if (fog.enableFog)
	{
		cameraParams.fog.factor = fog.enableFog ? 1.0f : 0.0f;
		cameraParams.fog.near = fog.fogNear;
		cameraParams.fog.far = fog.fogFar;
		cameraParams.fog.scale = 1.0f / (fog.fogFar - fog.fogNear);
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

const IMaterialPtr& CMaterialSystem::GetGridMaterial() const
{
	return m_gridMaterial;
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

	const bool prevDeviceState = m_shaderAPI->IsDeviceActive();
	if(!prevDeviceState)
	{
		for(int i = 0; i < m_lostDeviceCb.numElem(); i++)
		{
			if (!m_lostDeviceCb[i])
				continue;

			if(!m_lostDeviceCb[i]())
				return false;
		}
	}

	m_renderLibrary->SetVSync(r_vSync.GetBool());
	m_renderLibrary->BeginFrame(swapChain);
	const bool deviceState = m_shaderAPI->IsDeviceActive();

	// reset viewport and scissor
	if (!swapChain)
		m_shaderAPI->ResizeRenderTarget(m_defaultDepthTexture, { m_backbufferSize.x, m_backbufferSize.y, 1 });

	if(!prevDeviceState && deviceState)
	{
		for(int i = 0; i < m_restoreDeviceCb.numElem(); i++)
		{
			if (!m_restoreDeviceCb[i])
				continue;

			if(!m_restoreDeviceCb[i]())
				return false;
		}
	}

	if (s_threadedMaterialLoader.GetCount())
		s_threadedMaterialLoader.SignalWork();

	FramePrepareInternal();
	return true;
}

void CMaterialSystem::FramePrepareInternal()
{
	if (m_frameBegun)
		return;
	m_frameBegun = true;
	m_proxyUpdateCmdRecorder = g_renderAPI->CreateCommandRecorder("ProxyUpdate");
}

// tells 3d device to end and present frame
bool CMaterialSystem::EndFrame()
{
	if (!m_shaderAPI)
		return false;

	if (!m_frameBegun)
		return false;

	// issue the rendering of anything
	m_shaderAPI->ResetCounters();

	m_pendingCmdBuffers.append(m_proxyUpdateCmdRecorder->End());
	m_proxyUpdateCmdRecorder = nullptr;

	SubmitQueuedCommands();

	m_renderLibrary->EndFrame();

	++m_frame;
	m_proxyDeltaTime = m_proxyTimer.GetTime(true);

	m_frameBegun = false;

	return true;
}

void CMaterialSystem::SubmitQueuedCommands()
{
	m_shaderAPI->SubmitCommandBuffers(m_pendingCmdBuffers);
	m_pendingCmdBuffers.clear();
}

Future<bool> CMaterialSystem::SubmitQueuedCommandsAwaitable()
{
	Future<bool> future = m_shaderAPI->SubmitCommandBuffersAwaitable(m_pendingCmdBuffers);
	m_pendingCmdBuffers.clear();

	return future;
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

	m_renderLibrary->SetBackbufferSize(m_backbufferSize.x, m_backbufferSize.y);
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
	ASSERT_MSG(m_renderLibrary, "MatSystem is not initialized");

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
	depthBias = -r_depthBias.GetFloat();
	depthBiasSlopeScale = -r_slopeDepthBias.GetFloat();
}

//--------------------------------------------------------------------------------------
// Shader dynamic states

// sets a fog info
void CMaterialSystem::SetFogInfo(const FogInfo &info)
{
	m_fogInfo = info;
	++m_cameraChangeId;
}

// returns fog info
void CMaterialSystem::GetFogInfo(FogInfo &info) const
{
	if( m_config.overdrawMode)
	{
		info = {};
		info.viewPos = m_fogInfo.viewPos;
		return;
	}

	info = m_fogInfo;
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

IDynamicMeshPtr CMaterialSystem::GetDynamicMesh()
{
	if (m_freeDynamicMeshes.numElem())
		return IDynamicMeshPtr(&m_dynamicMeshes[m_freeDynamicMeshes.popBack()]);

	const int id = m_dynamicMeshes.numElem();
	CDynamicMesh& dynMesh = m_dynamicMeshes.append();
	if (!dynMesh.Init(id, m_dynamicMeshVertexFormat))
	{
		m_dynamicMeshes.removeIndex(m_dynamicMeshes.numElem() - 1);
		ASSERT_FAIL("Failed to init DynamicMesh");
		return nullptr;
	}

	// NOTE: buffer is released on MeshBuffer::End
	return IDynamicMeshPtr(&dynMesh);
}

void CMaterialSystem::ReleaseDynamicMesh(int id)
{
	ASSERT_MSG(arrayFindIndex(m_freeDynamicMeshes, id), "DynamicMesh already released");
	m_freeDynamicMeshes.append(id);
}

// returns temp buffer with data written. SubmitCommandBuffers uploads it to GPU
GPUBufferView CMaterialSystem::GetTransientUniformBuffer(const void* data, int64 size)
{
	CScopedMutex m(s_matSystemBufferMutex);

	const ShaderAPICapabilities& caps = m_shaderAPI->GetCaps();
	const int bufferAlignment = max(caps.minUniformBufferOffsetAlignment, caps.minStorageBufferOffsetAlignment);
	constexpr int64 maxTransientBufferSize = 128 * 1024;

	TransientBufferCollection& collection = m_transientUniformBuffers;

	const int bufferIndex = collection.bufferIdx;
	IGPUBufferPtr& buffer = collection.buffers[bufferIndex];
	if (!buffer)
		buffer = m_shaderAPI->CreateBuffer(BufferInfo(1, maxTransientBufferSize), BUFFERUSAGE_UNIFORM | BUFFERUSAGE_STORAGE | BUFFERUSAGE_COPY_DST, "TransientUniformBuffer");

	if (data && size > 0)
	{
		if (!m_bufferCmdRecorder)
			m_bufferCmdRecorder = g_renderAPI->CreateCommandRecorder("BufferSubmit");

		const int64 writeOffset = NextBufferOffset(size, collection.bufferOffsets[bufferIndex], maxTransientBufferSize, bufferAlignment);
		m_bufferCmdRecorder->WriteBuffer(buffer, data, size, writeOffset);

		return GPUBufferView(buffer, writeOffset, size);
	}
	return GPUBufferView(buffer, collection.bufferOffsets[bufferIndex]);
}

GPUBufferView CMaterialSystem::GetTransientVertexBuffer(const void* data, int64 size)
{
	CScopedMutex m(s_matSystemBufferMutex);

	const int bufferAlignment = 4; // vertex buffers are aligned always 4 bytes
	constexpr int64 maxTransientBufferSize = 128 * 1024;

	TransientBufferCollection& collection = m_transientVertexBuffers;

	const int bufferIndex = collection.bufferIdx;
	IGPUBufferPtr& buffer = collection.buffers[bufferIndex];
	if (!buffer)
		buffer = m_shaderAPI->CreateBuffer(BufferInfo(1, maxTransientBufferSize), BUFFERUSAGE_VERTEX | BUFFERUSAGE_COPY_DST, "TransientVertexBuffer");

	if (data && size > 0)
	{
		if (!m_bufferCmdRecorder)
			m_bufferCmdRecorder = g_renderAPI->CreateCommandRecorder("BufferSubmit");

		const int64 writeOffset = NextBufferOffset(size, collection.bufferOffsets[bufferIndex], maxTransientBufferSize, bufferAlignment);
		m_bufferCmdRecorder->WriteBuffer(buffer, data, size, writeOffset);

		return GPUBufferView(buffer, writeOffset, size);
	}

	return GPUBufferView(buffer, collection.bufferOffsets[bufferIndex]);
}

void CMaterialSystem::QueueCommandBuffers(ArrayCRef<IGPUCommandBufferPtr> cmdBuffers)
{
	QueueCommitInternalBuffers();
	for (const IGPUCommandBufferPtr& buffer : cmdBuffers)
		m_pendingCmdBuffers.append(buffer);
}

void CMaterialSystem::QueueCommandBuffer(const IGPUCommandBuffer* cmdBuffer)
{
	QueueCommitInternalBuffers();
	m_pendingCmdBuffers.append(IGPUCommandBufferPtr(const_cast<IGPUCommandBuffer*>(cmdBuffer)));
}

void CMaterialSystem::QueueCommitInternalBuffers()
{
	Array<IGPUCommandBufferPtr>& buffers = m_pendingCmdBuffers;

	for (CDynamicMesh& dynMesh : m_dynamicMeshes)
	{
		IGPUCommandBufferPtr submitBuf = dynMesh.GetSubmitBuffer();
		if (!submitBuf)
			continue;
		buffers.append(submitBuf);
	}

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
		m_cameraChangeId += 8192;
	}
}

void CMaterialSystem::SetupDrawCommand(const RenderDrawCmd& drawCmd, const RenderPassContext& passContext)
{
	if (!drawCmd.batchInfo.material)
		return;

	// as it could be called outside of BeginFrame/EndFrame
	FramePrepareInternal();

	const RenderInstanceInfo& instInfo = drawCmd.instanceInfo;
	const MeshInstanceData& instData = instInfo.instData;

	FixedArray<GPUBufferView, MAX_VERTEXSTREAM> bindVertexBuffers;

	uint usedVertexLayoutBits = 0;
	MeshInstanceFormatRef instFormatRef = instInfo.instFormat;
	if (instFormatRef.layout.numElem())
	{
		int bufferSlot = 0;
		for (int i = 0; i < instFormatRef.layout.numElem(); ++i)
		{
			if (instData.buffer && instFormatRef.layout[i].stepMode == VERTEX_STEPMODE_INSTANCE)
			{
				bindVertexBuffers.append(GPUBufferView(instData.buffer, instData.offset, instData.stride * instData.count));
				usedVertexLayoutBits |= (1 << i);
			}
			else if(instInfo.vertexBuffers[i])
			{
				bindVertexBuffers.append(instInfo.vertexBuffers[i]);
				usedVertexLayoutBits |= (1 << i);
			}
		}
	}

	// modify used layout flags
	instFormatRef.usedLayoutBits &= usedVertexLayoutBits;

	const RenderDrawBatch& meshInfo = drawCmd.batchInfo;
	if (!SetupMaterialPipeline(drawCmd.batchInfo.material, drawCmd.instanceInfo.uniformBuffers, meshInfo.primTopology, instFormatRef, passContext))
		return;

	if (instFormatRef.layout.numElem())
	{
		IMatSystemShader* matShader = static_cast<CMaterial*>(drawCmd.batchInfo.material)->m_shader;
		int bufferSlot = 0;
		for (const GPUBufferView& bindBufferView : bindVertexBuffers)
			passContext.recorder->SetVertexBufferView(bufferSlot++, bindBufferView);

		passContext.recorder->SetIndexBufferView(instInfo.indexBuffer, instInfo.indexFormat);
	}

	if (meshInfo.firstIndex < 0 && meshInfo.numIndices == 0)
		passContext.recorder->Draw(meshInfo.numVertices, meshInfo.firstVertex, instData.count, instData.first);
	else
		passContext.recorder->DrawIndexed(meshInfo.numIndices, meshInfo.firstIndex, instData.count, meshInfo.baseVertex, instData.first);
}

void CMaterialSystem::UpdateMaterialProxies(IMaterial* material, IGPUCommandRecorder* commandRecorder, bool force) const
{
	if (!material)
		return;

	CMaterial* matSysMaterial = static_cast<CMaterial*>(material);

	const uint proxyFrame = m_frame;
	if (!force && matSysMaterial->m_frameBound == proxyFrame)
		return;

	matSysMaterial->UpdateProxy(m_proxyDeltaTime, commandRecorder);
	matSysMaterial->m_frameBound = proxyFrame;
}

bool CMaterialSystem::SetupMaterialPipeline(IMaterial* material, ArrayCRef<RenderBufferInfo> uniformBuffers, EPrimTopology primTopology, const MeshInstanceFormatRef& meshInstFormat, const RenderPassContext& passContext)
{
	IShaderAPI* renderAPI = m_shaderAPI;

	// TODO: overdraw material. Or maybe debug property in shader?
	IMaterialPtr originalMaterial;
	if (passContext.beforeMaterialSetup)
	{
		originalMaterial.Assign(material);
		material = passContext.beforeMaterialSetup(material);

		if(material == originalMaterial)
			originalMaterial = nullptr;
	}

	if (!material)
		return false;

	// force load shader and textures if not already
	material->LoadShaderAndTextures();

	CMaterial* matSysMaterial = static_cast<CMaterial*>(material);
	IMatSystemShader* matShader = matSysMaterial->m_shader;

	if (!matShader)
		return false;

	UpdateMaterialProxies(material, m_proxyUpdateCmdRecorder);

	const uint proxyFrame = m_frame;
	if (matSysMaterial->m_frameBound != proxyFrame)
	{
		matSysMaterial->UpdateProxy(m_proxyDeltaTime, m_proxyUpdateCmdRecorder);
		matSysMaterial->m_frameBound = proxyFrame;
	}

	const IMatSystemShader::PipelineInputParams pipelineInputParams {
		passContext.recorder->GetRenderTargetFormats(),
		passContext.recorder->GetDepthTargetFormat(),
		meshInstFormat,
		primTopology,
		CULL_BACK,
		passContext.recorder->GetTargetMultiSamples(),
		0xffffffff, // TODO: msaa
		false, // TODO: msaa
		passContext.recorder->IsDepthReadOnly()
	};

	return matShader->SetupRenderPass(renderAPI, pipelineInputParams, uniformBuffers, passContext, originalMaterial);
}

bool CMaterialSystem::SetupDrawDefaultUP(EPrimTopology primTopology, int vertFVF, const void* verts, int numVerts, const RenderPassContext& passContext)
{
	const CMaterial* material = static_cast<CMaterial*>(GetDefaultMaterial().Ptr());
	IMatSystemShader* matShader = material->m_shader;
	if (!matShader)
		return false;

	// as it could be called outside of BeginFrame/EndFrame
	FramePrepareInternal();

	ASSERT_MSG(passContext.data && passContext.data->type == RENDERPASS_DEFAULT, "RenderPassContext for DrawDefaultUP must always have MatSysDefaultRenderPass");

	ASSERT_MSG(vertFVF & VERTEX_FVF_XYZ, "DrawDefaultUP must have FVF_XYZ in vertex flags");

	CMeshBuilder meshBuilder(GetDynamicMesh());
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

	const RenderInstanceInfo& instInfo = drawCmd.instanceInfo;
	const MeshInstanceData& instData = instInfo.instData;
	const RenderDrawBatch& meshInfo = drawCmd.batchInfo;

	// material must support correct vertex layout state
	uint usedVertexLayoutBits = 0;
	MeshInstanceFormatRef instFormatRef = instInfo.instFormat;
	if (instFormatRef.layout.numElem())
	{
		for (int i = 0; i < instFormatRef.layout.numElem(); ++i)
		{
			if (instData.buffer && instFormatRef.layout[i].stepMode == VERTEX_STEPMODE_INSTANCE)
			{
				ASSERT_FAIL("DrawDefaultUP does not support instancing yet");
			}
			else
				usedVertexLayoutBits |= instInfo.vertexBuffers[i] ? (1 << i) : 0;
		}
	}

	// modify used layout flags
	instFormatRef.usedLayoutBits &= usedVertexLayoutBits;

	const IMatSystemShader::PipelineInputParams pipelineInputParams {
		passContext.recorder->GetRenderTargetFormats(),
		passContext.recorder->GetDepthTargetFormat(),
		instFormatRef,
		primTopology,
		CULL_BACK,
		1, // TODO: msaa
		0xffffffff, // TODO: msaa
		false, // TODO: msaa
		passContext.recorder->IsDepthReadOnly()
	};

	IShaderAPI* renderAPI = m_shaderAPI;
	if (!matShader->SetupRenderPass(renderAPI, pipelineInputParams, nullptr, passContext, nullptr))
	{
		return false;
	}

	if (instFormatRef.layout.numElem())
	{
		for (int i = 0; i < instInfo.vertexBuffers.numElem(); ++i)
			passContext.recorder->SetVertexBufferView(i, instInfo.vertexBuffers[i]);
	}

	const MatSysDefaultRenderPass& rendPassInfo = *reinterpret_cast<const MatSysDefaultRenderPass*>(passContext.data);

	if (rendPassInfo.scissorRectangle.leftTop != IVector2D(-1, -1) && rendPassInfo.scissorRectangle.rightBottom != IVector2D(-1, -1))
		passContext.recorder->SetScissorRectangle(rendPassInfo.scissorRectangle);

	if (meshInfo.firstIndex < 0 && meshInfo.numIndices == 0)
		passContext.recorder->Draw(meshInfo.numVertices, meshInfo.firstVertex, instData.count, instData.first);
	else
		passContext.recorder->DrawIndexed(meshInfo.numIndices, meshInfo.firstIndex, instData.count, meshInfo.baseVertex, instData.first);

	return true;
}

// use this if you have objects that must be destroyed when device is lost
void CMaterialSystem::AddDestroyLostCallbacks(DEVICE_LOST_RESTORE_CB destroy, DEVICE_LOST_RESTORE_CB restore)
{
	if(destroy)
		m_lostDeviceCb.addUnique(destroy);

	if(restore)
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
void CMaterialSystem::RemoveLostRestoreCallbacks(DEVICE_LOST_RESTORE_CB destroy, DEVICE_LOST_RESTORE_CB restore)
{
	m_lostDeviceCb.remove(destroy);
	m_restoreDeviceCb.remove(restore);
}
