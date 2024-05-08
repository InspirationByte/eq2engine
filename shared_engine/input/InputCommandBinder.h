//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Input command binder and host
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "input/in_keys_ident.h"

class IVirtualStream;
class ConCommand;
class ConCommandBase;
class IGPURenderPassRecorder;
struct InputAxisAction;

struct InputCommand
{
	using Func = void(*)(void* userData, short value);

	Func				func{ nullptr };

	const ConCommand*	activateCmd{ nullptr };		// 'plus' command
	const ConCommand*	deactivateCmd{ nullptr };	// 'minus' command
	void*				userData{ nullptr };

	EqString			argumentString;
	EqString			commandString;				// safe for writing
	bool				custom{ false };
};

struct InputBinding : public InputCommand
{
	const InputAxisAction*	boundAxisAction{ nullptr };
	int						modifierIds[2]{ -1, -1 };	// modifier key indices
	int						keyIdx{ -1 };				// native key index
};

struct InputTouchZone : public InputCommand
{
	Vector2D	position;
	Vector2D	size;

	EqString	name;
	int			finger{ -1 };
};

struct InputAxisAction
{
	using Func = void (*)(short value);

	EqString	name;
	Func		func;
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
	bool					BindKey(const char* pszKeyStr, const char* pszCommand, const char *pszArgs);
	InputBinding*			AddBinding(const char* pszKeyStr, const char* pszCommand, const char *pszArgs);
	InputBinding*			AddBinding(const char* pszKeyStr, const char* name, InputCommand::Func func, void* userData = nullptr);

	// removes single binding on specified keychar
	void					UnbindKey( const char* pszKeyStr);
	void					UnbindCommandByName( const char* name, const char* argStr = nullptr);
	void					DeleteBinding( InputBinding* binding );

	// clears and removes all key bindings
	void					UnbindAll();
	void					UnbindAll_Joystick();

	// searches for binding
	InputBinding*			FindBinding(const char* pszKeyStr, const char* cmdPrefixStr = nullptr) const;
	InputBinding*			FindBindingByCommand(ConCommandBase* cmdBase, const char* argStr = nullptr, const InputBinding* startFrom = nullptr) const;
	InputBinding*			FindBindingByCommandName(const char* name, const char* argStr = nullptr, const InputBinding* startFrom = nullptr) const;

	// registers axis action
	// they will be prefixed as "j_" + name
	void					RegisterJoyAxisAction( const char* name, InputAxisAction::Func axisFunc );

	// binding list
	ArrayCRef<InputBinding*>	GetBindingList() const { return m_bindings; }
	ArrayCRef<InputTouchZone>	GetTouchZoneList() const { return m_touchZones; }
	ArrayCRef<InputAxisAction>	GetAxisActionList() const { return m_axisActs; }

	// debug render
	void					DebugDraw(const Vector2D& screenSize, IGPURenderPassRecorder* rendPassRecorder);

	//
	// Event processing
	//
	void					OnKeyEvent( int keyIdent, bool bPressed );
	void					OnMouseEvent( int button, bool bPressed );
	void					OnMouseWheel( int scroll );

	void					OnTouchEvent( const Vector2D& pos, int finger, bool down );
	void					OnJoyAxisEvent( short axis, short value );

protected:
	// executes binding with selected state
	void					ExecuteBinding(InputBinding& binding, short value);
	void					ExecuteTouchZone(InputTouchZone& zone, short value);

	bool					CheckModifiersAndDepress(InputBinding& binding, int keyIdent, bool pressed);
	bool					ResolveCommandBinding(InputBinding& binding, bool quiet);

	InputAxisAction*		FindAxisAction(const char* name) const;

	Array<int>				m_currentButtons{ PP_SL };	// current keyboard buttons
	Array<InputBinding*>	m_bindings{ PP_SL };
	Array<InputTouchZone>	m_touchZones{ PP_SL };
	Array<InputAxisAction>	m_axisActs{ PP_SL };

	bool					m_init{ false };
};

void UTIL_GetBindingKeyString(EqString& outStr, const InputBinding* binding, bool humanReadable = false);
void UTIL_GetBindingKeyStringLocalized(EqWString& outStr, const InputBinding* binding, bool humanReadable = false);

extern CStaticAutoPtr<CInputCommandBinder> g_inputCommandBinder;
