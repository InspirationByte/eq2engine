//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: KeyBinding list
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/ILocalize.h"
#include "utils/KeyValues.h"

#include <SDL.h>

#include "InputCommandBinder.h"
#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "font/IFontCache.h"


static constexpr const char INPUT_CMD_ACTIVATE_PREFIX		= '+';
static constexpr const char INPUT_CMD_DEACTIVATE_PREFIX		= '-';

CStaticAutoPtr<CInputCommandBinder> g_inputCommandBinder;

DECLARE_CVAR(in_keys_debug, "0", "Debug CInputCommandBinder", 0);
DECLARE_CVAR(in_touchzones_debug, "0", "Debug touch zones on screen and messages", CV_CHEAT);

DECLARE_CMD(in_touchzones_reload, "Reload touch zones", 0)
{
	g_inputCommandBinder->InitTouchZones();
}

static void cmd_conKeyList(const ConCommandBase* base, Array<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	InputKeyMap* names = s_keyMapList;

	do
	{
		InputKeyMap& name = *names;

		if (name.name == nullptr)
			break;

		if (list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		if (*query == 0 || xstristr(name.name, query))
			list.append(name.name);

	} while (names++);
}

static void cmd_boundKeysList(const ConCommandBase* base, Array<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	ArrayCRef<InputBinding*> bindingList = g_inputCommandBinder->GetBindingList();

	EqString keyNameString;
	for (const InputBinding* binding : bindingList)
	{
		keyNameString.Clear();
		UTIL_GetBindingKeyString(keyNameString, binding);

		if (list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		list.append(keyNameString);
	}
}

static void InputExecAxisActionCommand(void* userData, short value)
{
	const InputBinding& binding = *reinterpret_cast<const InputBinding*>(userData);
	binding.boundAxisAction->func(value);
}

static void InputExecInputCommand(void* userData, short value)
{
	const InputCommand& cmd = *reinterpret_cast<const InputBinding*>(userData);

	Array<EqString> args(PP_SL);
	xstrsplit(cmd.argumentString, " ", args);

	const ConCommand* conCmd = (abs(value) > 16384) ? cmd.activateCmd : cmd.deactivateCmd;
	if (!conCmd)
		return;

	if (in_keys_debug.GetBool())
		MsgWarning("dispatch %s\n", conCmd->GetName());

	conCmd->DispatchFunc(args);
}

DECLARE_CMD_VARIANTS(bind, "Binds action to key", cmd_conKeyList, 0)
{
	if (CMD_ARGC > 1)
	{
		EqString agrstr;

		for (int i = 2; i < CMD_ARGC; i++)
			agrstr.Append(EqString::Format((i < CMD_ARGC - 1) ? "%s " : "%s", CMD_ARGV(i).ToCString()));

		g_inputCommandBinder->BindKey(CMD_ARGV(0), CMD_ARGV(1), agrstr);
	}
	else
		MsgInfo("Usage: bind <key> <command> [args,...]\n");
}

DECLARE_CMD(in_listBinding, "Shows bound keys", 0)
{
	MsgInfo("---- List of bound keys to commands ----\n");
	ArrayCRef<InputBinding*> bindingList = g_inputCommandBinder->GetBindingList();

	for (const InputBinding* binding : bindingList)
	{
		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		Msg("bind %s %s %s\n", 
			keyNameString.ToCString(), 
			binding->commandString.ToCString(), 
			binding->custom ? "(custom)" : binding->argumentString.ToCString());
	}

	MsgInfo("---- %d keys/commands bound ----\n", bindingList.numElem());
}

DECLARE_CMD(in_listTouchZones, "Shows bound keys", 0)
{
	MsgInfo("---- List of bound touchzones to commands ----\n");
	ArrayCRef<InputTouchZone> touchList = g_inputCommandBinder->GetTouchZoneList();

	for (const InputTouchZone& tz : touchList)
	{
		Msg("Touchzone %s (%s) [x=%g,y=%g] [w=%g,h=%g]\n",
			tz.name.ToCString(),
			tz.commandString.ToCString(),
			tz.position.x, tz.position.y,
			tz.size.x, tz.size.y);
	}

	MsgInfo("---- %d touch zones ----\n", touchList.numElem());
}

DECLARE_CMD(in_listAxisActions, "Shows axis list can be bound", 0)
{
	MsgInfo("---- List of axese ----\n");
	ArrayCRef<InputAxisAction> axisActs = g_inputCommandBinder->GetAxisActionList();

	for (const InputAxisAction& act : axisActs)
	{
		Msg("    %s\n", act.name.ToCString());
	}

	MsgInfo("---- %d axes ----\n", axisActs.numElem());
}


DECLARE_CMD_VARIANTS(unbind, "Unbinds a key", cmd_boundKeysList, 0)
{
	if (CMD_ARGC > 0)
	{
		g_inputCommandBinder->UnbindKey(CMD_ARGV(0));
	}
}

DECLARE_CMD(unbindall, "Unbinds all keys", 0)
{
	g_inputCommandBinder->UnbindAll();
}

DECLARE_CMD(unbindjoystick, "Unbinds joystick controls", 0)
{
	g_inputCommandBinder->UnbindAll_Joystick();
}

//---------------------------------------------------------
// UTILITY FUNCTIONS

static int InputBindingGetModifierCount(const InputBinding& binding)
{
	return (binding.modifierIds[1] != -1) ? 2 : ((binding.modifierIds[0] != -1) ? 1 : 0);
}

static bool InputGetBindingKeyIndices(int outKeys[3], const char* pszKeyStr)
{
	// parse pszKeyStr into modifiers
	const char* keyStr = pszKeyStr;

	// init
	outKeys[0] = -1;
	outKeys[1] = -1;
	outKeys[2] = -1;

	for (int i = 0; i < 2; i++)
	{
		const char* subStart = xstristr(keyStr, "+");
		if (!subStart)
			continue;

		EqString modifier(keyStr, subStart - keyStr);
		outKeys[i] = KeyStringToKeyIndex(modifier);
		if (outKeys[i] == -1)
		{
			MsgError("Unknown key/mapping '%s'\n", modifier.ToCString());
			return false;
		}

		keyStr = subStart + 1;

	}

	// Find the final key matching the *keychar
	outKeys[2] = KeyStringToKeyIndex(keyStr);

	if (outKeys[2] == -1)
	{
		MsgError("Unknown key/mapping '%s'\n", keyStr);
		return false;
	}

	return true;
}

static void ResolveInputCommand(InputCommand& cmd)
{
	// if we connecting libraries dynamically, that wouldn't properly execute
	cmd.activateCmd = g_consoleCommands->FindCommand(EqString::Format("+%s", cmd.commandString.ToCString()));
	cmd.deactivateCmd = g_consoleCommands->FindCommand(EqString::Format("-%s", cmd.commandString.ToCString()));

	// if found only one command with plus or minus
	if (!cmd.activateCmd || !cmd.deactivateCmd)
		cmd.activateCmd = g_consoleCommands->FindCommand(cmd.commandString);
}

void UTIL_GetBindingKeyString(EqString& outStr, const InputBinding* binding, bool humanReadable /*= false*/)
{
	if (!binding)
		return;

	outStr.Clear();

	const int validModifiers = InputBindingGetModifierCount(*binding);
	for (int i = 0; i < validModifiers; i++)
	{
		if(humanReadable)
			outStr.Append(s_keyMapList[binding->modifierIds[i]].hrname); // TODO: apply localizer
		else
			outStr.Append(s_keyMapList[binding->modifierIds[i]].name);
		outStr.Append(INPUT_CMD_ACTIVATE_PREFIX);
	}

	if (humanReadable)
		outStr.Append(s_keyMapList[binding->keyIdx].hrname); // TODO: apply localizer
	else
		outStr.Append(s_keyMapList[binding->keyIdx].name);
}

void UTIL_GetBindingKeyStringLocalized(EqWString& outStr, const InputBinding* binding, bool humanReadable /*= false*/)
{
	if (!binding)
		return;

	outStr.Clear();

	const int validModifiers = (binding->modifierIds[1] != -1) ? 2 : ((binding->modifierIds[0] != -1) ? 1 : 0);
	for (int i = 0; i < validModifiers; i++)
	{
		const char* token = humanReadable ? s_keyMapList[binding->modifierIds[i]].hrname : nullptr;
		if (!token)
			token = s_keyMapList[binding->modifierIds[i]].name;

		outStr.Append(LocalizedString(token));
		outStr.Append('+');
	}

	const char* token = humanReadable ? s_keyMapList[binding->keyIdx].hrname : nullptr;
	if (!token)
		token = s_keyMapList[binding->keyIdx].name;

	outStr.Append(LocalizedString(token));
}

//---------------------------------------------------------

CInputCommandBinder::CInputCommandBinder()
{
}

void CInputCommandBinder::Init()
{
	InitTouchZones();

#ifdef PLAT_SDL
	// init key names
	for (InputKeyMap* kn = s_keyMapList; kn->name; kn++)
	{
		if (!kn->hrname)
			kn->hrname = xstrdup(SDL_GetKeyName(SDL_SCANCODE_TO_KEYCODE(kn->keynum)));
	}
#endif // PLAT_SDL

	// resolve bindings for first time quietly
	for (InputBinding* binding : m_bindings)
		ResolveCommandBinding(*binding, true);

	m_init = true;
}

void CInputCommandBinder::Shutdown()
{
#ifdef PLAT_SDL
	// free key names
	for (InputKeyMap* kn = s_keyMapList; kn->name; kn++)
	{
		if (kn->hrname && *kn->hrname != '#')
		{
			SAFE_DELETE_ARRAY(kn->hrname);
		}
	}
#endif // PLAT_SDL

	for (InputBinding* binding : m_bindings)
		delete binding;

	m_bindings.clear(true);
	m_axisActs.clear(true);
	m_touchZones.clear(true);
	m_init = false;
	BitArrayImpl::clear(m_currentButtonBits, elementsOf(m_currentButtonBits));
}

void CInputCommandBinder::InitTouchZones()
{
	m_touchZones.clear(true);

	KeyValues kvs;
	if(!kvs.LoadFromFile("resources/in_touchzones.res"))
		return;

	KVSection* zones = kvs.GetRootSection()->FindSection("zones");
	if (!zones)
	{
		MsgError("touchzones file is invalid\n");
		return;
	}

	for(const KVSection* zoneDef : zones->keys)
	{
		InputTouchZone newZone;
		newZone.name = zoneDef->name;

		KVSection* zoneCmd = zoneDef->FindSection("bind");

		newZone.commandString = KV_GetValueString(zoneCmd, 0, "zone_no_bind");
		newZone.argumentString = KV_GetValueString(zoneCmd, 1, "");

		newZone.position = KV_GetVector2D(zoneDef->FindSection("position"));
		newZone.size = KV_GetVector2D(zoneDef->FindSection("size"));

		// resolve commands

		// if we connecting libraries dynamically, that wouldn't properly execute
		ResolveInputCommand(newZone);

		DevMsg(DEVMSG_CORE, "Touchzone: %s (%s) [x=%g,y=%g] [w=%g,h=%g]\n", 
			newZone.name.ToCString(), newZone.commandString.ToCString(), 
			newZone.position.x, newZone.position.y, 
			newZone.size.x, newZone.size.y);

		// if anly command found
		if(newZone.activateCmd || newZone.deactivateCmd)
		{
			m_touchZones.append( newZone );
		}
		else
		{
			MsgError("touchzone %s: unknown command '%s'\n", newZone.name.ToCString(), newZone.commandString.ToCString());
		}
	}
}

// saves binding using file handle
void CInputCommandBinder::WriteBindings(IVirtualStream* stream)
{
	if(!stream)
		return;

	stream->Print("unbindall\n" );

	for(const InputBinding* binding : m_bindings)
	{
		if (binding->custom)
			continue;

		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		stream->Print("bind %s %s %s\n",keyNameString.GetData(),
										binding->commandString.GetData(),
										binding->argumentString.GetData() );
	}
}

bool CInputCommandBinder::BindKey( const char* pszKeyStr, const char* pszCommand, const char* pszArgs )
{
	InputBinding* binding = AddBinding(pszKeyStr, pszCommand, pszArgs);

	if (!binding)
		return false;

	if(m_init && !ResolveCommandBinding(*binding, false))
	{
		DeleteBinding( binding );
		return false;
	}

	return true;
}

// binds a command with arguments to known key
InputBinding* CInputCommandBinder::AddBinding( const char* pszKeyStr, const char* pszCommand, const char *pszArgs )
{
	InputBinding* binding = nullptr;
	while (binding = FindBindingByCommandName(pszCommand, pszArgs, binding))
	{
		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		if (!keyNameString.CompareCaseIns(pszKeyStr))
		{
			if(strlen(pszArgs))
				MsgWarning("Command '%s %s' already bound to '%s'\n", pszCommand, pszArgs, pszKeyStr);
			else
				MsgWarning("Command '%s' already bound to '%s'\n", pszCommand, pszKeyStr);

			return nullptr;
		}
	}

	int bindingKeyIndices[3];
	if (!InputGetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return nullptr;

	// create new binding
	InputBinding* newBind = PPNew InputBinding;
	newBind->modifierIds[0] = bindingKeyIndices[0];
	newBind->modifierIds[1] = bindingKeyIndices[1];
	newBind->keyIdx = bindingKeyIndices[2];
	newBind->commandString = pszCommand;

	if(pszArgs)
		newBind->argumentString = pszArgs;

	m_bindings.append(newBind);

	return newBind;
}

InputBinding* CInputCommandBinder::AddBinding(const char* pszKeyStr, const char* name, InputCommand::Func func, void* userData /*= nullptr*/)
{
	InputBinding* binding = nullptr;
	while (binding = FindBindingByCommandName(name, nullptr, binding))
	{
		if (!binding->custom)
			continue;

		EqString keyNameString;
		UTIL_GetBindingKeyString(keyNameString, binding);

		if (!keyNameString.CompareCaseIns(pszKeyStr))
		{
			MsgWarning("Command '%s' already bound to '%s'\n", name, pszKeyStr);
			return nullptr;
		}
	}

	int bindingKeyIndices[3];
	if (!InputGetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return nullptr;

	// create new binding
	InputBinding* newBind = PPNew InputBinding;
	newBind->modifierIds[0] = bindingKeyIndices[0];
	newBind->modifierIds[1] = bindingKeyIndices[1];
	newBind->keyIdx = bindingKeyIndices[2];
	newBind->commandString = name;
	newBind->func = func;
	newBind->userData = userData;
	newBind->custom = true;

	m_bindings.append(newBind);

	return newBind;
}

bool CInputCommandBinder::ResolveCommandBinding(InputBinding& binding, bool quiet)
{
	if (binding.func)
		return true;

	// try resolve axis first
	if(s_keyMapList[binding.keyIdx].keynum >= JOYSTICK_START_AXES)
	{
		binding.boundAxisAction = FindAxisAction(binding.commandString);
		if(binding.boundAxisAction)
			binding.func = InputExecAxisActionCommand;
	}

	// if no axis action is bound, try bind concommands
	if(!binding.func)
	{
		ResolveInputCommand(binding);

		if(binding.activateCmd || binding.deactivateCmd)
			binding.func = InputExecInputCommand;
	}
	
	if(!quiet && !binding.func)
	{
		MsgError("Cannot bind command '%s' to '%s'\n", binding.commandString.ToCString(), s_keyMapList[binding.keyIdx].name);
		return false;
	}

	return true;
}

InputAxisAction* CInputCommandBinder::FindAxisAction(const char* name) const
{
	for(int i = 0; i < m_axisActs.numElem();i++)
	{
		if(!m_axisActs[i].name.CompareCaseIns(name))
			return const_cast<InputAxisAction*>(&m_axisActs[i]);
	}

	return nullptr;
}

// searches for binding
InputBinding* CInputCommandBinder::FindBinding(const char* pszKeyStr, const char* cmdPrefixStr) const
{
	int bindingKeyIndices[3];
	if (!InputGetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return nullptr;

	for(InputBinding* binding : m_bindings)
	{
		const char* cmdNameStr = binding->commandString;
		if (*cmdNameStr == INPUT_CMD_ACTIVATE_PREFIX || *cmdNameStr == INPUT_CMD_DEACTIVATE_PREFIX)
			++cmdNameStr;

		const char* catSubChr = strchr(cmdNameStr, '.');
		if (catSubChr)
		{
			const int catLen = catSubChr - cmdNameStr;
			if (strncmp(cmdNameStr, cmdPrefixStr, catLen))
				continue;
		}

		if (binding->modifierIds[0] == bindingKeyIndices[0] &&
			binding->modifierIds[1] == bindingKeyIndices[1] &&
			binding->keyIdx == bindingKeyIndices[2])
		{
			return binding;
		}
	}

	return nullptr;
}

InputBinding* CInputCommandBinder::FindBindingByCommand(ConCommandBase* cmdBase, const char* argStr /*= nullptr*/, const InputBinding* startFrom /*= nullptr*/) const
{
	const int startIdx = arrayFindIndex(m_bindings, startFrom) + 1;
	for (int i = startIdx; i < m_bindings.numElem(); i++)
	{
		InputBinding& binding = const_cast<InputBinding&>(*m_bindings[i]);
		if ((binding.activateCmd == cmdBase || binding.deactivateCmd == cmdBase) && 
			(!argStr || argStr && !binding.argumentString.CompareCaseIns(argStr)))
		{
			return &binding;
		}
	}

	return nullptr;
}

InputBinding* CInputCommandBinder::FindBindingByCommandName(const char* name, const char* argStr /*= nullptr*/, const InputBinding* startFrom /*= nullptr*/) const
{
	const int startIdx = arrayFindIndex(m_bindings, startFrom) + 1;
	for (int i = startIdx; i < m_bindings.numElem(); i++)
	{
		InputBinding& binding = const_cast<InputBinding&>(*m_bindings[i]);

		if(!binding.commandString.CompareCaseIns(name) &&
			(!argStr || argStr && !binding.argumentString.CompareCaseIns(argStr)))
		{
			return &binding;
		}
	}

	return nullptr;
}

void CInputCommandBinder::DeleteBinding( InputBinding* binding )
{
	if(binding == nullptr)
		return;

	if(m_bindings.fastRemove(binding))
		delete binding;
}

// removes single binding on specified keychar
void CInputCommandBinder::UnbindKey(const char* pszKeyStr)
{
	int bindingKeyIndices[3];
	if (!InputGetBindingKeyIndices(bindingKeyIndices, pszKeyStr))
		return;

	int results = 0;
	for(int i = 0; i < m_bindings.numElem();i++)
	{
		InputBinding* binding = m_bindings[i];
		if (binding->custom)
			continue;

		if (binding->modifierIds[0] == bindingKeyIndices[0] &&
			binding->modifierIds[1] == bindingKeyIndices[1] &&
			binding->keyIdx == bindingKeyIndices[2])
		{
			m_bindings.fastRemoveIndex(i--);
			delete binding;

			results++;
		}
	}

	if(results)
		MsgInfo("'%s' unbound (%d matches)\n", pszKeyStr, results);
}

void CInputCommandBinder::UnbindCommandByName(const char* name, const char* argStr /*= nullptr*/)
{
	InputBinding* binding = nullptr;
	while (binding = FindBindingByCommandName(name, argStr, binding))
	{
		if (binding->custom)
			continue;

		DeleteBinding(binding);
	}
}

// clears and removes all key bindings
void CInputCommandBinder::UnbindAll()
{
	for (int i = 0; i < m_bindings.numElem(); i++)
	{
		InputBinding* binding = m_bindings[i];
		if (binding->custom)
			continue;
		delete binding;
		m_bindings.fastRemoveIndex(i--);
	}

	m_axisActs.clear(true);
	BitArrayImpl::clear(m_currentButtonBits, elementsOf(m_currentButtonBits));
}

void CInputCommandBinder::UnbindAll_Joystick()
{
	for(int i = 0; i < m_bindings.numElem();i++)
	{
		InputBinding* binding = m_bindings[i];
		const int keyNum = s_keyMapList[binding->keyIdx].keynum;
		if(keyNum >= JOYSTICK_START_KEYS && keyNum < MOU_B1)
		{
			m_bindings.fastRemoveIndex(i--);
			delete binding;
		}
	}
}

// registers axis action
void CInputCommandBinder::RegisterJoyAxisAction( const char* name, InputAxisAction::Func axisFunc )
{
	InputAxisAction act;
	act.name = "ax_" + _Es(name);
	act.func = axisFunc;

	m_axisActs.append( act );
}

bool CInputCommandBinder::CheckModifiersAndDepress(InputBinding& binding, int currentKeyIdent, bool currentPressed)
{
	const int validModifiers = InputBindingGetModifierCount(binding);
	if (!validModifiers)
		return true;

	int numModifiers = 0;
	for (int i = 0; i < validModifiers; i++)
	{
		const int keyIdent = s_keyMapList[binding.modifierIds[numModifiers]].keynum;

		// depress 
		if (!currentPressed && currentKeyIdent == keyIdent)
			ExecuteBinding(binding, 0);

		const bool modifierIsPressed = BitArrayImpl::isTrue(m_currentButtonBits, elementsOf(m_currentButtonBits), keyIdent);
		if (modifierIsPressed)
			++numModifiers;

		if (numModifiers >= validModifiers)
			break;
	}

	return (numModifiers == validModifiers);
}

//
// Event processing
//
void CInputCommandBinder::OnKeyEvent(int keyIdent, bool pressed)
{
	if(in_keys_debug.GetBool())
		MsgWarning("-- KeyPress: %s (%d)\n", KeyIndexToString(keyIdent), pressed);

	BitArrayImpl::set(m_currentButtonBits, elementsOf(m_currentButtonBits), keyIdent, pressed);

	Array<InputBinding*> complexExecuteList(PP_SL);
	Array<InputBinding*> executeList(PP_SL);

	const short pressure = pressed ? SHRT_MAX : 0;

	for(InputBinding* binding : m_bindings)
	{
		// here we also depress modifiers if has any
		if (!CheckModifiersAndDepress(*binding, keyIdent, pressed))
			continue;

		// check on the keymap
		if (s_keyMapList[binding->keyIdx].keynum == keyIdent)
		{
			const int validModifiers = InputBindingGetModifierCount(*binding);
			if (validModifiers)
				complexExecuteList.append(binding);
			else
				executeList.append(binding);
		}
	}

	// complex actions are in favor
	if (complexExecuteList.numElem())
	{
		for (InputBinding* binding: complexExecuteList)
			ExecuteBinding(*binding, pressure);

		return;
	}

	for (InputBinding* binding : executeList)
		ExecuteBinding(*binding, pressure);
}

void CInputCommandBinder::OnMouseEvent( int button, bool pressed )
{
	const short pressure = pressed ? SHRT_MAX : 0;

	for(InputBinding* binding : m_bindings)
	{
		if (!CheckModifiersAndDepress(*binding, button, pressed))
			continue;

		if(s_keyMapList[binding->keyIdx].keynum == button)
			ExecuteBinding(*binding, pressure);
	}
}

void CInputCommandBinder::OnMouseWheel( int scroll )
{
	const short pressure = (scroll > 0) ? SHRT_MAX : 0;

	const int button = (scroll > 0) ?  MOU_WHUP : MOU_WHDN;
	for(InputBinding* binding : m_bindings)
	{
		if (!CheckModifiersAndDepress(*binding, 0, true))
			continue;

		if(s_keyMapList[binding->keyIdx].keynum == button)
			ExecuteBinding(*binding, pressure);
	}
}

void CInputCommandBinder::OnTouchEvent( const Vector2D& pos, int finger, bool down )
{
	if(in_touchzones_debug.GetInt() == 2)
		MsgWarning("-- Touch [%g %g] (%d)\n", pos.x, pos.y, down);

	for(InputTouchZone& tz : m_touchZones)
	{
		const AARectangle rect(tz.position - tz.size*0.5f, tz.position + tz.size*0.5f);

		if(!down)
		{
			if(tz.finger == finger) // if finger up
			{
				ExecuteTouchZone( tz, 0);
				tz.finger = -1;
			}
		}
		else if( rect.Contains(pos) )
		{
			if (in_touchzones_debug.GetInt() == 2)
				Msg("found zone %s\n", tz.name.ToCString());

			tz.finger = finger;
			ExecuteTouchZone( tz, SHRT_MAX); // TODO: variable touchzone values (stick support)
		}
	}
}

void CInputCommandBinder::OnJoyAxisEvent( short axis, short value )
{
	const int joyAxisCode = axis + JOYSTICK_START_AXES;

	for (InputBinding* binding : m_bindings)
	{
		// run through all axes
		if (binding->modifierIds[0] == -1 &&
			binding->modifierIds[1] == -1 &&
			s_keyMapList[binding->keyIdx].keynum == joyAxisCode)
		{
			ExecuteBinding(*binding, value);
		}
	}
}

void CInputCommandBinder::DebugDraw(const Vector2D& screenSize, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!in_touchzones_debug.GetBool())
		return;

	g_matSystem->Setup2D(screenSize.x,screenSize.y);

	eqFontStyleParam_t fontParams;
	fontParams.styleFlag |= TEXT_STYLE_SHADOW;
	fontParams.textColor = color_white;

	Array<AARectangle> rects(PP_SL);
	rects.resize(m_touchZones.numElem());

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	RenderPassContext defaultPassContext(rendPassRecorder, &defaultRenderPass);

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);

	for (int i = 0; i < m_touchZones.numElem(); i++)
	{
		const InputTouchZone* tz = &m_touchZones[i];
		AARectangle rect((tz->position - tz->size * 0.5f) * screenSize, (tz->position + tz->size * 0.5f) * screenSize);

		rects.append(rect);
		const Vertex2D touchQuad[] = { MAKETEXQUAD(rect.leftTop.x, rect.leftTop.y,rect.rightBottom.x, rect.rightBottom.y, 0) };

		const float touchColor = tz->finger >= 0 ? 0.25f : 0.85f;

		meshBuilder.Color4f(touchColor, touchColor, touchColor, 0.25f);
		meshBuilder.TexturedQuad2(
			touchQuad[0].position, touchQuad[1].position, touchQuad[2].position, touchQuad[3].position,
			touchQuad[0].texCoord, touchQuad[1].texCoord, touchQuad[2].texCoord, touchQuad[3].texCoord);
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, defaultPassContext);

	static IEqFont* defaultFont = g_fontCache->GetFont("default", 30);
	for (int i = 0; i < m_touchZones.numElem(); i++)
	{
		const InputTouchZone& tz = m_touchZones[i];
		const AARectangle& rect = rects[i];

		defaultFont->SetupRenderText(tz.name, rect.leftTop, fontParams, rendPassRecorder);
	}
}

// executes binding with selected state
void CInputCommandBinder::ExecuteBinding(InputBinding& binding, short value)
{
	ResolveCommandBinding(binding, false);

	if (!binding.func)
		return;

	binding.func(binding.custom ? binding.userData : &binding, value);
}

void CInputCommandBinder::ExecuteTouchZone(InputTouchZone& zone, short value)
{
	if (!zone.func)
		return;

	zone.func(zone.custom ? zone.userData : &zone, value);
}
