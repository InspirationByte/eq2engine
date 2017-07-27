//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech material
//
// TODO:	mat-file serialization for material editor purposes and other issues
//			serialize material file to memory buffer
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"

#include "material.h"
#include "utils/strtools.h"
#include "utils/eqthread.h"
#include "BaseShader.h"
#include "IFileSystem.h"

#define MATERIAL_FILE_EXTENSION		".mat"

CMaterial::CMaterial() : m_state(MATERIAL_LOAD_ERROR), m_pShader(nullptr), m_proxyIsDirty(true), m_frameBound(0)
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
	if(materialPath != NULL)
	{
		m_szMaterialName = materialPath;
		m_szMaterialName.Path_FixSlashes();

		if( m_szMaterialName.c_str()[0] == CORRECT_PATH_SEPARATOR )
			m_szMaterialName = m_szMaterialName.c_str()+1;
	}

	DevMsg(DEVMSG_MATSYSTEM, "Loading material '%s'\n", m_szMaterialName.c_str());

	//
	// load a material file
	//
	EqString materialKVSFilename(materials->GetMaterialPath() + m_szMaterialName + MATERIAL_FILE_EXTENSION);

	kvkeybase_t root;
	if( !KV_LoadFromFile(materialKVSFilename.c_str(), (SP_DATA | SP_MOD), &root))
	{
		MsgError("Can't load material '%s'\n", m_szMaterialName.c_str());
		m_szShaderName = "Error";

		m_state = MATERIAL_LOAD_NEED_LOAD;
		return;
	}

	kvkeybase_t* shader_root = root.keys.numElem() ? root.keys[0] : NULL;

	if(!shader_root)
	{
		MsgError("Material '%s' does not have a shader root section!\n",m_szMaterialName.c_str());
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
void CMaterial::Init(const char* materialName, kvkeybase_t* shader_root)
{
	m_szMaterialName = materialName;

	// section name is used as shader name
	m_szShaderName = shader_root->name;

	// begin initialization
	InitVars( shader_root );

	InitShader();
}

//
// Initializes the proxy data from section
//
void CMaterial::InitMaterialProxy(kvkeybase_t* proxySec)
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
			m_hMatProxies.append( pProxy );
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
void CMaterial::InitMaterialVars(kvkeybase_t* kvs)
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

		kvkeybase_t* materialVar = kvs->keys[i];

		// initialize material var by this
		GetMaterialVar(materialVar->GetName(), KV_GetValueString(materialVar));
	}
}

//
// Initializes the shader
//
void CMaterial::InitShader()
{
	if( m_pShader != NULL )
		return;

	// don't need to do anything if NODRAW
	if( !m_szShaderName.CompareCaseIns("NODRAW") )
	{
		m_state = MATERIAL_LOAD_OK;
		return;
	}

	m_pShader = materials->CreateShaderInstance( m_szShaderName.GetData() );

	// if not found - try make Error shader
	if(!m_pShader)// || (m_pShader && !stricmp(m_pShader->GetName(), "Error")))
	{
		MsgError("Invalid shader '%s' specified for material %s!\n",m_szShaderName.GetData(),m_szMaterialName.GetData());

		if(!m_pShader)
			m_pShader = materials->CreateShaderInstance("Error");
	}

	if(m_pShader)
	{
		// just init the parameters
		m_pShader->Init( this );
		m_state = MATERIAL_LOAD_NEED_LOAD;
	}
	else
		m_state = MATERIAL_LOAD_ERROR;
}

//
// Initializes material vars and shader name
//
void CMaterial::InitVars(kvkeybase_t* shader_root)
{
	// Get an API preferences
	kvkeybase_t* apiPrefs = shader_root->FindKeyBase(varargs("API_%s", g_pShaderAPI->GetRendererName()), KV_FLAG_SECTION);

	// init root material vars
	InitMaterialVars( shader_root );

	//
	// API preference lookup
	//
	if(apiPrefs)
	{
		kvkeybase_t* pPair = apiPrefs->FindKeyBase("Shader");

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
		kvkeybase_t* editorPrefs = shader_root->FindKeyBase("editor", KV_FLAG_SECTION);

		// API preference lookup
		if(editorPrefs)
		{
			kvkeybase_t* pPair = editorPrefs->FindKeyBase("Shader");

			// Set shader name from API prefs if available
			if(pPair)
				m_szShaderName = KV_GetValueString(pPair);

			// init material vars from editor overrides if have some
			InitMaterialVars( editorPrefs );
		}
	}

	// init material proxies
	kvkeybase_t* proxy_sec = shader_root->FindKeyBase("MaterialProxy", KV_FLAG_SECTION);
	InitMaterialProxy( proxy_sec );
}


