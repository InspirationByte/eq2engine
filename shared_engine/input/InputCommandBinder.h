//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Input command binder and host
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "input/in_keys_ident.h"

struct axisAction_t;
class IVirtualStream;
class ConCommand;
class ConCommandBase;
typedef void (*JOYAXISFUNC)(short value);

class in_binding_t
{
public:
	in_binding_t()
	{
		mod_index[0] = -1;
		mod_index[1] = -1;
		key_index = -1;

		cmd_act = nullptr;
		cmd_deact = nullptr;
		boundAction = nullptr;
	}

	EqString		argumentString;
	EqString		commandString;	// safe for writing

	ConCommand*		cmd_act;	// 'plus' command
	ConCommand*		cmd_deact;	// 'minus' command
	axisAction_t*	boundAction;

	int			mod_index[2];	// modifier buttons
	int			key_index;
};

struct in_touchzone_t
{
	in_touchzone_t()
	{
		finger = -1;
		cmd_act = nullptr;
		cmd_deact = nullptr;
	}

	Vector2D position;
	Vector2D size;

	ConCommand*	cmd_act;	// 'plus' command
	ConCommand*	cmd_deact;	// 'minus' command

	EqString	name;

	EqString	argumentString;
	EqString	commandString;	// safe for writing

	int			finger;
};

struct axisAction_t
{
	EqString		name;
	JOYAXISFUNC		func;
};

class CInputCommandBinder
{
public:
							CInputCommandBinder();

	void					Init();
	void					Shutdown();

	void					InitTouchZones();

	// saves binding using file handle
	void					WriteBindings(IVirtualStream* stream);

	// binds a command with arguments to known key
	bool					BindKey( const char* pszKeyStr, const char* pszCommand, const char *pszArgs );

	// removes single binding on specified keychar
	void					UnbindKey( const char* pszKeyStr);
	void					UnbindCommandByName( const char* name, const char* argStr = nullptr);
	void					DeleteBinding( in_binding_t* binding );

	// clears and removes all key bindings
	void					UnbindAll();
	void					UnbindAll_Joystick();

	// searches for binding
	in_binding_t*			FindBinding(const char* pszKeyStr) const;
	in_binding_t*			FindBindingByCommand(ConCommandBase* cmdBase, const char* argStr = nullptr, in_binding_t* startFrom = nullptr) const;
	in_binding_t*			FindBindingByCommandName(const char* name, const char* argStr = nullptr, in_binding_t* startFrom = nullptr) const;

	// registers axis action
	// they will be prefixed as "j_" + name
	void					RegisterJoyAxisAction( const char* name, JOYAXISFUNC axisFunc );

	// binding list
	Array<in_binding_t*>*	GetBindingList() {return &m_bindings;}
	Array<in_touchzone_t>*	GetTouchZoneList() {return &m_touchZones;}
	Array<axisAction_t>*	GetAxisActionList() {return &m_axisActs;}

	// debug render
	void					DebugDraw(const Vector2D& screenSize);

	//
	// Event processing
	//
	void					OnKeyEvent( int keyIdent, bool bPressed );
	void					OnMouseEvent( int button, bool bPressed );
	void					OnMouseWheel( int scroll );

	void					OnTouchEvent( const Vector2D& pos, int finger, bool down );
	void					OnJoyAxisEvent( short axis, short value );

	// executes binding with selected state
	void					ExecuteBinding( in_binding_t* pBinding, bool bState );
	void					ExecuteTouchZone( in_touchzone_t* zone, bool bState );

	template <typename T>
	void					ExecuteBoundCommands(T* zone, bool bState);

	bool					CheckModifiersAndDepress(in_binding_t* binding, int keyIdent, bool bPressed);

protected:
	// adds binding
	in_binding_t*			AddBinding( const char* pszKeyStr, const char* pszCommand, const char *pszArgs );

	bool					ResolveCommandBinding(in_binding_t* binding, bool quiet);

	axisAction_t*			FindAxisAction(const char* name) const;

	Array<int>				m_currentButtons{ PP_SL };	// current keyboard buttons
	Array<in_binding_t*>	m_bindings{ PP_SL };
	Array<in_touchzone_t>	m_touchZones{ PP_SL };
	Array<axisAction_t>		m_axisActs{ PP_SL };

	bool					m_init;
};

bool UTIL_GetBindingKeyIndices(int outKeys[3], const char* pszKeyStr);
void UTIL_GetBindingKeyString(EqString& outStr, in_binding_t* binding, bool humanReadable = false);

extern CInputCommandBinder* g_inputCommandBinder;
