//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Console command accessor class
//////////////////////////////////////////////////////////////////////////////////

#ifndef ICONCOMMANDACCESSOR_H
#define ICONCOMMANDACCESSOR_H
#include <stdio.h>
#include <stdlib.h>

#define CMDACCESSOR_INTERFACE_VERSION		"CORE_CommandAccessor_001"

class IConCommandAccessor
{
public:
    // Executes file
    virtual void	ParseFileToCommandBuffer(const char* pszFilename, const char* lookupForCommand = NULL) = 0;

    // Sets command buffer
    virtual void	SetCommandBuffer(const char* pszBuffer) = 0;

    // Appends to command buffer
    virtual void	AppendToCommandBuffer(const char* pszBuffer) = 0;

    // Clears to command buffer
    virtual void	ClearCommandBuffer() = 0;

    // Resets counter of same commands
    virtual void	ResetCounter() = 0;

    // After calling ExecuteCommandLine or smth some dlls are connected and we need to retry to execute commands
   // virtual void	AppendFailedCommands() = 0;

    // Executes command buffer
    virtual bool	ExecuteCommandBuffer(unsigned int CmdFilterFlags = -1) = 0;	
};

IEXPORTS IConCommandAccessor *GetCommandAccessor( void );

#endif //ICONCOMMANDACCESSOR_H