bool CMaterial::LoadShaderAndTextures()
{
	InitShader();

	if(m_state != MATERIAL_LOAD_NEED_LOAD)
		return false;

	if(!m_pShader)
		return true;

	m_state = MATERIAL_LOAD_INQUEUE;

	// try init
	if(!m_pShader->IsInitialized() && !m_pShader->IsError())
	{
		m_pShader->InitTextures();
		m_pShader->InitShader();
	}

	if( m_pShader->IsInitialized() )
		m_state = MATERIAL_LOAD_OK;
	else if( m_pShader->IsError() )
		m_state = MATERIAL_LOAD_ERROR;
	else
		ASSERTMSG(false, varargs("please check shader '%s' (%s) for initialization (not error, not initialized)", m_szShaderName.c_str(), m_pShader->GetName()));

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
	int nameHash = StringToHash(pszVarName, true);

	for(int i = 0;i < m_hMatVars.numElem();i++)
	{
		if(m_hMatVars[i]->m_nameHash == nameHash)
			return m_hMatVars[i];
	}

	return NULL;
}

ITexture* CMaterial::GetBaseTexture(int stage)
{
	if(m_pShader != NULL && !IsError())
	{
		// try load
		LoadShaderAndTextures();

		// wait if it was loading in another thread
		WaitForLoading();

		return m_pShader->GetBaseTexture(stage);
	}
	else
	{
		// Return error texture
		return g_pShaderAPI->GetErrorTexture();
	}
}

int CMaterial::GetFlags() const
{
	if(!m_pShader)
		return 0;

	return m_pShader->GetFlags();
}

const char*	CMaterial::GetShaderName() const
{
	if(!m_pShader)
		return m_szShaderName.c_str();

	return m_pShader->GetName();
}

// creates or finds existing material vars
IMatVar* CMaterial::CreateMaterialVar(const char* pszVarName, const char* defaultParam)
{
	IMatVar *pMatVar = FindMaterialVar(pszVarName);

	if(!pMatVar)
	{
		CMatVar *pVar = new CMatVar;
		pVar->Init(pszVarName, defaultParam);

		m_hMatVars.append(pVar);

		pMatVar = pVar;
	}

	return pMatVar;
}

// remove material var
void CMaterial::RemoveMaterialVar(IMatVar* pVar)
{
	for(int i = 0; i < m_hMatVars.numElem();i++)
	{
		if(m_hMatVars[i] == pVar)
		{
			delete m_hMatVars[i];
			m_hMatVars.removeIndex(i);
			return;
		}
	}
}

void CMaterial::Ref_DeleteObject()
{
}

void CMaterial::Cleanup(bool dropVars, bool dropShader)
{
	// drop shader if we need
	if(dropShader && m_pShader)
	{
		m_pShader->Unload();

		delete m_pShader;
		m_pShader = NULL;
	}

	if(dropVars)
	{
		for(int i = 0; i < m_hMatVars.numElem();i++)
			delete m_hMatVars[i];

		m_hMatVars.clear();
	}

	// always drop proxies
	for(int i = 0; i < m_hMatProxies.numElem();i++)
		delete m_hMatProxies[i];

	m_hMatProxies.clear();

	m_state = MATERIAL_LOAD_NEED_LOAD;
}

void CMaterial::UpdateProxy(float fDt)
{
	if(!m_proxyIsDirty)
		return;

	for(int i = 0; i < m_hMatProxies.numElem(); i++)
		m_hMatProxies[i]->UpdateProxy( fDt );

	m_proxyIsDirty = false;
}

void CMaterial::Setup(uint paramMask)
{
	// shaders and textures needs to be reset
	if(GetFlags() & MATERIAL_FLAG_BASETEXTURE_CUR)
		g_pShaderAPI->Reset( STATE_RESET_SHADER );
	else
		g_pShaderAPI->Reset( STATE_RESET_SHADER | STATE_RESET_TEX );

	m_pShader->SetupShader();
	m_pShader->SetupConstants( paramMask );
}
