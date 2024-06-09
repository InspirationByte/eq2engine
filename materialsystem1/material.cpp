//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "utils/TextureAtlas.h"
#include "utils/KeyValues.h"
#include "materialvar.h"
#include "materialsystem1/IMaterialSystem.h"
#include "material.h"

DECLARE_CVAR(r_allowSourceTextures, "0", "enable materials and textures loading from source paths", CV_CHEAT);

using namespace Threading;
static CEqMutex s_materialVarMutex;

CMaterial::CMaterial(const char* materialName, int instanceFormatId, bool loadFromDisk)
	: m_loadFromDisk(loadFromDisk)
{
	m_szMaterialName = materialName;
	m_nameHash = StringToHash(m_szMaterialName, true);
	m_instanceFormatId = instanceFormatId;
}

CMaterial::~CMaterial()
{
	Cleanup();
}

void CMaterial::Ref_DeleteObject()
{
	g_matSystem->FreeMaterial(this);
	RefCountedObject::Ref_DeleteObject();
}

//
// Initializes the shader
//
void CMaterial::Init(IShaderAPI* renderAPI)
{
	ASSERT(m_loadFromDisk == true);

	DevMsg(DEVMSG_MATSYSTEM, "Loading material '%s'\n", m_szMaterialName.ToCString());

	const char* materialsPath = g_matSystem->GetMaterialPath();
	const char* materialsSRCPath = g_matSystem->GetMaterialSRCPath();

	const int materialSearchPath = (SP_DATA | SP_MOD);

	const char* materialsPaths[2] = {
		materialsPath, materialsSRCPath
	};

	bool success = false;
	KVSection root;

	const int numSteps = r_allowSourceTextures.GetBool() ? 2 : 1;

	for (int i = 0; i < numSteps && !success; ++i)
	{
		//
		// loading a material description
		//
		EqString atlasKVSFileName;
		fnmPathCombine(atlasKVSFileName, materialsPaths[i], fnmPathApplyExt(m_szMaterialName, s_materialAtlasFileExt));

		EqString materialKVSFilename;
		fnmPathCombine(materialKVSFilename, materialsPaths[i], fnmPathApplyExt(m_szMaterialName, s_materialFileExt));

		// load atlas file
		if (!m_atlas)
		{
			KVSection atlRoot;
			if (KV_LoadFromFile(atlasKVSFileName, materialSearchPath, &atlRoot))
			{
				const KVSection* atlasSec = atlRoot["AtlasGroup"];
				if (atlasSec)
				{
					m_atlas = PPNew CTextureAtlas(atlasSec);

					// atlas can override material name
					fnmPathCombine(materialKVSFilename, materialsPaths[i], fnmPathApplyExt(m_atlas->GetMaterialName(), s_materialFileExt));
				}
				else
					MsgError("Invalid atlas file '%s'\n", atlasKVSFileName.ToCString());
			}
		}

		// load material file
		if( KV_LoadFromFile(materialKVSFilename.ToCString(), materialSearchPath, &root))
		{
			success = true;
		}
		else
		{
			SAFE_DELETE(m_atlas);
		}
	}

	if (!success)
	{
		MsgError("Can't load material '%s'\n", m_szMaterialName.ToCString());

		// TODO: pick valid error shader based on Vertex Layout ID
		m_szShaderName = "Error";

		Atomic::Exchange(m_state, MATERIAL_LOAD_NEED_LOAD);
		return;
	}

	const KVSection* shaderRoot = root.keys.numElem() ? root.keys[0] : nullptr;
	if(!shaderRoot)
	{
		MsgError("Material '%s' does not have a shader root section!\n",m_szMaterialName.ToCString());
		Atomic::Exchange(m_state, MATERIAL_LOAD_ERROR);
		return;
	}

	// section name is used as shader name
	m_szShaderName = shaderRoot->name;

	// begin initialization
	InitVars( shaderRoot, renderAPI->GetRendererName() );
	InitShader(g_matSystem->GetShaderAPI());
}

// initializes material from keyvalues
void CMaterial::Init(IShaderAPI* renderAPI, const KVSection* shaderRoot)
{
	if(shaderRoot)
		ASSERT(m_loadFromDisk == false);

	if (shaderRoot)
	{
		// section name is used as shader name
		m_szShaderName = shaderRoot->name;

		// begin initialization
		InitVars(shaderRoot, renderAPI->GetRendererName());
	}

	InitShader(renderAPI);
}

