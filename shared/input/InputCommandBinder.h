//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Input command binder and host
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "input/in_keys_ident.h"

class IFileStream;
class ConCommand;
class ConCommandBase;
class IGPURenderPassRecorder;
struct InputVectorAction;

enum EInputDeviceType : int
{
	INPUTDEV_KEYBOARD	= (1 << 0),
	INPUTDEV_MOUSE		= (1 << 1),
	INPUTDEV_CONTROLLER = (1 << 2),
	INPUTDEV_TOUCHPAD	= (1 << 3)
};

struct InputCommand
{
	using Func = void(*)(void* userData, const Vector3D& value);

	Func				func{ nullptr };

	const ConCommand*	activateCmd{ nullptr };		// 'plus' command
	const ConCommand*	deactivateCmd{ nullptr };	// 'minus' command
	void*				userData{ nullptr };

	EqString			argumentString;
	EqString			commandString;				// safe for writing
	bool				custom{ false };
	bool				isError{ false };
};

struct InputBinding : public InputCommand
{
	const InputVectorAction*	boundAxisAction{ nullptr };
	int						modifierIds[2]{ -1, -1 };	// modifier key indices
	int						keyIdx{ -1 };				// native key index
};

struct InputVectorAction
{
	using Func = void (*)(void* userData, const Vector3D& value);

	EqString	name;
	Func		func;
	void*		userData{ nullptr };
	int			axisCount{ 0 };
};

struct InputTouchZone : public InputCommand
{
	Vector2D	position{ vec2_zero };
	Vector2D	size{ 1.0f };

	EqString	name;
	int			finger{ -1 };
};

class CInputCommandBinder
{
public:
	CInputCommandBinder();

	void					Init();
	void					Shutdown();

	void					InitTouchZones();

	EInputDeviceType		GetLastInputDeviceUsed() const { return m_lastInputDev; }

	// saves binding using file handle
	void					WriteBindings(IFileStream* stream);

	// binds a command with arguments to known key
	bool					AddBinding(const char* key, const char* command, const char* args);
	InputBinding*			AddBinding(const char* key, const char* name, InputCommand::Func func, void* userData = nullptr);

	// removes single binding on specified keychar
	void					UnbindKey( const char* pszKeyStr );
	void					UnbindCommandByName( const char* name, const char* argStr = nullptr);
	void					DeleteBinding(InputBinding* binding);

	// clears and removes all key bindings
	void					UnbindAll();
	void					UnbindAll_Joystick();

	// searches for binding
	InputBinding*			FindBinding(const char* key, const char* cmdPrefix = nullptr) const;
	InputBinding*			FindBindingByCommand(ConCommandBase* cmdBase, const char* argStr = nullptr, const InputBinding* startFrom = nullptr) const;
	InputBinding*			FindBindingByCommandName(const char* name, const char* argStr = nullptr, const InputBinding* startFrom = nullptr) const;

	// registers axis action
	// they will be prefixed as "j_" + name
	void					CreateVectorAction( const char* name, InputVectorAction::Func axisFunc, int axisCount = 1, void* userData = nullptr);
	void					RemoveVectorAction(const char* name);

	// binding list
	ArrayCRef<InputBinding*>	GetBindingList() const { return m_bindings; }
	ArrayCRef<InputTouchZone>	GetTouchZoneList() const { return m_touchZones; }
	ArrayCRef<InputVectorAction>	GetAxisActionList() const { return m_axisActs; }

	// debug render
	void					DebugDraw(const Vector2D& screenSize, IGPURenderPassRecorder* rendPassRecorder);

	//
	// Event processing
	//
	void					OnPressEvent( int keyIdent, bool down );
	void					OnVectorEvent( int keyIdent, const Vector3D& value );
	void					OnTouchEvent( int finger, const Vector2D& position, bool down );

protected:
	InputBinding*			CreateCommandBinding(const char* pszKeyStr, const char* pszCommand, const char* pszArgs);

	// executes binding with selected state
	void					ExecuteBinding(InputBinding& binding, const Vector3D& value);
	void					ExecuteTouchZone(InputTouchZone& zone, const Vector3D& value);

	bool					CheckModifiersAndDepress(InputBinding& binding, int keyIdent, bool pressed);
	bool					ResolveCommandBinding(InputBinding& binding, bool quiet);

	InputVectorAction*		FindAxisAction(const char* name) const;

	static constexpr int	BITS_BUTTONS = 1024;
	BIT_STORAGE_TYPE		m_currentButtonBits[bitArray2Dword(BITS_BUTTONS)]{0};	// current keyboard buttons
	Array<InputBinding*>	m_bindings{ PP_SL };
	Array<InputTouchZone>	m_touchZones{ PP_SL };
	Array<InputVectorAction>	m_axisActs{ PP_SL };

	EInputDeviceType		m_lastInputDev{ INPUTDEV_KEYBOARD };

	bool					m_init{ false };
};

int UTIL_GetBindingKeyIds(int keyIds[3], const InputBinding* binding);
void UTIL_GetBindingKeyString(EqString& outStr, const InputBinding* binding, bool humanReadable = false);
void UTIL_GetBindingKeyStringLocalized(EqWString& outStr, const InputBinding* binding, bool humanReadable = false);

extern CStaticAutoPtr<CInputCommandBinder> g_inputCommandBinder;
