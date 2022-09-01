//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IDkCore.h"

// interface pointer keeper
struct coreInterface_t
{
	const char*				name;		// module name
	struct DKMODULE*		module;		// module which loads this interface
	IEqCoreModule*	ptr;		// the interface pointer itself
};

// Equilibrium core interface
class CDkCore : public IDkCore
{
public:
	CDkCore();

	bool					Init(const char* pszApplicationName,const char *pszCommandLine);	// Initializes core
	bool					Init(const char* pszApplicationName,int argc, char **argv);	// Initializes core for tools. This is an console app initializer, and logging will be forced

	void					Shutdown();	// Shutdowns core

	char*					GetApplicationName()  const;

	// now configuration is global for all applications
	KeyValues*				GetConfig()  const;

	bool					IsInitialized()  const;

	void					AddExceptionCallback(CoreExceptionCallback callback);
	void					RemoveExceptionCallback(CoreExceptionCallback callback);

	const Array<CoreExceptionCallback>&	GetExceptionHandlers() const { return m_exceptionCb; }
// Interface management for engine

	void					RegisterInterface(const char* pszName, IEqCoreModule* iface);			// registers interface for faster access
	IEqCoreModule*			GetInterface(const char* pszName) const;								// returns registered interface
	void					UnregisterInterface(const char* pszName);								// unregisters interface

private:
	EqString						m_szApplicationName;
	EqString						m_szCurrentSessionUserName;
	bool							m_bInitialized;

	KeyValues*						m_coreConfiguration;

	Array<coreInterface_t>			m_interfaces{ PP_SL };
	Array<CoreExceptionCallback>	m_exceptionCb{ PP_SL };
};