//
// Initializes the proxy data from section
//
void CMaterial::InitMaterialProxy(const KVSection* proxySec)
{
	if(!proxySec)
		return;

	// try any kind of proxy
	for(const KVSection* proxyItemSec : proxySec->Keys())
	{
		IMaterialProxy* pProxy = g_matSystem->CreateProxyByName(proxyItemSec->name );

		if(pProxy)
		{
			// initialize proxy
			pProxy->InitProxy( this, proxyItemSec);
			m_proxies.append( pProxy );
		}
		else
		{
			MsgWarning("Unknown proxy '%s' for material %s!\n", proxyItemSec->GetName(), m_szMaterialName.GetData());
		}
	}
}

//
// Initializes the material vars from the keyvalues section
//
void CMaterial::InitMaterialVars(const KVSection* kvs, const char* prefix)
{
	int numMaterialVars = 0;

	for (const KVSection* materialVarSec : kvs->Keys())
	{
		if (materialVarSec->IsSection())
			continue;

		// ignore some preserved vars
		if (!CString::CompareCaseIns(materialVarSec->GetName(), "Shader"))
			continue;

		++numMaterialVars;
	}

	m_vars.variables.reserve(numMaterialVars);

	// init material vars
	for(const KVSection* materialVarSec : kvs->Keys())
	{
		if(materialVarSec->IsSection() )
			continue;

		// ignore some preserved vars
		if( !CString::CompareCaseIns(materialVarSec->GetName(), "Shader") )
			continue;

		const EqString matVarName(prefix ? EqString::Format("%s.%s", prefix, materialVarSec->GetName()) : _Es(materialVarSec->GetName()));

		// initialize material var by this
		const int nameHash = StringToHash(matVarName, true);

		{
			CScopedMutex m(s_materialVarMutex);
			auto it = m_vars.variableMap.find(nameHash);
			if (it.atEnd())
			{
				const int varId = m_vars.variables.numElem();
				MatVarData& newVar = m_vars.variables.append();
				MatVarHelper::Init(newVar, KV_GetValueString(materialVarSec));
				m_vars.variableMap.insert(nameHash, varId);
			}
			else
			{
				MatVarHelper::SetString(m_vars.variables[*it], KV_GetValueString(materialVarSec));
			}
		}
	}
}

//
// Initializes the shader
//
void CMaterial::InitShader(IShaderAPI* renderAPI)
{
	if(m_shader)
		return;

	PROF_EVENT_F();

	const ShaderFactory* shaderFactory = g_matSystem->GetShaderFactory(m_szShaderName.GetData(), m_instanceFormatId);
	if (!shaderFactory)
	{
		MsgError("Invalid shader '%s' specified for material %s!\n", m_szShaderName.GetData(), m_szMaterialName.GetData());
		shaderFactory = g_matSystem->GetShaderFactory("Error", m_instanceFormatId);
	}

	if(shaderFactory && arrayFindIndex(shaderFactory->vertexLayoutIds, m_instanceFormatId) == -1)
	{
		MsgError("Vertex instance format is unsupported by shader '%s' specified for material '%s'\n", shaderFactory->shaderName, m_szMaterialName.GetData());
		shaderFactory = g_matSystem->GetShaderFactory("Error", m_instanceFormatId);
	}
	ASSERT_MSG(shaderFactory, "Error shader is not registered or overrides hasn't been set up");

	if (shaderFactory)
	{
		m_shader = shaderFactory->func();
		m_shader->Init(renderAPI, this);
		Atomic::Exchange(m_state, MATERIAL_LOAD_NEED_LOAD);
	}
	else
		Atomic::Exchange(m_state, MATERIAL_LOAD_ERROR);
}

//
// Initializes material vars and shader name
//
void CMaterial::InitVars(const KVSection* shaderRoot, const char* renderAPIName)
{
	// Get an API preferences
	const KVSection* apiPrefs = shaderRoot->FindSection(EqString::Format("API_%s", renderAPIName).ToCString(), KV_FLAG_SECTION);

	// init root material vars
	InitMaterialVars( shaderRoot );

	//
	// API preference lookup
	//
	if(apiPrefs)
	{
		const KVSection* pPair = apiPrefs->FindSection("Shader");

		// Set shader name from API prefs if available
		if(pPair)
			m_szShaderName = KV_GetValueString(pPair);

		// init material vars from api preferences if have some
		InitMaterialVars( apiPrefs );
	}

	//
	// if shader has an editor section we should override it here
	//
	if(g_matSystem->GetConfiguration().editormode)
	{
		const KVSection* editorPrefs = shaderRoot->FindSection("editor", KV_FLAG_SECTION);

		// API preference lookup
		if(editorPrefs)
		{
			const KVSection* pPair = editorPrefs->FindSection("Shader");

			// Set shader name from API prefs if available
			if(pPair)
				m_szShaderName = KV_GetValueString(pPair);

			// init material vars from editor overrides if have some
			// see BaseShader on how it handles editor prefix
			InitMaterialVars( editorPrefs, "editor");
		}
	}

	// init material proxies
	const KVSection* proxySec = shaderRoot->FindSection("MaterialProxy", KV_FLAG_SECTION);
	InitMaterialProxy(proxySec);
}

