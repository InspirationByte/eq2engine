//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////


#ifndef CONCOMMAND_H
#define CONCOMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include "ConCommandBase.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"
#include "InterfaceManager.h"


typedef void (*CON_COMMAND_CALLBACK)(DkList<EqString>*);

class ConCommand : public ConCommandBase
{
public:
	ConCommand(char const *name,CON_COMMAND_CALLBACK callback,char const *desc = 0, int flags = 0);

	//Command execution
	void DispatchFunc(DkList<EqString> *args);

private:
	void Create(char const *pszName,CON_COMMAND_CALLBACK callback,char const *pszHelpString, int nFlags);

	CON_COMMAND_CALLBACK m_fnCallback;
};

#define DECLARE_CMD(name,desc,flags) \
	void CC_##name##_f(DkList<EqString> *args); \
	static ConCommand cmd_##name(#name,CC_##name##_f,desc,flags); \
	void CC_##name##_f(DkList<EqString> *args)

#define DECLARE_CMD_STRING(name,localname,desc,flags) \
	void CC_##name##_f(DkList<EqString> *args); \
	static ConCommand cmd_##name(localname,CC_##name##_f,desc,flags); \
	void CC_##name##_f(DkList<EqString> *args)

#define DECLARE_CMD_NONSTATIC(name,desc,flags) \
	void CC_##name##_f(DkList<EqString> *args); \
	ConCommand cmd_##name##(#name,CC_##name##_f,desc,flags); \
	void CC_##name##_f(DkList<EqString> *args)

#define CMD_ARGV(index)		args->ptr()[index]
#define CMD_ARGC			args->numElem()
#define CMD_ARG_CONCAT(val)	{for(int i = 0; i < CMD_ARGC; i++) val = val + CMD_ARGV(i) + ' ';}

#endif //CONCOMMAND_H
