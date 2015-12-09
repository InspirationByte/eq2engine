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
#include "utils/eqstring.h"
#include "BaseShader.h"

CMaterial::CMaterial() : m_state(MATERIAL_LOAD_ERROR), m_pShader(nullptr), m_proxyIsDirty(true), m_frameBound(0)
{
}

CMaterial::~CMaterial()
{
}

void CMaterial::Init(const char* szFileName_noExt, bool flushMatVarsOnly)
{
	m_szMaterialName = szFileName_noExt;
	m_szMaterialName.Path_FixSlashes();

	if( m_szMaterialName.c_str()[0] == '/' ||  m_szMaterialName.c_str()[0] == '\\' )
		m_szMaterialName = m_szMaterialName.c_str()+1;

	DevMsg(2, "Loading material '%s'\n",szFileName_noExt);

	InitVars();
}

void CMaterial::InitMaterialProxy(kvkeybase_t* pProxyData)
{
	if(!pProxyData)
		return;

	// try any kind of proxy
	for(int i = 0; i < pProxyData->keys.numElem();i++)
	{
		IMaterialProxy* pProxy = proxyfactory->CreateProxyByName( pProxyData->keys[i]->name );

		if(pProxy)
		{
			// initialize proxy
			pProxy->InitProxy( this, pProxyData->keys[i] );

			// add to list
			m_hMatProxies.append( pProxy );
		}
		else
		{
			MsgWarning("Unknown proxy '%s' for material %s!\n", pProxyData->keys[i]->name, m_szMaterialName.GetData());
		}
	}
}

void CMaterial::InitVars(bool flushOnly)
{
	KeyValues pKV;

	if( pKV.LoadFromFile((materials->GetMaterialPath() + m_szMaterialName + _Es(".mat")).GetData()) )
	{
		kvkeybase_t* pRootSection = NULL;

		if(pKV.GetRootSection()->keys.numElem() > 0)
			pRootSection = pKV.GetRootSection()->keys[0];

		if(!pRootSection)
		{
			MsgError("Invalid file %s for materials\n",m_szMaterialName.GetData());
			m_state = MATERIAL_LOAD_ERROR;
			return;
		}

		// Get shader name as section name (VALVE's analogy)
		m_szShaderName = pRootSection->name;

		// Rendering API preferences in shader
		kvkeybase_t* pCurrentAPIPrefsSection = pRootSection->FindKeyBase(varargs("API_%s", g_pShaderAPI->GetRendererName()), KV_FLAG_SECTION);

		// API preference lookup
		if(pCurrentAPIPrefsSection)
		{
			kvkeybase_t* pPair = pCurrentAPIPrefsSection->FindKeyBase("Shader");

			// Set shader name from API prefs if available
			if(pPair)
				m_szShaderName = pPair->values[0];
		}

		if(materials->GetConfiguration().editormode)
		{
			pCurrentAPIPrefsSection = pRootSection->FindKeyBase("editor", KV_FLAG_SECTION);

			// API preference lookup
			if(pCurrentAPIPrefsSection)
			{
				kvkeybase_t* pPair = pCurrentAPIPrefsSection->FindKeyBase("Shader");

				// Set shader name from API prefs if available
				if(pPair)
					m_szShaderName = pPair->values[0];
			}
		}

		for(int i = 0; i < pRootSection->keys.numElem();i++)
		{
			// ignore sections
			if( pRootSection->keys[i]->keys.numElem() )
				continue;

			if(flushOnly)
			{
				// variable name lookup
				char* var_name = pRootSection->keys[i]->name;

				IMatVar* pVarToFlush = FindMaterialVar(var_name);
				if(pVarToFlush)
				{
					// flush variable falue
					pVarToFlush->SetString(pRootSection->keys[i]->values[0]);
				}
				else
				{
					// create if we not found mathing var
					CMatVar *pVar = new CMatVar;
					pVar->Init(pRootSection->keys[i]->name, pRootSection->keys[i]->values[0]);

					m_hMatVars.append(pVar);
				}
			}
			else
			{
				CMatVar *pVar = new CMatVar;
				pVar->Init(pRootSection->keys[i]->name,pRootSection->keys[i]->values[0]);

				m_hMatVars.append(pVar);
			}
		}

		if(pCurrentAPIPrefsSection)
		{
			// API-specified parameters
			for(int i = 0; i < pCurrentAPIPrefsSection->keys.numElem();i++)
			{
				// ignore sections
				if( pCurrentAPIPrefsSection->keys[i]->keys.numElem() )
					continue;

				// make var if no var is present
				IMatVar* pVar = GetMaterialVar(pCurrentAPIPrefsSection->keys[i]->name, pCurrentAPIPrefsSection->keys[i]->values[0]);

				// set it's value
				pVar->SetString( pCurrentAPIPrefsSection->keys[i]->values[0] );
			}
		}

		// load default proxies
		kvkeybase_t* proxy_sec = pRootSection->FindKeyBase("MaterialProxy", KV_FLAG_SECTION);
		InitMaterialProxy(proxy_sec);

		// load API-specified proxies
		if(pCurrentAPIPrefsSection)
		{
			proxy_sec = pCurrentAPIPrefsSection->FindKeyBase("MaterialProxy");
			InitMaterialProxy(proxy_sec);
		}
	}
	else
	{
		MsgError("Can't load material '%s' from file\n", m_szMaterialName.GetData());

		// make error shader and init
		m_pShader = materials->CreateShaderInstance("Error");

		if(m_pShader)
			((CBaseShader*)m_pShader)->m_pAssignedMaterial = this;

		m_state = MATERIAL_LOAD_NEED_LOAD;
		return;
	}

	// do i need to load shader?
	// this is a case when material is ok
	if( m_pShader == NULL )
	{
		if(!m_szShaderName.CompareCaseIns("nodraw"))
		{
			m_state = MATERIAL_LOAD_OK;
			return;
		}

		m_pShader = materials->CreateShaderInstance( m_szShaderName.GetData() );

		if(!m_pShader || (m_pShader && !stricmp(m_pShader->GetName(), "error")))
		{
			MsgError("Invalid shader '%s' specified for material %s!\n",m_szShaderName.GetData(),m_szMaterialName.GetData());

			m_pShader = materials->CreateShaderInstance("Error");

			if(m_pShader)
			{
				((CBaseShader*)m_pShader)->m_pAssignedMaterial = this;	// assign material to shader
				m_state = MATERIAL_LOAD_NEED_LOAD;
			}
			else
				m_state = MATERIAL_LOAD_ERROR;

			return;
		}

		((CBaseShader*)m_pShader)->m_pAssignedMaterial = this;
		m_state = MATERIAL_LOAD_NEED_LOAD;
	}
}


