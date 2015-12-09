//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech material proxy factory
//////////////////////////////////////////////////////////////////////////////////

#include "materialsystem/IMaterialProxy.h"

struct proxyfactory_t
{
	char* name;
	PROXY_DISPATCHER disp;
};

// proxy registrator
class CProxyFactory : public IProxyFactory
{
public:
	void				RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName);
	IMaterialProxy*		CreateProxyByName(const char* pszName);

protected:

	DkList<proxyfactory_t> m_FactoryList;
};

static CProxyFactory s_ProxyFactory;

IProxyFactory* proxyfactory = &s_ProxyFactory;

void CProxyFactory::RegisterProxy(PROXY_DISPATCHER dispfunc, const char* pszName)
{
	proxyfactory_t factory;
	factory.name = strdup(pszName);
	factory.disp = dispfunc;

	m_FactoryList.append(factory);
}

IMaterialProxy* CProxyFactory::CreateProxyByName(const char* pszName)
{
	for(int i = 0; i < m_FactoryList.numElem(); i++)
	{
		if(!stricmp(m_FactoryList[i].name, pszName))
		{
			return (m_FactoryList[i].disp)();
		}
	}

	return NULL;
}