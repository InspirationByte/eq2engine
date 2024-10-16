//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Material system realtime parameter proxy
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IMaterial;
struct KVSection;

class IMaterialProxy
{
public:
	virtual ~IMaterialProxy() {}

	virtual void InitProxy(IMaterial* material, const KVSection* pKeyBase) = 0;
	virtual void UpdateProxy(float dt) = 0;
};

//----------------------------------------------------------------

#define PAIR_VARIABLE	'$'
#define PAIR_CONSTANT	'#'

#define VAR_ELEM_OPEN	'['
#define VAR_ELEM_CLOSE	']'

#define CONST_NAME_FRAMETIME	"frametime"
#define CONST_NAME_GAMETIME		"gametime"

enum ProxyVarType_e
{
	PV_CONSTANT = 0,
	PV_VARIABLE,
	PV_GAMETIME,
	PV_FRAMETIME,
};

struct proxyvar_t
{
	proxyvar_t()
	{
		vec_idx = -1;
		type = PV_CONSTANT;
	}

	MatVarProxy<Vector4D>	mv;
	float					value{ 0.0f };

	int8					vec_idx : 4;
	int8					type : 4;
};

//--------------------------------------------------------

class CBaseMaterialProxy : public IMaterialProxy
{
public:
	CBaseMaterialProxy() = default;

protected:
	void UpdateVar(proxyvar_t& var, float fDt);

	void ParseVariable(IMaterial* material, proxyvar_t& var, const char* pszVal);

	void mvSetValue(proxyvar_t& var, float value);
	void mvSetValueInt(proxyvar_t& var, int value);
};

//---------------------------------------------------------

inline void CBaseMaterialProxy::ParseVariable(IMaterial* material, proxyvar_t& var, const char* pszVal)
{
	var.value = 0.0f;
	var.vec_idx = -1;
	var.type = PV_CONSTANT;

	if(!pszVal)
		return;

	char* pairval = (char*)pszVal;

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

		var.mv = material->GetMaterialVar(varNameStr, "0");

		if(!var.mv.IsValid())
		{
			MsgWarning("Proxy error: variable '%s' not found in '%s'\n", varNameStr, material->GetName());
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
			var.value = var.mv.Get().x;
		else
			var.value = var.mv.Get()[(int)var.vec_idx];

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
	else if(!CString::CompareCaseIns(pairval, CONST_NAME_FRAMETIME))
	{
		var.value = 0.0f;
		var.vec_idx = -1;
		var.type = PV_FRAMETIME;
	}
	else if(!CString::CompareCaseIns(pairval, CONST_NAME_GAMETIME))
	{
		var.value = 0.0f;
		var.vec_idx = -1;
		var.type = PV_GAMETIME;
	}
}

inline void CBaseMaterialProxy::UpdateVar(proxyvar_t& var, float fDt)
{
	if(!var.mv.IsValid())
		return;

	switch(var.type)
	{
		case PV_VARIABLE:
		{
			if(var.vec_idx == -1)
				var.value = var.mv.Get().x;
			else
				var.value = var.mv.Get()[(int)var.vec_idx];

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

inline void CBaseMaterialProxy::mvSetValue(proxyvar_t& var, float value)
{
	if(!var.mv.IsValid() || var.type != PV_VARIABLE)
		return;

	var.value = value;

	if(var.vec_idx >= 0)
	{
		Vector4D outval = var.mv.Get();
		outval[(int)var.vec_idx] = value;
		var.mv.Set(outval);
	}
	else
		var.mv.Set(value);
}

inline void CBaseMaterialProxy::mvSetValueInt(proxyvar_t& var, int value)
{
	if(!var.mv.IsValid() || var.type != PV_VARIABLE)
		return;

	var.value = value;
	MatIntProxy(var.mv).Set(value);
}


//-----------------------------------------------------------------------------------

#define DECLARE_PROXY(localName, className)											\
	static IMaterialProxy *C##className##Factory( void )							\
	{																				\
		IMaterialProxy *pShader = static_cast< IMaterialProxy * >(new className); 	\
		return pShader;																\
	}

#define REGISTER_PROXY(localName, className)	\
	g_matSystem->RegisterProxy( &C##className##Factory, #localName );
