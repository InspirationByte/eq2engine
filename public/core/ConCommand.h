//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////


#ifndef CONCOMMAND_H
#define CONCOMMAND_H

#include <stdio.h>
#include <stdlib.h>
#include "ConCommandBase.h"

#define CONCOMMAND_ARGUMENTS ConCommand* cmd, Array<EqString>& args

#define CONCOMMAND_FN(name) CC_##name##_f

#define DECLARE_CONCOMMAND_FN( name ) void CONCOMMAND_FN(name)(CONCOMMAND_ARGUMENTS)

#define CMD_ARGV(index)		args.ptr()[index]
#define CMD_ARGC			args.numElem()
#define CMD_ARG_CONCAT(val)	{for(int i = 0; i < CMD_ARGC; i++) val = val + CMD_ARGV(i) + ' ';}

#define DECLARE_CMD(name,desc,flags)				\
	DECLARE_CONCOMMAND_FN(name);					\
	static ConCommand cmd_##name(#name,CONCOMMAND_FN(name),desc,flags); \
	DECLARE_CONCOMMAND_FN(name)

#define DECLARE_CMD_RENAME(name,localname,desc,flags)				\
	DECLARE_CONCOMMAND_FN(name);									\
	static ConCommand cmd_##name(localname,CC_##name##_f,desc,flags); \
	DECLARE_CONCOMMAND_FN(name)

#define DECLARE_CMD_VARIANTS(name,desc,variantsfn,flags)				\
	DECLARE_CONCOMMAND_FN(name);					\
	static ConCommand cmd_##name(#name,CONCOMMAND_FN(name), variantsfn,desc,flags); \
	DECLARE_CONCOMMAND_FN(name)

class ConCommand;

typedef void (*CON_COMMAND_CALLBACK)(CONCOMMAND_ARGUMENTS);

class ConCommand : public ConCommandBase
{
public:
	ConCommand(char const *name,CON_COMMAND_CALLBACK callback,char const *desc = 0, int flags = 0);
	ConCommand(char const *name,CON_COMMAND_CALLBACK callback, CMDBASE_VARIANTS_CALLBACK variantsList,char const *desc = 0, int flags = 0);

	// Command execution
	void DispatchFunc(Array<EqString>& args);

private:
	void Create(char const *pszName,CON_COMMAND_CALLBACK callback, CMDBASE_VARIANTS_CALLBACK variantsList,char const *pszHelpString, int nFlags);

	CON_COMMAND_CALLBACK m_fnCallback;
};

#define HOOK_TO_CMD(name)		\
	static ConCommand *name = (ConCommand*)g_consoleCommands->FindCommand(#name);

#endif //CONCOMMAND_H
