//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: KeyBinding list
//////////////////////////////////////////////////////////////////////////////////


#ifndef KEYS_H
#define KEYS_H

#include <stdio.h>
#include <stdlib.h>
#include "ConCommand.h"
#include "utils/DkList.h"
#include "in_keys_ident.h"
#include "IFileSystem.h"

// binding
class binding_t
{
public:
	binding_t()
	{
		key_index = -1;
		isMouse = false;
	}

	EqString	argumentString;
	EqString	commandString;	// safe for writing

	ConCommand*	boundCommand1;	// 'plus' command
	ConCommand*	boundCommand2;	// 'minus' command

	int		key_index;
	bool	isMouse;
};

//
class CKeyCommandBinder
{
public:
						CKeyCommandBinder();

	// saves binding using file handle
	void				WriteBindings(DKFILE *pCFGFile);

	// binds a command with arguments to known key
	void				BindKey( char* pszCommand, char *pszArgs, const char* pszKeyStr );

	// searches for first key
	int					GetBindingIndexByKey(uint keyIdent);

	// removes single binding on specified keychar
	void				RemoveBinding( const char* pszKeyStr);

	// clears and removes all key bindings
	void				UnbindAll();

	// searches for binding
	binding_t*			FindBinding(const char* pszKeyStr);

	// binding list
	DkList<binding_t*>*	GetBindingList() {return &m_pBindings;}

	//
	// Event processing
	//
	void				OnKeyEvent( const int keyIdent, bool bPressed );
	void				OnMouseEvent( const int button, bool bPressed );
	void				OnMouseWheel( const int scroll );

	// executes binding with selected state
	void				ExecuteBinding( binding_t* pBinding, bool bState );

private:
	DkList<binding_t*>	m_pBindings;
};

CKeyCommandBinder* GetKeyBindings( void );

#endif //KEYS_H