bool CMaterial::LoadShaderAndTextures()
{
	if(m_state != MATERIAL_LOAD_NEED_LOAD)
		return false;

	if(m_pShader)
	{
		m_state = MATERIAL_LOAD_INQUEUE;

		m_pShader->InitParams();

		if( m_pShader->IsInitialized() )
			m_state = MATERIAL_LOAD_OK;
		else if( m_pShader->IsError() )
			m_state = MATERIAL_LOAD_ERROR;
		else
			ASSERTMSG(false, varargs("please check shader '%s' (%s) for initialization (not error, not initialized)", m_szShaderName.c_str(), m_pShader->GetName()));
	}

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
	for(int i = 0;i < m_hMatVars.numElem();i++)
	{
		if(!stricmp(m_hMatVars[i]->GetName(),pszVarName))
			return m_hMatVars[i];
	}

	CMatVar* pNewVar = (CMatVar*)CreateMaterialVar(pszVarName);
	pNewVar->SetString(defaultparameter);

	return pNewVar;
}

IMatVar *CMaterial::FindMaterialVar(const char* pszVarName) const
{
	for(int i = 0;i < m_hMatVars.numElem();i++)
	{
		if(!stricmp(m_hMatVars[i]->GetName(),pszVarName))
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
		return "Error";

	return m_pShader->GetName();
}

// creates or finds existing material vars
IMatVar* CMaterial::CreateMaterialVar(const char* pszVarName)
{
	IMatVar *pMatVar = FindMaterialVar(pszVarName);

	if(!pMatVar)
	{
		CMatVar *pVar = new CMatVar;
		pVar->Init(pszVarName,"0");

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
	// TODO: this must be done
	//materials->FreeMaterial(this);
}

void CMaterial::Cleanup(bool bUnloadShaders, bool bUnloadTextures, bool keepMaterialVars)
{
	if(!keepMaterialVars)
	{
		for(int i = 0; i < m_hMatVars.numElem();i++)
		{
			delete m_hMatVars[i];
			m_hMatVars.removeIndex(i);
			i--;
		}
	}

	// proxy will be reinitialized
	for(int i = 0; i < m_hMatProxies.numElem();i++)
	{
		delete m_hMatProxies[i];
		m_hMatProxies.removeIndex(i);
		i--;
	}

	if(m_pShader)
	{
		m_pShader->Unload(bUnloadShaders,bUnloadTextures);
		if(bUnloadShaders && bUnloadTextures)
		{
			delete m_pShader;
			m_pShader = NULL;
		}
	}

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

void CMaterial::Setup()
{
	// shaders and textures needs to be reset
	g_pShaderAPI->Reset( STATE_RESET_SHADER | STATE_RESET_TEX );

	m_pShader->SetupShader();
	m_pShader->SetupConstants();
}