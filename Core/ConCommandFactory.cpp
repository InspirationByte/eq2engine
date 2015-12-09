//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Variables factory - DarkTech Core Implementation
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include "ConCommandFactory.h"
#include "ConVar.h"
#include "ConCommand.h"
#include "core_base_header.h"
#include "DebugInterface.h"

#ifdef _WIN32
EXPOSE_SINGLE_INTERFACE_EXPORTS_EX(Cvars,ConCommandFactory,IConCommandFactory)
#else
	//static ConCommandFactory g_Cvars;

	IConCommandFactory *s_pCvars = NULL; //( IConCommandFactory * )&g_Cvars;

    IEXPORTS IConCommandFactory *GetCvars( void )
    {
        if(!s_pCvars)
            s_pCvars = new ConCommandFactory();
        return s_pCvars;
    }
#endif

#ifdef _DEBUG
#	define _CRTDBG_MAP_ALLOC
#	include <malloc.h>
#	include <crtdbg.h>
#endif
/*

// Don't touch!
IEXPORTS IConCommandFactory* GetCvars()
{
	static ConCommandFactory g_CmdFactory;
    return &g_CmdFactory;
}
*/
ConCommandFactory::ConCommandFactory()
{
	m_bCanTryExecute = true;
	m_bTrying = false;
}

void MsgFormatCVListText(ConVar *pConVar)
{
	Msg("%s = \"%s\"\n\n",pConVar->GetName(),pConVar->GetString());
	MsgError("   \"%s\"\n",pConVar->GetDesc());
}

void MsgFormatCMDListText(ConCommandBase *pConCommand)
{
	Msg("%s\n",pConCommand->GetName());
	MsgError("   \"%s\"\n",pConCommand->GetDesc());
}

void CC_Cvarlist_f(DkList<EqString> *args)
{
	MsgWarning("********Variable list starts*********\n");

	const ConCommandBase	*pBase;			// Temporary Pointer to cvars

	int iCVCount = 0;
	// Loop through cvars...

	const DkList<ConCommandBase*> *pCommandBases = GetCvars()->GetAllCommands();

	for ( int i = 0; i < pCommandBases->numElem(); i++ )
	{
		pBase = pCommandBases->ptr()[i];

		if(pBase->IsConVar())
		{
			ConVar *cv = (ConVar*)pBase;

			if(!(cv->GetFlags() & CV_INVISIBLE))
				MsgFormatCVListText(cv);

			iCVCount++;
		}
	}

	MsgWarning(" %i Total Cvars\n",iCVCount);
	MsgWarning("********Variable list ends*********\n");
}

void CC_Cmdlist_f(DkList<EqString> *args)
{
	MsgWarning("********Command list starts*********\n");
	const ConCommandBase	*pBase;			// Temporary Pointer to cvars
	int iCVCount = 0;
	// Loop through cvars...
	const DkList<ConCommandBase*> *pCommandBases = GetCvars()->GetAllCommands();

	for ( int i = 0; i < pCommandBases->numElem(); i++ )
	{
		pBase = pCommandBases->ptr()[i];

		if(pBase->IsConCommand())
		{
			if(!(pBase->GetFlags() & CV_INVISIBLE))
				MsgFormatCMDListText((ConCommandBase*)pBase);

			iCVCount++;
		}
	}

	MsgWarning(" %i Total Commands\n",iCVCount);
	MsgWarning("********Command list ends*********\n");
}

ConCommand cvarlist("cvarlist",CC_Cvarlist_f,"Prints out all aviable cvars",CV_UNREGISTERED);
ConCommand cmdlist("cmdlist",CC_Cmdlist_f,"Prints out all aviable commands",CV_UNREGISTERED);

void ConCommandFactory::RegisterCommands()
{
	RegisterCommand(&cvarlist);
	RegisterCommand(&cmdlist);
}

//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-
//	Console variable factory
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

const ConVar *ConCommandFactory::FindCvar(const char* name)
{
	ConCommandBase const *pBase = FindBase(name);
	if(pBase)
	{
		if(pBase->IsConVar())
			return ( ConVar* )pBase;
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

const ConCommand *ConCommandFactory::FindCommand(const char* name)
{
	ConCommandBase const *pBase = FindBase(name);
	if(pBase)
	{
		if(pBase->IsConCommand())
			return ( ConCommand* )pBase;
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

const ConCommandBase *ConCommandFactory::FindBase(const char* name)
{
	for ( int i = 0; i < m_pCommandBases.numElem(); i++ )
	{
		if ( !stricmp( name, m_pCommandBases[i]->GetName() ) )
			return m_pCommandBases[i];
	}
	return NULL;
}

int alpha_cmd_comp(const void * a, const void * b)
{
	ConCommandBase* pBase1 = *(ConCommandBase**)a;
	ConCommandBase* pBase2 = *(ConCommandBase**)b;

	return stricmp(pBase1->GetName(),pBase2->GetName());//( *(int*)a - *(int*)b );
}

void ConCommandFactory::RegisterCommand(ConCommandBase *pCmd)
{
	ASSERT(pCmd != NULL);
	ASSERT(FindBase(pCmd->GetName()) == NULL);

	//if(FindBase(pCmd->GetName()) != NULL)
	//	MsgError("%s already declared.\n",pCmd->GetName());

	// Already registered
	if ( pCmd->IsRegistered() )
		return;

	m_bCanTryExecute = true;

	m_pCommandBases.append( pCmd );

	// alphabetic sort
	qsort(m_pCommandBases.ptr(), m_pCommandBases.numElem(), sizeof(ConCommandBase*), alpha_cmd_comp);
}

void ConCommandFactory::UnregisterCommand(ConCommandBase *pCmd)
{
	if ( !pCmd->IsRegistered() )
		return;

	for ( int i = 0; i < m_pCommandBases.numElem(); i++ )
	{
		if ( !stricmp( pCmd->GetName(), m_pCommandBases[i]->GetName() ) )
		{
			m_pCommandBases.removeIndex(i);
			return;
		}
	}
}

void ConCommandFactory::DeInit()
{
	m_pFailedCommands.clear();
}