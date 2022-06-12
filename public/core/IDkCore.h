//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: MuscleCore interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class KeyValues;
class ICommandLine;

using CoreExceptionCallback = void(*)();

// DarkTech core interface
class IDkCore
{
public:
	virtual ~IDkCore() {}
	
	virtual bool					Init(const char* pszApplicationName, const char* pszCommandLine) = 0;	// Initializes core

	virtual bool					Init(const char* pszApplicationName, int argc, char **argv) = 0;	// Initializes core for tools

	virtual void					Shutdown() = 0;	// Shutdowns core

	virtual char*					GetApplicationName() const = 0; // returns current application name string

	virtual bool					IsInitialized() const = 0;	// Return status of initialization

	virtual void					AddExceptionCallback(CoreExceptionCallback callback) = 0;
	virtual void					RemoveExceptionCallback(CoreExceptionCallback callback) = 0;

	// now configuration is global for all applications
	virtual KeyValues*				GetConfig() const = 0;

// Interface management for engine

	virtual void					RegisterInterface(const char* pszName, IEqCoreModule* iface) = 0;		// registers interface for faster access
	virtual IEqCoreModule*			GetInterface(const char* pszName) const = 0;							// returns registered interface
	virtual void					UnregisterInterface(const char* pszName) = 0;							// unregisters interface
};

#ifndef _DKLAUNCHER_
IEXPORTS IDkCore*	GetCore();
#endif