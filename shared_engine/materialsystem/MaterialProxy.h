//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Material system realtime parameter proxy
//////////////////////////////////////////////////////////////////////////////////

#ifndef MATERIALPROXY_H
#define MATERIALPROXY_H

#include "materialsystem/IMaterialSystem.h"

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
		value = 0.0f;
		vec_idx = -1;
		type = PV_CONSTANT;
		mv = NULL;
	}

	IMatVar*		mv;
	float			value;

	int8			vec_idx : 4;
	int8			type : 4;
};

//--------------------------------------------------------

class CBaseMaterialProxy : public IMaterialProxy
{
public:
	CBaseMaterialProxy();

	void UpdateVar(proxyvar_t& var, float fDt);

protected:
	void ParseVariable(proxyvar_t& var, const char* pszVal);
	
	void mvSetValue(proxyvar_t& var, float value);
	void mvSetValueInt(proxyvar_t& var, int value);

protected:
	IMaterial*	m_pMaterial;
	bool		m_bIsError;
};

void InitMaterialProxies();

#endif // MATERIALPROXY_H