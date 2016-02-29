//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: MuscleCore interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef IDKCORE
#define IDKCORE

#include "ICmdLineParser.h"

class KeyValues;
class ICommandLineParse;

// DarkTech core interface
class IDkCore
{
public:
	virtual bool					Init(const char* pszApplicationName, const char* pszCommandLine) = 0;	// Initializes core

	virtual 	bool				Init(const char* pszApplicationName, int argc, char **argv) = 0;	// Initializes core for tools

	virtual void					Shutdown() = 0;	// Shutdowns core

	virtual char*					GetApplicationName() = 0; // returns current application name string

	virtual char*					GetCurrentUserName() = 0;	// returns current user name string

	virtual bool					IsInitialized() = 0;	// Return status of initialization

	// now configuration is global for all applications
	virtual KeyValues*				GetConfig() = 0;

// Interface management for engine

	virtual void					RegisterInterface(const char* pszName, ICoreModuleInterface* iface) = 0;	// registers interface for faster access
	virtual ICoreModuleInterface*	GetInterface(const char* pszName) = 0;										// returns registered interface
	virtual void					UnregisterInterface(const char* pszName) = 0;								// unregisters interface
};

#ifndef _DKLAUNCHER_
IEXPORTS IDkCore*	GetCore();
#endif

#endif //IDKCORE