MatVarData& CMaterial::VarAt(int idx) const
{
	return const_cast<MatVarData&>(m_vars.variables[idx]);
}

bool CMaterial::LoadShaderAndTextures()
{
	if (m_state != MATERIAL_LOAD_NEED_LOAD)
		return false;

	return DoLoadShaderAndTextures();
}

bool CMaterial::DoLoadShaderAndTextures()
{
	IShaderAPI* renderAPI = g_matSystem->GetShaderAPI();
	InitShader(renderAPI);

	IMatSystemShader* shader = m_shader;
	if(!shader)
		return true;

	Atomic::Exchange(m_state, MATERIAL_LOAD_INQUEUE);

	// try init
	if(!shader->IsInitialized())
	{
		shader->InitTextures(renderAPI);
		shader->InitShader(renderAPI);
	}

	if(shader->IsInitialized() )
		Atomic::Exchange(m_state, MATERIAL_LOAD_OK);
	else
		ASSERT_FAIL("please check shader '%s' (%s) for initialization (not error, not initialized)", m_szShaderName.ToCString(), m_shader->GetName());

	return true;
}

// waits for material loading
void CMaterial::WaitForLoading() const
{
	do{
		Threading::YieldCurrentThread();
	} while(m_state == MATERIAL_LOAD_INQUEUE);
}

MatVarProxyUnk CMaterial::GetMaterialVar(const char* pszVarName, const char* defaultValue)
{
	CScopedMutex m(s_materialVarMutex);

	const int nameHash = StringToHash(pszVarName, true);

	auto it = m_vars.variableMap.find(nameHash);
	if (!it.atEnd())
		return MatVarProxyUnk(*it, m_vars);

	const int varId = m_vars.variables.numElem();
	MatVarData& newVar = m_vars.variables.append();
	MatVarHelper::Init(newVar, defaultValue);

	m_vars.variableMap.insert(nameHash, varId);

	return MatVarProxyUnk(varId, m_vars);
}

MatVarProxyUnk CMaterial::FindMaterialVar(const char* pszVarName) const
{
	const int nameHash = StringToHash(pszVarName, true);

	CScopedMutex m(s_materialVarMutex);
	auto it = m_vars.variableMap.find(nameHash);
	if (it.atEnd())
		return MatVarProxyUnk();

	return MatVarProxyUnk(*it, *const_cast<MaterialVarBlock*>(&m_vars));
}

const ITexturePtr& CMaterial::GetBaseTexture(int stage)
{
	if(m_shader != nullptr && !IsError())
	{
		// try load
		DoLoadShaderAndTextures();

		// wait if it was loading in another thread
		WaitForLoading();

		return m_shader->GetBaseTexture(stage);
	}
	else
	{
		// Return error texture
		return g_matSystem->GetErrorCheckerboardTexture();
	}
}

int CMaterial::GetFlags() const
{
	if(!m_shader)
		return 0;

	return m_shader->GetFlags();
}

const char*	CMaterial::GetShaderName() const
{
	if(!m_shader)
		return m_szShaderName.ToCString();

	return m_shader->GetName();
}

CTextureAtlas* CMaterial::GetAtlas() const
{
	return m_atlas;
}

void CMaterial::Cleanup(bool dropVars, bool dropShader)
{
	WaitForLoading();
	
	if(dropShader && m_shader)
	{
		m_shader->Unload();
		SAFE_DELETE(m_shader);
	}

	if(dropVars)
	{
		CScopedMutex m(s_materialVarMutex);
		m_vars.variables.clear(true);
		m_vars.variableMap.clear(true);
		SAFE_DELETE(m_atlas);
	}

	// always drop proxies
	for (int i = 0; i < m_proxies.numElem(); i++)
		delete m_proxies[i];
	m_proxies.clear(true);

	Atomic::Exchange(m_state, MATERIAL_LOAD_NEED_LOAD);
}

void CMaterial::UpdateProxy(float fDt, IGPUCommandRecorder* cmdRecorder)
{
	if (!m_shader)
		return;

	for(IMaterialProxy* proxy : m_proxies)
		proxy->UpdateProxy(fDt);

	m_shader->UpdateProxy(cmdRecorder);
}
