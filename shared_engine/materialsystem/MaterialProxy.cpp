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

void CBaseMaterialProxy::ParseVariable(proxyvar_t& var, char* pszVal)
{
	var.value = 0.0f;
	var.vec_idx = -1;
	var.type = PV_CONSTANT;
	var.mv = NULL;

	if(!pszVal)
		return;

	char* pairval = pszVal;
		
	char firstSymbol = pairval[0];
	if(firstSymbol == PAIR_VARIABLE)
	{
		char* varName = pairval+1;
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

		var.mv = m_pMaterial->GetMaterialVar(varNameStr, "0");

		if(!var.mv)
		{
			MsgWarning("Proxy error: variable '%s' not found in '%s'\n", varNameStr, m_pMaterial->GetName());
			return;
		}

		char* varExt = varName + len;

		// got array
		if(*varExt == VAR_ELEM_OPEN)
		{
			varExt += 1;
				
			char varIndexstr[2];
			varIndexstr[0] = *varExt;
			varIndexstr[1] = '\0';

			var.vec_idx = atoi(varIndexstr);
		}

		if(var.vec_idx == -1)
			var.value = var.mv->GetFloat();
		else
			var.value = var.mv->GetVector4()[(int)var.vec_idx];

		var.type = PV_VARIABLE;
	}
	else if(firstSymbol == PAIR_CONSTANT)
	{
		char *constString = pairval+1;
		float value = (float)atof(constString);

		var.value = value;
		var.vec_idx = -1;
		var.type = PV_CONSTANT;
	}
	else if(!stricmp(pairval, CONST_NAME_FRAMETIME))
	{
		var.value = 0.0f;
		var.vec_idx = -1;
		var.type = PV_FRAMETIME;
	}
	else if(!stricmp(pairval, CONST_NAME_GAMETIME))
	{
		var.value = 0.0f;
		var.vec_idx = -1;
		var.type = PV_GAMETIME;
	}
}

void CBaseMaterialProxy::UpdateVar(proxyvar_t& var, float fDt)
{
	if(!var.mv)
		return;

	switch(var.type)
	{
		case PV_VARIABLE:
		{
			if(var.vec_idx == -1)
				var.value = var.mv->GetFloat();
			else
				var.value = var.mv->GetVector4()[(int)var.vec_idx];

			break;
		}
		case PV_GAMETIME:
			var.value = 0.0f; // + game time
			break;
		case PV_FRAMETIME:
			var.value = fDt;
			break;
	}
}

void CBaseMaterialProxy::mvSetValue(proxyvar_t& var, float value)
{
	if(!var.mv || var.type != PV_VARIABLE)
		return;

	var.value = value;

	if(var.vec_idx >= 0)
	{
		Vector4D outval = var.mv->GetVector4();
		outval[(int)var.vec_idx] = value;
		var.mv->SetVector4(outval);
	}
	else
		var.mv->SetFloat(value);
}

void CBaseMaterialProxy::mvSetValueInt(proxyvar_t& var, int value)
{
	if(!var.mv || var.type != PV_VARIABLE)
		return;

	var.value = value;

	if(var.vec_idx >= 0)
	{
		Vector4D outval = var.mv->GetVector4();
		outval[(int)var.vec_idx] = value;
		var.mv->SetVector4(outval);
	}
	else
		var.mv->SetInt(value);
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

		ParseVariable(in1, pKeyBase->values[0] );
		ParseVariable(in2, pKeyBase->values[1] );
		ParseVariable(out, pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value + in2.value*dt);
		else
			mvSetValue(out, in1.value + in2.value);
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

		ParseVariable(in1, pKeyBase->values[0] );
		ParseVariable(in2, pKeyBase->values[1] );
		ParseVariable(out, pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value - in2.value*dt);
		else
			mvSetValue(out, in1.value - in2.value);
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

		ParseVariable(in1, pKeyBase->values[0] );
		ParseVariable(in2, pKeyBase->values[1] );
		ParseVariable(out, pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value * in2.value*dt);
		else
			mvSetValue(out, in1.value * in2.value);
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

		ParseVariable(in1, pKeyBase->values[0] );
		ParseVariable(in2, pKeyBase->values[1] );
		ParseVariable(out, pKeyBase->values[2] );

		useFrameTime = false;

		for(int i = 3; i < pKeyBase->values.numElem(); i++)
		{
			if(!stricmp(pKeyBase->values[i], "useftime"))
				useFrameTime = true;
		}
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in1, dt);
		UpdateVar(in2, dt);

		if(useFrameTime)
			mvSetValue(out, in1.value / in2.value*dt);
		else
			mvSetValue(out, in1.value / in2.value);
	}
private:
	proxyvar_t in1;
	proxyvar_t in2;

	proxyvar_t out;

	bool useFrameTime;
};

// sine proxy
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

		ParseVariable(in, pKeyBase->values[0] );
		ParseVariable(out, pKeyBase->values[1] );
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in, dt);

		mvSetValue(out, sin(in.value));
	}
private:
	proxyvar_t in;

	proxyvar_t out;
};


// saturate proxy
class CAbsProxy : public CBaseMaterialProxy
{
public:
	void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase)
	{
		m_pMaterial = pAssignedMaterial;
		if(pKeyBase->values.numElem() < 2)
		{
			MsgError("'abs' proxy in '%s' error: invalid argument count\n Usage: abs v1 out [options]\n");
			return;
		}

		ParseVariable(in, pKeyBase->values[0] );
		ParseVariable(out, pKeyBase->values[1] );
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(in, dt);

		mvSetValue(out, fabs(in.value));
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

		// frame count is the only static variable, frame rate is dynamic
		frameCount = KV_GetValueInt(pKeyBase);

		pair = pKeyBase->FindKeyBase("framerate");
		ParseVariable(frameRate, (char*)KV_GetValueString(pair));

		pair = pKeyBase->FindKeyBase("frameVar");
		ParseVariable(out,(char*)KV_GetValueString(pair));

		time = 0.0f;
	}

	void UpdateProxy(float dt)
	{
		UpdateVar(frameRate, dt);
		UpdateVar(out, dt);

		if(int(time) >= frameCount)
			time = 0.0f;

		time += dt * frameRate.value;

		mvSetValueInt( out, time );
	}
private:
	float		time;

	proxyvar_t	frameRate;
	int			frameCount;

	proxyvar_t	out;
};

// declare proxies
DECLARE_PROXY(add, CAddProxy);
DECLARE_PROXY(subtract, CSubProxy);
DECLARE_PROXY(multiply, CMulProxy);
DECLARE_PROXY(divide, CDivProxy);
DECLARE_PROXY(sin, CSinProxy);
DECLARE_PROXY(abs, CAbsProxy);
DECLARE_PROXY(animatedtexture, CAnimatedTextureProxy);

void InitMaterialProxies()
{
	REGISTER_PROXY(add, CAddProxy);
	REGISTER_PROXY(subtract, CSubProxy);
	REGISTER_PROXY(multiply, CMulProxy);
	REGISTER_PROXY(divide, CDivProxy);
	REGISTER_PROXY(sin, CSinProxy);
	REGISTER_PROXY(abs, CAbsProxy);
	REGISTER_PROXY(animatedtexture, CAnimatedTextureProxy);
}