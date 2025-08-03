//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/IDkCore.h"
#include "utils/KeyValues.h"

class IEqCoreModule;

// interface pointer keeper
struct coreInterface_t
{
	const char*			name;		// module name
	struct OSModule*	module;		// module which loads this interface
	IEqCoreModule*		ptr;		// the interface pointer itself
};

// Equilibrium core interface
class CDkCore : public IDkCore
{
public:
	bool						Init(const CoreAppInitParameters& initParams);
	void						Shutdown();	// Shutdowns core

	char*						GetApplicationName()  const;

	// now configuration is global for all applications
	const KVSection&			GetConfig()  const;
	const CoreDebugSettings&	GetDebugSettings() const { return m_debugSettings; }

	bool						IsInitialized()  const;

	void						AddExceptionCallback(CoreExceptionCallback callback);
	void						RemoveExceptionCallback(CoreExceptionCallback callback);

	const Array<CoreExceptionCallback>&	GetExceptionHandlers() const { return m_exceptionCb; }

	// loads module
	OSModule*					OpenModule(const char* mod_name, EqString* outError = nullptr);

	// frees module
	void						CloseModule(OSModule* pModule);

	// returns procedure address of the loaded module
	void*						GetProcedureAddress(OSModule* pModule, const char* pszProc) const;

// Interface management for engine

	void						OnModuleLoaded(const char* pszName);
	void						OnModuleUnloaded(const char* pszName);

	void						RegisterInterface(const char* pszName, IEqCoreModule* iface);			// registers interface for faster access
	IEqCoreModule*				GetInterface(const char* pszName) const;								// returns registered interface
	void						UnregisterInterface(const char* pszName);								// unregisters interface

private:
	Array<OSModule*>				m_modules{ PP_SL };
	Array<coreInterface_t>			m_interfaces{ PP_SL };
	Array<CoreExceptionCallback>	m_exceptionCb{ PP_SL };

	CoreDebugSettings				m_debugSettings;
	KVSection						m_coreConfiguration;

	EqString						m_szApplicationName;
	EqString						m_szCurrentSessionUserName;
	bool							m_isInit{ false };

#ifdef HAS_LIVEPP_SUPPORT
	bool							m_livePPEnabled{ false };
#endif
};
