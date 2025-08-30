//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: MuscleCore interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct KVSection;
class ICommandLine;

using CoreExceptionCallback = void(*)();

struct CoreDebugSettings
{
	bool fullCrashDumps = false;
	bool crashOnAssert = false;
	bool assertPromptInDebugger = false;
	bool printMemLeaksAtExit = false;
};

struct CoreAppInitParameters
{
	using CommandLine = ArrayCRef<const char*>;
	const char*		appConfigName = nullptr;		// config name. if null, defaults to E2.CONFIG
	const char*		appName = nullptr;
	CommandLine		commandLine = nullptr;
};

struct OSModule;

// DarkTech core interface
class IDkCore
{
public:
	virtual ~IDkCore() {}
	
	virtual bool					Init(const CoreAppInitParameters& initParams) = 0;
	virtual void					Shutdown() = 0;	// Shutdowns core

	virtual char*					GetApplicationName() const = 0; // returns current application name string

	virtual bool					IsInitialized() const = 0;	// Return status of initialization

	virtual void					AddExceptionCallback(CoreExceptionCallback callback) = 0;
	virtual void					RemoveExceptionCallback(CoreExceptionCallback callback) = 0;

	// now configuration is global for all applications
	virtual const KVSection&			GetConfig() const = 0;
	virtual const CoreDebugSettings&	GetDebugSettings() const = 0;

// Dynamic library stuff

	// load module
	virtual OSModule*				OpenModule(const char* mod_name, EqString* outError = nullptr) = 0;

	// free module
	virtual void					CloseModule(OSModule* pModule) = 0;

	// returns procedure address of the loaded module
	virtual void*					GetProcedureAddress(OSModule* pModule, const char* pszProc) const = 0;

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