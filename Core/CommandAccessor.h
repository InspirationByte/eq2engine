//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Console command accessor class
//////////////////////////////////////////////////////////////////////////////////

#ifndef CONCOMMANDACCESSOR_H
#define CONCOMMANDACCESSOR_H
#include <stdio.h>
#include <stdlib.h>

#include "DebugInterface.h"
#include "utils/DkList.h"
#include "utils/eqstring.h"
#include "ICommandAccessor.h"

// To prevent stack buffer overflow
#define MAX_SAME_COMMANDS 5

#define COMMANDBUFFER_SIZE (32*1024) // 32 kb buffer, equivalent of MAX_CFG_SIZE

class ConCommandAccessor : public IConCommandAccessor
{
public:
	ConCommandAccessor();
	~ConCommandAccessor();

	// Executes file
	void		ParseFileToCommandBuffer(const char* pszFilename, const char* lookupForCommand = NULL);

	// Sets command buffer
	void		SetCommandBuffer(const char* pszBuffer);

	// Appends to command buffer, also can find only the needed command
	void		AppendToCommandBuffer(const char* pszBuffer);

	// Clears to command buffer
	void		ClearCommandBuffer();

	// After calling ExecuteCommandLine or smth some dlls are connected and we need to retry to execute commands
	//void		AppendFailedCommands();

	// Resets counter of same commands
	void		ResetCounter() {m_nSameCommandsExecuted = 0;}

	// Executes command buffer
	bool		ExecuteCommandBuffer(unsigned int CmdFilterFlags = -1);

	void		EnableInitOnlyVarsChangeProtection(bool bEnable);

private:

	//DkList<DkStr> m_pFailedCommands;

	char m_pszCommandBuffer[COMMANDBUFFER_SIZE];
	char m_pszLastCommandBuffer[COMMANDBUFFER_SIZE];

	int   m_nSameCommandsExecuted;

	bool m_bCanTryExecute;
	bool m_bTrying;
	bool m_bEnableInitOnlyChange;
};

#endif //CONCOMMANDACCESSOR_H
