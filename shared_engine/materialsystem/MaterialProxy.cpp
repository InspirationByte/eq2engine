//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Standard proxy arithmetics - add, multiply, subtract, divide
// Standard functions - sin, cos
//////////////////////////////////////////////////////////////////////////////////

#include "MaterialProxy.h"

#define PAIR_VARIABLE	'$'
#define PAIR_CONSTANT	'#'

#define VAR_ELEM_OPEN	'['
#define VAR_ELEM_CLOSE	']'

#define CONST_NAME_FRAMETIME	"frametime"
#define CONST_NAME_GAMETIME		"gametime"


CBaseMaterialProxy::CBaseMaterialProxy()
{
	m_pMaterial = NULL;
}

proxyvar_t CBaseMaterialProxy::ParseVariable(char* pszVal)
{
	proxyvar_t var;

	var.fValue = 0.0f;
	var.vec_elem = -1;
	var.type = PV_CONSTANT;
	var.pVar = NULL;

	if(!pszVal)
		return var;

	char* pairval = pszVal;
		
	char firstSymbol = pairval[0];
	if(firstSymbol == PAIR_VARIABLE)
	{
		char *varName = pairval+1;
		int len = 0;

		while(1)
		{
			if(varName[len] == '\0' || varName[len] == VAR_ELEM_OPEN)
				break;

			len++;
		}

		char varNameStr[128];
		memcpy(varNameStr, varName, len);
		varNameStr[len] = '\0';

		var.pVar = m_pMaterial->GetMaterialVar(varNameStr, "0");

		if(!var.pVar)
			MsgWarning("Proxy error: variable '%s' not found in '%s'\n", varNameStr, m_pMaterial->GetName());

		char* varExt = varName + len;

		// got array
		if(*varExt == VAR_ELEM_OPEN)
		{
			varExt += 1;
				
			char varIndexstr[2];
			memcpy(varIndexstr, varExt, 1);
			varIndexstr[1] = '\0';

			var.vec_elem = atoi(varIndexstr);
		}

		if(var.pVar)
		{
			if(var.vec_elem == -1)
				var.fValue = var.pVar->GetFloat();
			else
				var.fValue = var.pVar->GetVector4()[(int)var.vec_elem];
		}

		var.type = PV_VARIABLE;
	}
	else if(firstSymbol == PAIR_CONSTANT)
	{
		char *constString = pairval+1;
		float value = (float)atof(constString);

		var.fValue = value;
		var.vec_elem = -1;
		var.type = PV_CONSTANT;
	}
	else if(!stricmp(pairval, CONST_NAME_FRAMETIME))
	{
		var.fValue = 0.0f;
		var.vec_elem = -1;
		var.type = PV_FRAMETIME;
	}
	else if(!stricmp(pairval, CONST_NAME_GAMETIME))
	{
		var.fValue = 0.0f;
		var.vec_elem = -1;
		var.type = PV_GAMETIME;
	}

	return var;
}

void CBaseMaterialProxy::UpdateVar(proxyvar_t* var, float fDt)
{
	if(var->type == PV_CONSTANT)
		return;
	else if(var->type == PV_VARIABLE)
	{
		if(var->pVar)
		{
			if(var->vec_elem == -1)
				var->fValue = var->pVar->GetFloat();
			else
				var->fValue = var->pVar->GetVector4()[(int)var->vec_elem];
		}
	}
	else if(var->type == PV_GAMETIME)
	{
		var->fValue = 0.0f; // + game time
	}
	else if(var->type == PV_FRAMETIME)
	{
		var->fValue = fDt;
	}
}

void CBaseMaterialProxy::PVarSetValue(proxyvar_t* var, float value)
{
	if(var->type == PV_CONSTANT)
		return;
	else if(var->type == PV_VARIABLE)
	{
		if(var->pVar)
		{
			var->fValue = value;

			if(var->vec_elem == -1)
			{
				var->pVar->SetFloat(value);
			}
			else
			{
				Vector4D outval = var->pVar->GetVector4();
				outval[(int)var->vec_elem] = value;
				var->pVar->SetVector4(outval);
			}
		}
	}
}

