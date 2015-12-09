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
struct coreinterface_t
{
	const char* name;
	void* ptr;
};

// DarkTech core interface
class CDkCore : public IDkCore
{
public:
	bool					Init(const char* pszApplicationName,const char *pszCommandLine);	// Initializes core
	bool					Init(const char* pszApplicationName,int argc, char **argv);	// Initializes core for tools. This is an console app initializer, and logging will be forced

	void					InitSubInterfaces();

	void					Shutdown();	// Shutdowns core

	ICommandLineParse*		GetCoreCommandLine();	// command line parser interface

	char*					GetApplicationName();

	char*					GetCurrentUserName();

	bool					IsInitialized() {return m_bInitialized;}

	// now configuration is global for all applications
	KeyValues*				GetConfig();

// Interface management for engine

	void					RegisterInterface(const char* pszName, void* ptr);	// registers interface for faster access
	void*					GetInterface(const char* pszName);			// returns registered interface
	void					UnregisterInterface(const char* pszName);			// unregisters interface

private:
	EqString				m_szApplicationName;
	EqString				m_szCurrentSessionUserName;
	bool					m_bInitialized;

	KeyValues				m_coreConfiguration;

	DkList<coreinterface_t> m_interfaces;
};

#endif //CDKCORE
