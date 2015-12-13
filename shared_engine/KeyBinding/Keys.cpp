//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: KeyBinding list
//////////////////////////////////////////////////////////////////////////////////

#include "KeyBinding/Keys.h"
#include "cfgloader.h"

#include "IConCommandFactory.h"
#include "DebugInterface.h"
#include "utils/strtools.h"
#include "InterfaceManager.h"

#if !defined(EDITOR) && !defined(NO_ENGINE)
#include "IEngineGame.h"
#include "sys_console.h"
#endif

_INTERFACE_FUNCTION(CKeyCommandBinder, CKeyCommandBinder, GetKeyBindings)

//EXPOSE_SINGLE_INTERFACE_NOBASE_EX(KeyBindings,CKeyCommandBinder);

CKeyCommandBinder::CKeyCommandBinder()
{

}

// saves binding using file handle
void CKeyCommandBinder::WriteBindings(DKFILE *pCFGFile)
{
	if(!pCFGFile)
		return;

	pCFGFile->Print("unbindall\n" );

	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		// resolve key name
		EqString keyNameString = s_keynames[ m_pBindings[i]->key_index ].name;

		pCFGFile->Print("bind %s %s %s\n",	keyNameString.GetData(),
																m_pBindings[i]->commandString.GetData(),
																m_pBindings[i]->argumentString.GetData() );
	}
}


// binds a command with arguments to known key
void CKeyCommandBinder::BindKey( char* pszCommand, char *pszArgs, const char* pszKeyStr )
{
	// check if key bound

	// Find the key matching the *keychar
	int keyindex = KeyStringToKeyIndex( pszKeyStr );

	if(keyindex == -1)
	{
		MsgError("Unknown key identifier '%s'\n", pszKeyStr);
		return;
	}

	// create new binding
	binding_t *keybind = new binding_t;

	keybind->key_index = keyindex;

	keybind->commandString = pszCommand;

	if(pszArgs)
		keybind->argumentString = pszArgs;

	// recognize mouse command
	if(_Es(pszKeyStr).Find("mouse") != -1)
		keybind->isMouse = true;

	// if we connecting libraries dynamically, that wouldn't properly execute
	keybind->boundCommand1 = (ConCommand*)GetCvars()->FindCommand(varargs("+%s", pszCommand));
	keybind->boundCommand2 = (ConCommand*)GetCvars()->FindCommand(varargs("-%s", pszCommand));

	// if found only one command with plus or minus
	if(!keybind->boundCommand1 || !keybind->boundCommand2)
		keybind->boundCommand1 = (ConCommand*)GetCvars()->FindCommand( pszCommand );

	// if anly command found
	if(keybind->boundCommand1 || keybind->boundCommand2)
	{
		m_pBindings.append( keybind );
	}
	else
	{
		MsgError("Unknown command '%s'\n", pszCommand);
		delete keybind;
	}
}


// searches for first key
int CKeyCommandBinder::GetBindingIndexByKey(uint keyNum)
{
	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == keyNum)
			return i;
	}

	return -1;
}

// removes single binding on specified keychar
void CKeyCommandBinder::RemoveBinding(const char* pszKeyStr)
{
	int index = KeyStringToKeyIndex( pszKeyStr );
	if(index == -1)
	{
		Msg("Unknown key identifier '%s'\n", pszKeyStr);
		return;
	}

	bool bResult = false;

	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(m_pBindings[i]->key_index == index)
		{
			delete m_pBindings[i];

			m_pBindings.removeIndex(i);
			i--;

			bResult = true;
		}
	}

	if(bResult)
		MsgInfo("'%s' unbound\n", pszKeyStr);
}

// clears and removes all key bindings
void CKeyCommandBinder::UnbindAll()
{
	for(int i = 0; i < m_pBindings.numElem();i++)
		delete m_pBindings[i];

	m_pBindings.clear();
}

// searches for binding
binding_t* CKeyCommandBinder::FindBinding(const char* pszKeyStr)
{
	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(!stricmp( s_keynames[m_pBindings[i]->key_index].name, pszKeyStr ))
			return m_pBindings[i];
	}

	return NULL;
}

//
// Event processing
//
void CKeyCommandBinder::OnKeyEvent(const int keyIdent, bool bPressed)
{
	for(int i = 0; i < m_pBindings.numElem(); i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == keyIdent)
		{
			ExecuteBinding( m_pBindings[i], bPressed);
		}
	}
}

void CKeyCommandBinder::OnMouseEvent( const int button, bool bPressed )
{
	for(int i = 0; i < m_pBindings.numElem(); i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == button)
		{
			ExecuteBinding( m_pBindings[i], bPressed);
		}
	}
}

void CKeyCommandBinder::OnMouseWheel( const int scroll )
{
	int button = (scroll > 0) ?  MOU_WHUP : MOU_WHDN;

	for(int i = 0; i < m_pBindings.numElem(); i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == button)
		{
			ExecuteBinding( m_pBindings[i], (scroll > 0));
		}
	}
}

// executes binding with selected state
void CKeyCommandBinder::ExecuteBinding( binding_t* pBinding, bool bState )
{
	DkList<EqString> args;

	xstrsplit( pBinding->argumentString.GetData(), " ", args);

	ConCommand *cmd = NULL;

	if(bState) //Handle press and de-press
		cmd = pBinding->boundCommand1;
	else
		cmd = pBinding->boundCommand2;

	// if there is only one command
	if(!cmd && bState)
		cmd = pBinding->boundCommand1;

	// dispatch command
	if(cmd)
	{
#if !defined(EDITOR) && !defined(NO_ENGINE)
		// skip client controls if console enabled
		if((cmd->GetFlags() & CV_CLIENTCONTROLS) && (engine->GetGameState() == IEngineGame::GAME_PAUSE || g_pSysConsole->IsVisible()))
			return;
#endif

		cmd->DispatchFunc( &args );
	}

	return;
}

#ifndef DLL_EXPORT
DECLARE_CMD(bind,"Binds action to key",0)
{
	if(args)
	{
		if(args->numElem() > 1)
		{
			EqString agrstr;

			for(int i = 2; i < args->numElem(); i++)
				agrstr.Append(varargs("%s ",args->ptr()[i].GetData()));

			GetKeyBindings()->BindKey((char*)args->ptr()[1].GetData(),(char*)agrstr.GetData(), (char*)args->ptr()[0].GetData());
		}
		else
			MsgInfo("Example: bind <key> <command> [args,...]\n");
	}
	else
		MsgInfo("Example: bind <key> <command> [args,...]\n");
}

DECLARE_CMD(list_binding,"Shows bound keys",0)
{
	MsgInfo("Actions binded to keys list\n");
	DkList<binding_t*> *bindingList = GetKeyBindings()->GetBindingList();

	for(int i = 0; i < bindingList->numElem();i++)
	{
		EqString string = s_keynames[bindingList->ptr()[i]->key_index].name;

		Msg("bind %s %s %s\n", string.GetData(), bindingList->ptr()[i]->commandString.GetData(), bindingList->ptr()[i]->argumentString.GetData() );
	}

	MsgInfo("Actions binded to keys list end\n");
}

DECLARE_CMD(unbind,"Unbinds a key",0)
{
	if(args && args->numElem() > 0)
	{
		GetKeyBindings()->RemoveBinding(args->ptr()[0].GetData());
	}
}

DECLARE_CMD(unbindall,"Unbinds all keys",0)
{
	GetKeyBindings()->UnbindAll();
}

#endif // DLL_EXPORT
