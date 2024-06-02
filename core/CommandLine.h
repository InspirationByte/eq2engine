//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Command line parser
//////////////////////////////////////////////////////////////////////////////////-

#pragma once
#include "core/ICommandLine.h"

class CCommandLine : public ICommandLine
{
public:
	CCommandLine();
	~CCommandLine();

	void				Init(const char* pszCommandLine);
	void				DeInit();

	void				ExecuteCommandLine(CommandFilterFn func = nullptr) const;

	const char*			GetArgumentString(int index) const;
	int					FindArgument(const char* arg, int startfrom = 0)  const;

	const char*			GetArgumentsOf(int paramIndex) const;
	int					GetArgumentsOf(int paramIndex, const char** values, int maxValues) const;
	int					GetArgumentCount()  const;

	//-------------------------
	bool				IsInitialized() const		{return true;}

protected:
	void				Parse(const char* pszCommandLine);
	void				AddArgument(const char* pFirst, const char* pLast);

	Array<EqString>		m_args{ PP_SL };
};