void CBaseMaterialProxy::PVarSetValueInt(proxyvar_t* var, int value)
{
	if(var->type == PV_CONSTANT)
		return;
	else if(var->type == PV_VARIABLE)
	{
		if(var->pVar)
		{
			var->fValue = value;

			if(var->vec_elem == -1)
			{
				var->pVar->SetInt(value);
			}
			else
			{
				Vector4D outval = var->pVar->GetVector4();
				outval[(int)var->vec_elem] = value;
				var->pVar->SetVector4(outval);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------
//	Make some default proxies for user
//-------------------------------------------------------------------------------------------------------

// add proxy

class CAddProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'add' proxy in '%s' error: invalid argument count\n Usage: add v1 v2 out [options]\n");
			return;
		}

		in1 = ParseVariable( pKeyBase->values[0] );
		in2 = ParseVariable( pKeyBase->values[1] );
		out = ParseVariable( pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(&in1, dt);
		UpdateVar(&in2, dt);

		if(useFrameTime)
			PVarSetValue(&out, in1.fValue + in2.fValue*dt);
		else
			PVarSetValue(&out, in1.fValue + in2.fValue);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

// subtract proxy

class CSubProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'sub' proxy in '%s' error: invalid argument count\n Usage: sub v1 v2 out [options]\n");
			return;
		}

		in1 = ParseVariable( pKeyBase->values[0] );
		in2 = ParseVariable( pKeyBase->values[1] );
		out = ParseVariable( pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(&in1, dt);
		UpdateVar(&in2, dt);

		if(useFrameTime)
			PVarSetValue(&out, in1.fValue - in2.fValue*dt);
		else
			PVarSetValue(&out, in1.fValue - in2.fValue);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

// multiply proxy

class CMulProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'mul' proxy in '%s' error: invalid argument count\n Usage: mul v1 v2 out [options]\n");
			return;
		}

		in1 = ParseVariable( pKeyBase->values[0] );
		in2 = ParseVariable( pKeyBase->values[1] );
		out = ParseVariable( pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(&in1, dt);
		UpdateVar(&in2, dt);

		if(useFrameTime)
			PVarSetValue(&out, in1.fValue * in2.fValue*dt);
		else
			PVarSetValue(&out, in1.fValue * in2.fValue);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

// divide proxy

class CDivProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;

		if(pKeyBase->values.numElem() < 3)
		{
			MsgError("'div' proxy in '%s' error: invalid argument count\n Usage: div v1 v2 out [options]\n");
			return;
		}

		in1 = ParseVariable( pKeyBase->values[0] );
		in2 = ParseVariable( pKeyBase->values[1] );
		out = ParseVariable( pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(&in1, dt);
		UpdateVar(&in2, dt);

		if(useFrameTime)
			PVarSetValue(&out, in1.fValue / in2.fValue*dt);
		else
			PVarSetValue(&out, in1.fValue / in2.fValue);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

// sinus proxy
class CSinProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;
		if(pKeyBase->values.numElem() < 2)
		{
			MsgError("'sin' proxy in '%s' error: invalid argument count\n Usage: sin v1 out [options]\n");
			return;
		}

		in = ParseVariable( pKeyBase->values[0] );
		out = ParseVariable( pKeyBase->values[1] );
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(&in, dt);

		PVarSetValue(&out, sin(in.fValue));
	}
private:
	proxyvar_t in;

	proxyvar_t out;
};

// sinus proxy
class CAnimatedTextureProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;
		kvkeybase_t* pair = NULL;

		pair = pKeyBase->FindKeyBase("framerate");
		fFrate = ParseVariable((char*)KV_GetValueString(pair));

		pair = pKeyBase->FindKeyBase("framecount");
		fFcount = ParseVariable((char*)KV_GetValueString(pair));

		pair = pKeyBase->FindKeyBase("frameVar");
		out = ParseVariable((char*)KV_GetValueString(pair));

		time = 0.0f;
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(&fFrate, dt);
		UpdateVar(&out, dt);

		if(int(time) > int(fFcount.fValue))
			time = 0.0f;

		time += dt * fFrate.fValue;

		PVarSetValueInt( &out, time );
	}
private:
	float time;

	proxyvar_t fFrate;
	proxyvar_t fFcount;

	proxyvar_t out;
};

// declare proxies
DECLARE_PROXY(add, CAddProxy);
DECLARE_PROXY(subtract, CSubProxy);
DECLARE_PROXY(multiply, CMulProxy);
DECLARE_PROXY(divide, CDivProxy);
DECLARE_PROXY(sin, CSinProxy);
DECLARE_PROXY(animatedtexture, CAnimatedTextureProxy);

void InitMaterialProxies()
{
	REGISTER_PROXY(add, CAddProxy);
	REGISTER_PROXY(subtract, CSubProxy);
	REGISTER_PROXY(multiply, CMulProxy);
	REGISTER_PROXY(divide, CDivProxy);
	REGISTER_PROXY(sin, CSinProxy);
	REGISTER_PROXY(animatedtexture, CAnimatedTextureProxy);
}