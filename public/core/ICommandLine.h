//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// WARNING: this function declaration is compatible with cmdFilterFn_t
class ConCommandBase;
class EqString;
typedef bool (*cmdLineFilterFn_t)(ConCommandBase* pCmd, Array<EqString>& args);

#define CMDLINE_INTERFACE_VERSION		"CORE_CommandLine_002"

class ICommandLine : public IEqCoreModule
{
public:
	virtual void			Init(const char* pszCommandLine) = 0;
	virtual void			DeInit() = 0;

	virtual void			ExecuteCommandLine(cmdLineFilterFn_t func = nullptr) const = 0;

	virtual int				FindArgument(const char* arg, int startfrom = 0) const = 0;

	virtual const char*		GetArgumentString(int index) const = 0;

	virtual const char*		GetArgumentsOf(int paramIndex) const = 0;
	virtual int				GetArgumentCount() const = 0;
};

INTERFACE_SINGLETON(ICommandLine, CCommandLine, CMDLINE_INTERFACE_VERSION, g_cmdLine )
