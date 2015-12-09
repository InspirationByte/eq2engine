//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////

#ifndef ICMDLINEPARSER_H
#define ICMDLINEPARSER_H

#include <stdio.h>

#include "core_base_header.h"

#define CMDLINE_INTERFACE_VERSION		"CORE_CommandLine_001"

class ICommandLineParse
{
public:
	virtual void			Init(const char* pszCommandLine) = 0;								//Initializes command line interface. See sys_win.cpp, WINMAIN function
	virtual void			DeInit() = 0;								//De-Initializes command line interface. See sys_win.cpp, WINMAIN function

	virtual	void			Parse_AddParameter(const char* pFirst,const char* pLast ) = 0;
	virtual void			ExecuteCommandLine(bool cvars,bool commands,unsigned int CmdFilterFlags = -1) = 0;

	virtual char*			GetArgumentString(int index) = 0; 					//Returns argument string
	virtual int				FindArgument(const char* arg,int startfrom = 0) = 0;		//Returns argument index
	virtual char*			GetArgumentsOf(int paramIndex) = 0;							//Returns arguments of parameter
	virtual int				GetArgumentCount() = 0;	//Returns argument count
};

#ifndef _DKLAUNCHER_
IEXPORTS ICommandLineParse* GetCmdLine( void );
//INTERFACE_SINGLETON(ICommandLineParse, CommandLineParse, CMDLINE_INTERFACE_VERSION, GetCmdLine())
#endif

#endif //ICMDLINEPARSER_H
