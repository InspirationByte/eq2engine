//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium material
//
// TODO:	mat-file serialization for material editor purposes and other issues
//			serialize material file to memory buffer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "utils/TextureAtlas.h"
#include "utils/KeyValues.h"
#include "materialvar.h"
#include "materialsystem1/IMaterialSystem.h"
#include "material.h"

DECLARE_CVAR(r_allowSourceTextures, "0", "enable materials and textures loading from source paths", 0);

#define MATERIAL_FILE_EXTENSION		".mat"
#define ATLAS_FILE_EXTENSION		".atlas"

using namespace Threading;
static CEqMutex s_materialVarMutex;

CMaterial::CMaterial(const char* materialName, bool loadFromDisk)
	: m_loadFromDisk(loadFromDisk)
{
	m_szMaterialName = materialName;
	m_nameHash = StringToHash(m_szMaterialName, true);
}

CMaterial::~CMaterial()
{
	Cleanup();
}

void CMaterial::Ref_DeleteObject()
{
	materials->FreeMaterial(this);
	delete this;
}

//
// Initializes the shader
//
void CMaterial::Init()
{
	ASSERT(m_loadFromDisk == true);

	DevMsg(DEVMSG_MATSYSTEM, "Loading material '%s'\n", m_szMaterialName.ToCString());

	const char* materialsPath = materials->GetMaterialPath();
	const char* materialsSRCPath = materials->GetMaterialSRCPath();

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
		EqString atlasKVSFileName(materialsPaths[i] + m_szMaterialName + ATLAS_FILE_EXTENSION);
		EqString materialKVSFilename(materialsPaths[i] + m_szMaterialName + MATERIAL_FILE_EXTENSION);

		// load atlas file
		if (g_fileSystem->FileExist(atlasKVSFileName, materialSearchPath))
		{
			KVSection root;
			if (KV_LoadFromFile(atlasKVSFileName, materialSearchPath, &root))
			{
				KVSection* atlasSec = root.FindSection("atlasgroup");

				if (atlasSec)
				{
					if (!m_atlas)
						m_atlas = PPNew CTextureAtlas(atlasSec);

					root.Cleanup();

					// atlas can override material name
					materialKVSFilename = (materialsPaths[i] + _Es(m_atlas->GetMaterialName()) + MATERIAL_FILE_EXTENSION);
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
	}

	if (!success)
	{
		MsgError("Can't load material '%s'\n", m_szMaterialName.ToCString());
		m_szShaderName = "Error";

		Atomic::Exchange(m_state, MATERIAL_LOAD_NEED_LOAD);
		return;
	}

	KVSection* shader_root = root.keys.numElem() ? root.keys[0] : nullptr;

	if(!shader_root)
	{
		MsgError("Material '%s' does not have a shader root section!\n",m_szMaterialName.ToCString());
		Atomic::Exchange(m_state, MATERIAL_LOAD_ERROR);
		return;
	}

	// section name is used as shader name
	m_szShaderName = shader_root->name;

	// begin initialization
	InitVars( shader_root );
	InitShader();
}

// initializes material from keyvalues
void CMaterial::Init(KVSection* shader_root)
{
	if(shader_root)
		ASSERT(m_loadFromDisk == false);

	if (shader_root)
	{
		// section name is used as shader name
		m_szShaderName = shader_root->name;

		// begin initialization
		InitVars(shader_root);
	}

	InitShader();
}

//
// Initializes the proxy data from section
//
void CMaterial::InitMaterialProxy(KVSection* proxySec)
{
	if(!proxySec)
		return;

	// try any kind of proxy
	for(int i = 0; i < proxySec->keys.numElem();i++)
	{
		IMaterialProxy* pProxy = materials->CreateProxyByName( proxySec->keys[i]->name );

		if(pProxy)
		{
			// initialize proxy
			pProxy->InitProxy( this, proxySec->keys[i] );
			m_proxies.append( pProxy );
		}
		else
		{
			MsgWarning("Unknown proxy '%s' for material %s!\n", proxySec->keys[i]->name, m_szMaterialName.GetData());
		}
	}
}

//
// Initializes the material vars from the keyvalues section
//
void CMaterial::InitMaterialVars(KVSection* kvs, const char* prefix)
{
	int numMaterialVars = 0;

	for (int i = 0; i < kvs->keys.numElem(); i++)
	{
		KVSection* materialVarSec = kvs->keys[i];

		if (materialVarSec->IsSection())
			continue;

		// ignore some preserved vars
		if (!stricmp(materialVarSec->GetName(), "Shader"))
			continue;

		++numMaterialVars;
	}

	m_vars.variables.reserve(numMaterialVars);

	// init material vars
	for(int i = 0; i < kvs->keys.numElem();i++)
	{
		KVSection* materialVarSec = kvs->keys[i];

		if(materialVarSec->IsSection() )
			continue;

		// ignore some preserved vars
		if( !stricmp(materialVarSec->GetName(), "Shader") )
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
void CMaterial::InitShader()
{
	if( m_shader != nullptr)
		return;

	// don't need to do anything if NODRAW
	if( !m_szShaderName.CompareCaseIns("NODRAW") )
	{
		Atomic::Exchange(m_state, MATERIAL_LOAD_OK);
		return;
	}

	{
		PROF_EVENT("MatSystem Load Material InitShader");

		IMaterialSystemShader* shader = materials->CreateShaderInstance(m_szShaderName.GetData());

		// if not found - try make Error shader
		if (!shader)// || (m_shader && !stricmp(m_shader->GetName(), "Error")))
		{
			MsgError("Invalid shader '%s' specified for material %s!\n", m_szShaderName.GetData(), m_szMaterialName.GetData());

			if (!shader)
				shader = materials->CreateShaderInstance("Error");
		}

		if (shader)
		{
			// just init the parameters
			shader->Init(this);
			Atomic::Exchange(m_state, MATERIAL_LOAD_NEED_LOAD);
		}
		else
			Atomic::Exchange(m_state, MATERIAL_LOAD_ERROR);

		m_shader = shader;
	}
}

//
// Initializes material vars and shader name
//
void CMaterial::InitVars(KVSection* shader_root)
{
	// Get an API preferences
	KVSection* apiPrefs = shader_root->FindSection(EqString::Format("API_%s", g_pShaderAPI->GetRendererName()).ToCString(), KV_FLAG_SECTION);

	// init root material vars
	InitMaterialVars( shader_root );

	//
	// API preference lookup
	//
	if(apiPrefs)
	{
		KVSection* pPair = apiPrefs->FindSection("Shader");

		// Set shader name from API prefs if available
		if(pPair)
			m_szShaderName = KV_GetValueString(pPair);

		// init material vars from api preferences if have some
		InitMaterialVars( apiPrefs );
	}

	//
	// if shader has an editor section we should override it here
	//
	if(materials->GetConfiguration().editormode)
	{
		KVSection* editorPrefs = shader_root->FindSection("editor", KV_FLAG_SECTION);

		// API preference lookup
		if(editorPrefs)
		{
			KVSection* pPair = editorPrefs->FindSection("Shader");

			// Set shader name from API prefs if available
			if(pPair)
				m_szShaderName = KV_GetValueString(pPair);

			// init material vars from editor overrides if have some
			// see BaseShader on how it handles editor prefix
			InitMaterialVars( editorPrefs, "editor");
		}
	}

	// init material proxies
	KVSection* proxy_sec = shader_root->FindSection("MaterialProxy", KV_FLAG_SECTION);
	InitMaterialProxy( proxy_sec );
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
	InitShader();

	IMaterialSystemShader* shader = m_shader;
	if(!shader)
		return true;

	Atomic::Exchange(m_state, MATERIAL_LOAD_INQUEUE);

	// try init
	if(!shader->IsInitialized() && !shader->IsError())
	{
		PROF_EVENT("MatSystem Load Material Shader and Textures");
		shader->InitTextures();
		shader->InitShader();
	}

	if(shader->IsInitialized() )
		Atomic::Exchange(m_state, MATERIAL_LOAD_OK);
	else if(shader->IsError() )
		Atomic::Exchange(m_state, MATERIAL_LOAD_ERROR);
	else
		ASSERT_FAIL("please check shader '%s' (%s) for initialization (not error, not initialized)", m_szShaderName.ToCString(), m_shader->GetName());

	return true;
}

// waits for material loading
void CMaterial::WaitForLoading() const
{
	do{
		Threading::Yield();
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
		return g_pShaderAPI->GetErrorTexture();
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
	
	// drop shader if we need
	if(dropShader && m_shader)
	{
		m_shader->Unload();

		delete m_shader;
		m_shader = nullptr;
	}

	if(dropVars)
	{
		CScopedMutex m(s_materialVarMutex);

		m_vars.variables.clear(true);
		m_vars.variableMap.clear(true);

		delete m_atlas;
		m_atlas = nullptr;
	}

	// always drop proxies
	for (int i = 0; i < m_proxies.numElem(); i++)
		delete m_proxies[i];
	m_proxies.clear(true);

	Atomic::Exchange(m_state, MATERIAL_LOAD_NEED_LOAD);
}

void CMaterial::UpdateProxy(float fDt)
{
	for(int i = 0; i < m_proxies.numElem(); i++)
		m_proxies[i]->UpdateProxy( fDt );
}

void CMaterial::Setup(uint paramMask)
{
	// shaders and textures needs to be reset
	g_pShaderAPI->Reset( STATE_RESET_SHADER | STATE_RESET_TEX );

	IMaterialSystemShader* shader = m_shader;

	shader->SetupShader();
	shader->SetupConstants( paramMask );
}
