//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Material system realtime parameter proxy
//////////////////////////////////////////////////////////////////////////////////

#ifndef IMATERIALPROXY_H
#define IMATERIALPROXY_H

#include "IMaterial.h"
#include "utils/KeyValues.h"

class IMaterialProxy
{
public:
	virtual ~IMaterialProxy() {}
	virtual void InitProxy(IMaterial* pAssignedMaterial, kvkeybase_t* pKeyBase) = 0;
	virtual void UpdateProxy(float dt) = 0;
};

typedef IMaterialProxy* (*PROXY_DISPATCHER)( void );

// proxy registrator
class IProxyFactory
{
public:
	virtual void				RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName) = 0;
	virtual IMaterialProxy*		CreateProxyByName(const char* pszName) = 0;
};

extern IProxyFactory* proxyfactory;

#define DECLARE_PROXY(localName, className)											\
	static IMaterialProxy *C##className##Factory( void )							\
	{																				\
		IMaterialProxy *pShader = static_cast< IMaterialProxy * >(new className); 	\
		return pShader;																\
	}

#define REGISTER_PROXY(localName, className)	\
	proxyfactory->RegisterProxy( &C##className##Factory, #localName );	\

#endif // IMATERIALPROXY_H