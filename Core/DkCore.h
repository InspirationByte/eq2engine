//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: MuscleCore interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CDKCORE
#define CDKCORE

#include "IDkCore.h"

// interface pointer keeper
struct coreInterface_t
{
	const char*				name;		// module name
	DKMODULE*				module;		// module which loads this interface
	ICoreModuleInterface*	ptr;		// the interface pointer itself
};

// DarkTech core interface
class CDkCore : public IDkCore
{
public:
	bool					Init(const char* pszApplicationName,const char *pszCommandLine);	// Initializes core
	bool					Init(const char* pszApplicationName,int argc, char **argv);	// Initializes core for tools. This is an console app initializer, and logging will be forced

	void					InitSubInterfaces();

	void					Shutdown();	// Shutdowns core

	char*					GetApplicationName();

	char*					GetCurrentUserName();

	bool					IsInitialized() {return m_bInitialized;}

	// now configuration is global for all applications
	KeyValues*				GetConfig();

// Interface management for engine

	void					RegisterInterface(const char* pszName, ICoreModuleInterface* iface);	// registers interface for faster access
	ICoreModuleInterface*	GetInterface(const char* pszName);										// returns registered interface
	void					UnregisterInterface(const char* pszName);								// unregisters interface

private:
	EqString				m_szApplicationName;
	EqString				m_szCurrentSessionUserName;
	bool					m_bInitialized;

	KeyValues				m_coreConfiguration;

	DkList<coreInterface_t> m_interfaces;
};

#endif //CDKCORE
