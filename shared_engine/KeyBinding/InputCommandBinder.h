//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Input command binder and host
//////////////////////////////////////////////////////////////////////////////////


#ifndef INPUTCOMMANDBINDER_H
#define INPUTCOMMANDBINDER_H

#include <stdio.h>
#include <stdlib.h>
#include "ConCommand.h"
#include "utils/DkList.h"
#include "in_keys_ident.h"
#include "IFileSystem.h"

#include "math/Vector.h"

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

struct touchzone_t
{
	touchzone_t()
	{
		finger = -1;
	}

	Vector2D position;
	Vector2D size;

	ConCommand*	boundCommand1;	// 'plus' command
	ConCommand*	boundCommand2;	// 'minus' command

	EqString	name;

	EqString	argumentString;
	EqString	commandString;	// safe for writing

	int			finger;
};

typedef void (*JOYAXISFUNC)( short value );

struct axisAction_t
{
	EqString		name;
	JOYAXISFUNC		func;
};

//
class CInputCommandBinder
{
public:
							CInputCommandBinder();

	void					Init();
	void					Shutdown();

	void					InitTouchZones();

	// saves binding using file handle
	void					WriteBindings(IFile* cfgFile);

	// binds a command with arguments to known key
	void					BindKey( const char* pszCommand, const char *pszArgs, const char* pszKeyStr );

	// returns binding
	binding_t*				LookupBinding(uint keyIdent);

	// removes single binding on specified keychar
	void					RemoveBinding( const char* pszKeyStr);

	// clears and removes all key bindings
	void					UnbindAll();
	void					UnbindAll_Joystick();

	// searches for binding
	binding_t*				FindBinding(const char* pszKeyStr);

	// registers axis action
	void					RegisterJoyAxisAction( int axis, const char* name, JOYAXISFUNC axisFunc );
	void					BindJoyAxis( int axis, const char* actionName );

	// binding list
	DkList<binding_t*>*		GetBindingList() {return &m_pBindings;}
	DkList<touchzone_t>*	GetTouchZoneList() {return &m_touchZones;}

	// debug render
	void					DebugDraw(const Vector2D& screenSize);

	//
	// Event processing
	//
	void					OnKeyEvent( const int keyIdent, bool bPressed );
	void					OnMouseEvent( const int button, bool bPressed );
	void					OnMouseWheel( const int scroll );

	void					OnTouchEvent( const Vector2D& pos, int finger, bool down );

	// executes binding with selected state
	void					ExecuteBinding( binding_t* pBinding, bool bState );
	void					ExecuteTouchZone( touchzone_t* zone, bool bState );

	template <typename T>
	void					ExecuteBoundCommands(T* zone, bool bState);

private:
	DkList<binding_t*>		m_pBindings;
	DkList<touchzone_t>		m_touchZones;
	DkList<axisAction_t>	m_axisActs;
};

extern CInputCommandBinder* g_inputCommandBinder;

#endif //INPUTCOMMANDBINDER_H
