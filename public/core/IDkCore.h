//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: MuscleCore interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class KeyValues;
class ICommandLine;

using CoreExceptionCallback = void(*)();

struct CoreDebugSettings
{
	bool fullCrashDumps = false;
	bool crashOnAssert = false;
	bool assertPromptInDebugger = false;
	bool printMemLeaksAtExit = false;
};

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
	virtual KeyValues*					GetConfig() const = 0;
	virtual const CoreDebugSettings&	GetDebugSettings() const = 0;

// Interface management for engine

	virtual void					RegisterInterface(const char* pszName, IEqCoreModule* iface) = 0;		// registers interface for faster access
	virtual IEqCoreModule*			GetInterface(const char* pszName) const = 0;							// returns registered interface
	virtual void					UnregisterInterface(const char* pszName) = 0;							// unregisters interface

	template<typename T> void		RegisterInterface(T* iface) { RegisterInterface(T::CoreInterfaceName(), iface); }
	template<typename T> void		UnregisterInterface() 		{ UnregisterInterface(T::CoreInterfaceName()); }
	template<typename T> T*			GetInterface() const 		{ return static_cast<T*>(GetInterface(T::CoreInterfaceName())); }

	virtual void					OnModuleLoaded(const char* pszName) = 0;
	virtual void					OnModuleUnloaded(const char* pszName) = 0;
};

ENTRYPOINT_INTERFACE_SINGLETON(IDkCore, CDkCore, g_eqCore)