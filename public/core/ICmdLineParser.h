//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////

#ifndef ICMDLINEPARSER_H
#define ICMDLINEPARSER_H

#include <stdio.h>

#include "InterfaceManager.h"

#define CMDLINE_INTERFACE_VERSION		"CORE_CommandLine_001"

class ICommandLineParse : public ICoreModuleInterface
{
public:
	virtual void			Init(const char* pszCommandLine) = 0;
	virtual void			DeInit() = 0;

	virtual void			ExecuteCommandLine(bool cvars,bool commands,unsigned int CmdFilterFlags = -1) const = 0;

	virtual int				FindArgument(const char* arg, int startfrom = 0) const = 0;

	virtual const char*		GetArgumentString(int index) const = 0;

	virtual const char*		GetArgumentsOf(int paramIndex) const = 0;
	virtual int				GetArgumentCount() const = 0;
};

INTERFACE_SINGLETON( ICommandLineParse, CommandLineParse, CMDLINE_INTERFACE_VERSION, g_cmdLine )

#endif //ICMDLINEPARSER_H
