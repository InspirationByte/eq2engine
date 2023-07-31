//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Console variable system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ConCommandBase.h"

class ConCommand;
#define CONCOMMAND_ARGUMENTS ConCommand* cmd, Array<EqString>& args

// use this to access variables inside callback
#define CMD_ARGV(index)		args.ptr()[index]
#define CMD_ARGC			args.numElem()
#define CMD_ARG_CONCAT(val)	{for(int i = 0; i < CMD_ARGC; i++) val = val + CMD_ARGV(i) + ' ';}

typedef void (*CON_COMMAND_CALLBACK)(CONCOMMAND_ARGUMENTS);

class ConCommand : public ConCommandBase
{
public:
	ConCommand(const char* name, CON_COMMAND_CALLBACK callback, int flags = 0);

	template<typename... Ts>
	ConCommand(char const* name, CON_COMMAND_CALLBACK callback, int flags, Ts... ts)
		: ConCommand(name, callback, flags)
	{
		char _[] = { CmdInit<>(this, ts)... };
	}

	// Command execution
	void DispatchFunc(Array<EqString>& args);

private:
	CON_COMMAND_CALLBACK m_fnCallback;
};

#define HOOK_TO_CMD(name)		\
	static ConCommand *name = (ConCommand*)g_consoleCommands->FindCommand(#name);

#define CONCOMMAND_FN(name) CC_##name##_f
#define DECLARE_CONCOMMAND_FN( name ) static void CONCOMMAND_FN(name)(CONCOMMAND_ARGUMENTS)

#define DECLARE_CMD_FN_G(name, cmdfunc, desc, flags)	\
	ConCommand name(#name, cmdfunc, flags, ConCommand::Desc(desc));

#define DECLARE_CMD_FN_RENAME_G(name, cmdname, cmdfunc, desc, flags)	\
	ConCommand name(cmdname, cmdfunc, flags, ConCommand::Desc(desc));

// forward-declared callback only
#define DECLARE_CMD_F_G(name,desc,flags)	\
	DECLARE_CONCOMMAND_FN(name);		\
	ConCommand name(#name, CONCOMMAND_FN(name), flags, ConCommand::Desc(desc));

#define DECLARE_CMD_FN(name, cmdfunc, desc, flags)	\
	static DECLARE_CMD_FN_G(name, cmdfunc, desc, flags)

#define DECLARE_CMD_FN_RENAME(name, cmdname, cmdfunc, desc, flags)	\
	static DECLARE_CMD_FN_RENAME_G(name, cmdname, cmdfunc, desc, flags)

#define DECLARE_CMD_G(name,desc,flags)	\
	DECLARE_CMD_F_G(name, desc, flags)	\
	DECLARE_CONCOMMAND_FN(name)

// forward-declared callback only
#define DECLARE_CMD_F(name,desc,flags)	\
	DECLARE_CMD_F_G(name,desc,flags)

// callback comes right after use of this macro
#define DECLARE_CMD(name,desc,flags)	\
	DECLARE_CMD_G(name, desc, flags)

// uses different 'name' for cvar in code
#define DECLARE_CMD_RENAME(name, cmdname, desc, flags)				\
	DECLARE_CONCOMMAND_FN(name);								\
	static ConCommand name(cmdname, CC_##name##_f, flags, ConCommand::Desc(desc)); \
	DECLARE_CONCOMMAND_FN(name)

#define DECLARE_CMD_VARIANTS_F(name, desc, variantsfn, flags)	\
	DECLARE_CONCOMMAND_FN(name);							\
	static ConCommand name(#name, CONCOMMAND_FN(name), flags, ConCommand::Desc(desc), ConCommand::Variants(variantsfn));

#define DECLARE_CMD_VARIANTS(name, desc, variantsfn, flags)	\
	DECLARE_CMD_VARIANTS_F(name, desc, variantsfn, flags)	\
	DECLARE_CONCOMMAND_FN(name)
