//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////-

#ifndef CMDLINEPARSER_H
#define CMDLINEPARSER_H

#include <stdio.h>
#include "core/ICommandLine.h"
#include "ds/DkList.h"
#include "ds/eqstring.h"

class CCommandLine : public ICommandLine
{
public:
						CCommandLine();
						~CCommandLine();

	void				Init(const char* pszCommandLine);
	void				DeInit();

	void				ExecuteCommandLine(cmdLineFilterFn_t func = nullptr) const;

	const char*			GetArgumentString(int index) const;
	int					FindArgument(const char* arg, int startfrom = 0)  const;

	const char*			GetArgumentsOf(int paramIndex) const;
	int					GetArgumentCount()  const;

	//-------------------------
	bool				IsInitialized() const		{return true;}
	const char*			GetInterfaceName() const	{return CMDLINE_INTERFACE_VERSION;}

protected:
	void				Parse(const char* pszCommandLine);
	void				AddArgument(const char* pFirst, const char* pLast);


	DkList<EqString>	m_args;
};

#endif
