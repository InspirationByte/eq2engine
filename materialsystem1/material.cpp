//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

ConVar r_allowSourceTextures("r_allowSourceTextures", "0", "enable materials and textures loading from source paths", 0);

#define MATERIAL_FILE_EXTENSION		".mat"
#define ATLAS_FILE_EXTENSION		".atlas"

CMaterial::CMaterial(Threading::CEqMutex& mutex) 
	: m_state(MATERIAL_LOAD_ERROR),
	m_shader(nullptr), 
	m_loadFromDisk(true),
	m_frameBound(0),
	m_atlas(nullptr), 
	m_Mutex(mutex),
	m_nameHash(0)
{
}

CMaterial::~CMaterial()
{

}

//
// Initializes the shader
//
void CMaterial::Init(const char* materialPath)
{
	if (materialPath)
	{
		m_szMaterialName = materialPath;
		m_nameHash = StringToHash(materialPath, true);
	}

	m_loadFromDisk = true;

	DevMsg(DEVMSG_MATSYSTEM, "Loading material '%s'\n", m_szMaterialName.ToCString());

	const char* materialsPath = materials->GetMaterialPath();
	const char* materialsSRCPath = materials->GetMaterialSRCPath();

	const int materialSearchPath = (SP_DATA | SP_MOD);

	const char* materialsPaths[2] = {
		materialsPath, materialsSRCPath
	};

	bool success = false;
	KVSection root;

	int doMaterialPaths = materials->GetConfiguration().editormode;

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

		m_state = MATERIAL_LOAD_NEED_LOAD;
		return;
	}

	KVSection* shader_root = root.keys.numElem() ? root.keys[0] : nullptr;

	if(!shader_root)
	{
		MsgError("Material '%s' does not have a shader root section!\n",m_szMaterialName.ToCString());
		m_state = MATERIAL_LOAD_ERROR;
		return;
	}

	// section name is used as shader name
	m_szShaderName = shader_root->name;

	// begin initialization
	InitVars( shader_root );

	InitShader();
}

// initializes material from keyvalues
void CMaterial::Init(const char* materialName, KVSection* shader_root)
{
	m_szMaterialName = materialName;
	m_nameHash = StringToHash(materialName, true);

	m_loadFromDisk = false;

	// section name is used as shader name
	m_szShaderName = shader_root->name;

	// begin initialization
	InitVars( shader_root );

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

			// add to list
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
void CMaterial::InitMaterialVars(KVSection* kvs)
{
	// init material vars
	for(int i = 0; i < kvs->keys.numElem();i++)
	{
		// ignore sections
		if( kvs->keys[i]->IsSection() )
			continue;

		// ignore some preserved vars
		if( !stricmp(kvs->keys[i]->GetName(), "Shader") )
			continue;

		KVSection* materialVar = kvs->keys[i];

		// initialize material var by this
		IMatVar* pMatVar = FindMaterialVar(materialVar->GetName());

		if (!pMatVar)
		{
			CMatVar* pVar = PPNew CMatVar();
			pVar->Init(materialVar->GetName(), KV_GetValueString(materialVar));

			{
				Threading::CScopedMutex m(m_Mutex);
				m_variables.append(pVar);
			}
		}
		else
		{
			pMatVar->SetString(KV_GetValueString(materialVar));
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
		m_state = MATERIAL_LOAD_OK;
		return;
	}

	IMaterialSystemShader* shader = materials->CreateShaderInstance(m_szShaderName.GetData());

	// if not found - try make Error shader
	if(!shader)// || (m_shader && !stricmp(m_shader->GetName(), "Error")))
	{
		MsgError("Invalid shader '%s' specified for material %s!\n",m_szShaderName.GetData(),m_szMaterialName.GetData());

		if(!shader)
			shader = materials->CreateShaderInstance("Error");
	}

	if(shader)
	{
		// just init the parameters
		shader->Init( this );
		m_state = MATERIAL_LOAD_NEED_LOAD;
	}
	else
		m_state = MATERIAL_LOAD_ERROR;

	m_shader = shader;
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
			InitMaterialVars( editorPrefs );
		}
	}

	// init material proxies
	KVSection* proxy_sec = shader_root->FindSection("MaterialProxy", KV_FLAG_SECTION);
	InitMaterialProxy( proxy_sec );
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

	{
		Threading::CScopedMutex m(m_Mutex);
		m_state = MATERIAL_LOAD_INQUEUE;
	}

	// try init
	if(!shader->IsInitialized() && !shader->IsError())
	{
		shader->InitTextures();
		shader->InitShader();
	}

	if(shader->IsInitialized() )
		m_state = MATERIAL_LOAD_OK;
	else if(shader->IsError() )
		m_state = MATERIAL_LOAD_ERROR;
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

IMatVar *CMaterial::GetMaterialVar(const char* pszVarName, const char* defaultparameter)
{
	return CreateMaterialVar(pszVarName, defaultparameter);
}

IMatVar *CMaterial::FindMaterialVar(const char* pszVarName) const
{
	const int nameHash = StringToHash(pszVarName, true);

	{
		Threading::CScopedMutex m(m_Mutex);
	
		for(int i = 0; i < m_variables.numElem(); i++)
		{
			if(m_variables[i]->m_nameHash == nameHash)
				return m_variables[i];
		}
	}

	return nullptr;
}

ITexture* CMaterial::GetBaseTexture(int stage)
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

// creates or finds existing material vars
IMatVar* CMaterial::CreateMaterialVar(const char* pszVarName, const char* defaultParam)
{
	IMatVar *pMatVar = FindMaterialVar(pszVarName);

	if(!pMatVar)
	{
		CMatVar *pVar = PPNew CMatVar();
		pVar->Init(pszVarName, defaultParam);

		{
			Threading::CScopedMutex m(m_Mutex);
			m_variables.append(pVar);
		}

		pMatVar = pVar;
	}

	return pMatVar;
}

// remove material var
void CMaterial::RemoveMaterialVar(IMatVar* pVar)
{
	Threading::CScopedMutex m(m_Mutex);

	if (m_variables.fastRemove((CMatVar*)pVar))
		delete pVar;
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
		for(int i = 0; i < m_variables.numElem();i++)
			delete m_variables[i];

		m_variables.clear();

		delete m_atlas;
		m_atlas = nullptr;
	}

	// always drop proxies
	{
		Threading::CScopedMutex m(m_Mutex);
		for (int i = 0; i < m_proxies.numElem(); i++)
			delete m_proxies[i];

		m_proxies.clear();
	}

	m_state = MATERIAL_LOAD_NEED_LOAD;
}

void CMaterial::UpdateProxy(float fDt)
{
	Threading::CScopedMutex m(m_Mutex);
	for(int i = 0; i < m_proxies.numElem(); i++)
		m_proxies[i]->UpdateProxy( fDt );
}

void CMaterial::Setup(uint paramMask)
{
	// shaders and textures needs to be reset
	if(GetFlags() & MATERIAL_FLAG_BASETEXTURE_CUR)
		g_pShaderAPI->Reset( STATE_RESET_SHADER );
	else
		g_pShaderAPI->Reset( STATE_RESET_SHADER | STATE_RESET_TEX );

	IMaterialSystemShader* shader = m_shader;

	shader->SetupShader();
	shader->SetupConstants( paramMask );
}
