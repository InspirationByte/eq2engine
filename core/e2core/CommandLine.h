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

	void				Init(ArrayCRef<const char*> args);
	void				DeInit();

	void				ExecuteCommandLine(CommandFilterFn func = nullptr) const;

	ArrayCRef<EqString>	GetParameters() const { return m_args; }
	int					Find(const char* arg, int startfrom = 0)  const;

	const char*			GetArgumentsOf(int paramIndex) const;
	int					GetArgumentsOf(int paramIndex, const char** values, int maxValues) const;

	bool				IsInitialized() const		{return true;}

protected:
	void				Parse(const char* pszCommandLine);
	void				AddArgument(const char* pFirst, const char* pLast);

	Array<EqString>		m_args{ PP_SL };
};