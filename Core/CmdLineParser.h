//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////-

#ifndef CMDLINEPARSER_H
#define CMDLINEPARSER_H

#include <stdio.h>
#include "ICmdLineParser.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"

class CommandLineParse : public ICommandLineParse
{
public:
						CommandLineParse();
						~CommandLineParse();

	void				Init(const char* pszCommandLine);								//Initializes command line interface. See sys_win.cpp, WINMAIN function
	void				DeInit();								//De-Initializes command line interface. See sys_win.cpp, WINMAIN function

	void				Parse();
	void				Parse_AddParameter(const char* pFirst,const char* pLast );
	void				ExecuteCommandLine(bool cvars,bool commands,unsigned int CmdFilterFlags = -1);

	char*				GetArgumentString(int index); 					//Returns argument string
	int					FindArgument(const char* arg,int startfrom = 0);		//Returns argument index
	char*				GetArgumentsOf(int paramIndex);						//Returns arguments of parameter
	int					GetArgumentCount() { return m_iArgsCount;}				//Returns argument count

	bool				IsArgument(char* pszString, bool skipMinus = false);									//Is this argument?
protected:
	int					m_iArgsCount;
	DkList<EqString>	m_szParams;
	EqString			m_szFullArgString;
};

#endif
