//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Material system realtime parameter proxy
//////////////////////////////////////////////////////////////////////////////////

#ifndef MATERIALPROXY_H
#define MATERIALPROXY_H

#include "IMaterialSystem.h"

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
		fValue = 0.0f;
		vec_elem = -1;
		type = PV_CONSTANT;
		pVar = NULL;
	}

	IMatVar*		pVar;
	float			fValue;
	int8			vec_elem;

	ProxyVarType_e	type;
};

class CBaseMaterialProxy : public IMaterialProxy
{
public:
	CBaseMaterialProxy();

	void UpdateVar(proxyvar_t* var, float fDt);

protected:
	proxyvar_t ParseVariable(char* pszVal);
	
	void PVarSetValue(proxyvar_t* var, float value);
	void PVarSetValueInt(proxyvar_t* var, int value);

protected:
	IMaterial*	m_pMaterial;
	bool		m_bIsError;
};

void InitMaterialProxies();

#endif // MATERIALPROXY_H