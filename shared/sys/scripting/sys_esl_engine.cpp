//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "sys_esl.h"

#include "render/IDebugOverlay.h"
#include "audio/eqSoundEmitterSystem.h"
#include "audio/eqSoundEmitterObject.h"
#include "input/InputCommandBinder.h"
#include "sys/sys_state.h"
#include "sys/sys_host.h"
#include "sys/sys_in_joystick.h"

#include "sys/sys_in_console.h"
#include "sys/sys_version.h"

#ifdef IMGUI_ENABLED
#include "imgui_backend/imgui_host.h"
#include "imgui_lua_bindings.h"
#include "audio/SoundScriptEditorUI.h"
#endif

#include "sys_esl_engine.h"

//---------------------------------------------------------------------------------------
// Debug overlay
//---------------------------------------------------------------------------------------

static void S_IDebugOverlay_Text(IDebugOverlay* _self, const ColorRGBA& color, char const* text)
{
	_self->Text(color, text);
}

static void S_IDebugOverlay_TextFadeOut(IDebugOverlay* _self, int position, const ColorRGBA& color, float fFadeTime, char const* text)
{
	_self->TextFadeOut(position, color, fFadeTime, text);
}

static void S_IDebugOverlay_Text3D(IDebugOverlay* _self, const Vector3D& origin, float distance, const ColorRGBA& color, float fTime, char const* text)
{
	_self->Text3D(origin, distance, color, text, fTime);
}

static EqString GetTextStrArgs(lua_State* L, int startArg = 0)
{
	const int n = lua_gettop(L);  /* number of arguments */

	EqString out;
	for (int i = 1 + startArg; i <= n; i++)
	{
		size_t l;
		const char* s = luaL_tolstring(L, i, &l);
		if (i > 1) out.Append(' ');
		out.Append(s);
		lua_pop(L, 1);
	}
	return out;
}

static DDText3D& S_DDText3D_Text(const esl::ScriptState& state, DDText3D& builder)
{
	return builder.Text(GetTextStrArgs(state, 1));
}

template<typename T>
static T& S_DDNode_Name(const esl::ScriptState& state, T& builder)
{
	return static_cast<T&>(builder.Name(GetTextStrArgs(state, 1)));
}

template<typename T>
static T& S_DDNode_Time(const esl::ScriptState& state, T& builder, float time)
{
	return static_cast<T&>(builder.Time(time));
}

EQSCRIPT_TYPE_BEGIN( IDebugOverlay )
	EQSCRIPT_BIND_STATIC_FUNC("Text", S_IDebugOverlay_Text)
	EQSCRIPT_BIND_STATIC_FUNC("TextFadeOut", S_IDebugOverlay_TextFadeOut)
	EQSCRIPT_BIND_STATIC_FUNC("Text3D", S_IDebugOverlay_Text3D)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(DDText3D)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_STATIC_FUNC("Text", S_DDText3D_Text)
	EQSCRIPT_BIND_FUNC(Position)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_FUNC(Distance)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDText3D>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDText3D>)
EQSCRIPT_TYPE_END

static DDText3D* S_DbgText3D() { return PPNew DDText3D(PP_SL); }

EQSCRIPT_TYPE_BEGIN(DDBox)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_FUNC(Box)
	EQSCRIPT_BIND_FUNC(CenterSize)
	EQSCRIPT_BIND_FUNC(Mins)
	EQSCRIPT_BIND_FUNC(Maxs)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_FUNC(Fill)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDBox>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDBox>)
EQSCRIPT_TYPE_END

static DDBox* S_DbgBox() { return PPNew DDBox(PP_SL); }

EQSCRIPT_TYPE_BEGIN(DDOrientedBox)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_FUNC(Mins)
	EQSCRIPT_BIND_FUNC(Maxs)
	EQSCRIPT_BIND_FUNC(Position)
	EQSCRIPT_BIND_FUNC(Rotation)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_FUNC(Fill)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDOrientedBox>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDOrientedBox>)
EQSCRIPT_TYPE_END

static DDOrientedBox* S_DbgOriBox() { return PPNew DDOrientedBox(PP_SL); }

EQSCRIPT_TYPE_BEGIN(DDSphere)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_FUNC(Position)
	EQSCRIPT_BIND_FUNC(Radius)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_FUNC(Fill)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDSphere>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDSphere>)
EQSCRIPT_TYPE_END

static DDSphere* S_DbgSphere() { return PPNew DDSphere(PP_SL); }

EQSCRIPT_TYPE_BEGIN(DDCylinder)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_FUNC(Position)
	EQSCRIPT_BIND_FUNC(Radius)
	EQSCRIPT_BIND_FUNC(Height)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDCylinder>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDCylinder>)
EQSCRIPT_TYPE_END

static DDCylinder* S_DbgCylinder() { return PPNew DDCylinder(PP_SL); }

EQSCRIPT_TYPE_BEGIN(DDLine)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_FUNC(Start)
	EQSCRIPT_BIND_FUNC(End)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_FUNC(ColorStart)
	EQSCRIPT_BIND_FUNC(ColorEnd)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDLine>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDLine>)
EQSCRIPT_TYPE_END

static DDLine* S_DbgLine() { return PPNew DDLine(PP_SL); }

EQSCRIPT_TYPE_BEGIN(DDPoly)
	EQSCRIPT_BIND_FUNC(Dispatch)

	EQSCRIPT_BIND_FUNC(Point)
	EQSCRIPT_BIND_FUNC(Color)
	EQSCRIPT_BIND_FUNC(Fill)
	EQSCRIPT_BIND_FUNC(Outline)
	EQSCRIPT_BIND_STATIC_FUNC("Time", S_DDNode_Name<DDPoly>)
	EQSCRIPT_BIND_STATIC_FUNC("Name", S_DDNode_Time<DDPoly>)
EQSCRIPT_TYPE_END

static DDPoly* S_DbgPoly() { return PPNew DDPoly(PP_SL); }

//---------------------------------------------------------------------------------------
// Sound Emitter System
//---------------------------------------------------------------------------------------

//
// Emitsound params
//

static void S_EmitParams_SetInputValue(EmitParams& ep, const char* name, float value)
{
	const int nameHash = StringId24(name);
	const int valIdx = arrayFindIndexF(ep.inputs, [nameHash](const EmitParams::InputValue& in) {
		return in.nameHash == nameHash;
	});
	
	if (valIdx == -1)
	{
		ep.inputs.appendEmplace(nameHash, value);
		return;
	}
	ep.inputs[valIdx].value = value;
}

EQSCRIPT_TYPE_BEGIN(EmitParams)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*, int)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*, float, float)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*, const Vector3D&)
	EQSCRIPT_BIND_CONSTRUCTOR(const char*, const Vector3D&, float, float)

	EQSCRIPT_BIND_STATIC_FUNC("SetInputValue", S_EmitParams_SetInputValue)

	EQSCRIPT_BIND_VAR(sampleId)
	EQSCRIPT_BIND_VAR(flags)
EQSCRIPT_TYPE_END

//
// Sound emitter system
//
ESL_ENUM(IEqAudioSource::State);

EQSCRIPT_TYPE_BEGIN(CSoundEmitterSystem)
	EQSCRIPT_BIND_FUNC_NAMED("Precache", PrecacheSound)
	EQSCRIPT_BIND_FUNC_NAMED("Emit", EmitSound)
	EQSCRIPT_BIND_FUNC_NAMED("LoadScript", LoadScriptBank)
	EQSCRIPT_BIND_FUNC_NAMED("FreeScript", FreeScriptBank)
EQSCRIPT_TYPE_END


EQSCRIPT_TYPE_BEGIN(CSoundingObject)
	EQSCRIPT_BIND_CONSTRUCTOR()

	EQSCRIPT_BIND_FUNC_NAMED("Emit", EmitSound)

	EQSCRIPT_BIND_FUNC(GetEmitterState)
	EQSCRIPT_BIND_FUNC_OVERLOAD(SetEmitterState, void, (int, IEqAudioSource::State, bool))

	EQSCRIPT_BIND_FUNC(HasEmitter)
	EQSCRIPT_BIND_FUNC(GetEmitterSampleId)
	EQSCRIPT_BIND_FUNC(SetEmitterSampleId)

	EQSCRIPT_BIND_FUNC_OVERLOAD(StopEmitter, void, (int, bool))
	EQSCRIPT_BIND_FUNC_OVERLOAD(PlayEmitter, void, (int, bool))
	EQSCRIPT_BIND_FUNC_OVERLOAD(PauseEmitter, void, (int))
	EQSCRIPT_BIND_FUNC_OVERLOAD(StopLoop, void, (int, float))

	EQSCRIPT_BIND_FUNC_OVERLOAD(SetPitch, void, (int, float))
	EQSCRIPT_BIND_FUNC_OVERLOAD(SetVolume, void, (int, float))

	EQSCRIPT_BIND_FUNC_OVERLOAD(SetPosition, void, (int, const Vector3D&))
	EQSCRIPT_BIND_FUNC_OVERLOAD(SetVelocity, void, (int, const Vector3D&))
	EQSCRIPT_BIND_FUNC_OVERLOAD(SetConeProperties, void, (int, const Vector3D&, float, float, float, float))

	EQSCRIPT_BIND_FUNC_OVERLOAD(SetSamplePlaybackPosition, void, (int, int, float))
	EQSCRIPT_BIND_FUNC_OVERLOAD(SetSampleVolume, void, (int, int, float))

	EQSCRIPT_BIND_FUNC_OVERLOAD(SetInputValue, void, (int, const char*, float))
	EQSCRIPT_BIND_FUNC(GetChannelSoundCount)

	EQSCRIPT_BIND_FUNC(SetSoundVolumeScale)
	EQSCRIPT_BIND_FUNC(GetSoundVolumeScale)
EQSCRIPT_TYPE_END

static esl::LuaTable S_CSoundEmitterSystem_GetAllSoundsList(const esl::ScriptState& state)
{
	esl::LuaTable retTable = state.CreateTable();

	Array<SoundScriptDesc*> sndList(PP_SL);
	g_sounds->GetAllSoundsList(sndList);

	for (int i = 0; i < sndList.numElem(); ++i)
		retTable.Set(i+1, CSoundEmitterSystem::GetScriptName(sndList[i]));

	return retTable;
}

//---------------------------------------------------------------------------------------
// System Host
//---------------------------------------------------------------------------------------

static double S_Host_GetFrameTime()
{
	return g_pHost->GetFrameTime();
}

static void S_Host_RequestTextInput()
{
	g_pHost->RequestTextInput();
}

static void S_Host_ReleaseTextInput()
{
	g_pHost->ReleaseTextInput();
}

static bool S_Host_IsTextInputShown()
{
	return g_pHost->IsTextInputShown();
}

static esl::LuaTable S_Host_GetVideoModes(const esl::ScriptState& state)
{
	Array<SysVideoMode> allModes(PP_SL);
	g_pHost->GetVideoModes(allModes);

	esl::LuaTable luaVideoModeTable = state.CreateTable();

	for (int i = 0; i < allModes.numElem(); i++)
	{
		const SysVideoMode& vm = allModes[i];
		esl::LuaTable vmTable = state.CreateTable();
		vmTable.Set("displayId", vm.displayId);
		vmTable.Set("width", vm.width);
		vmTable.Set("height", vm.height);
		vmTable.Set("bitsPerPixel", vm.bitsPerPixel);
		vmTable.Set("refreshRate", vm.refreshRate);

		luaVideoModeTable.Set(i + 1, vmTable);
	}

	return luaVideoModeTable;
}

static IVector2D S_Host_GetWindowSize()
{
	return g_pHost->GetWindowSize();
}

static void S_Host_ApplyVideoMode()
{
	g_pHost->ApplyVideoMode();
}

//---------------------------------------------------------------------------------------
// Input
//---------------------------------------------------------------------------------------

struct LuaInputBinding
{
	~LuaInputBinding()
	{
		g_inputCommandBinder->DeleteBinding(inputCmd);
		SAFE_DELETE_ARRAY(name);
	}

	esl::LuaFunctionRef funcRef;
	InputBinding*		inputCmd{ nullptr };
	const char*			name{ nullptr };


	static Map<int, LuaInputBinding> s_bindings;
	static void ClearAll()
	{
		s_bindings.clear(true);
	}

	static void CommandHandler(void* userData, const Vector3D& value)
	{
		const int nameHash = reinterpret_cast<int>(userData);
		auto it = s_bindings.find(nameHash);
		ASSERT_MSG(!it.atEnd(), "LuaInputBinding CommandHandler ref is invalid");

		const LuaInputBinding& ref = it.value();

		using CommandFunc = esl::runtime::FunctionCall<void, float>;
		auto callResult = CommandFunc::Invoke(ref.funcRef, value.x);
		LUA_CHECK_CALL(callResult, ref.name);
	}
};
Map<int, LuaInputBinding> LuaInputBinding::s_bindings{ PP_SL };

static bool L_Input_AddBinding(char const* name, char const* keyStr, esl::LuaFunctionRef cmdFunc)
{
	// register con. command function reference
	ASSERT_MSG(cmdFunc.IsValid() == true, "Not valid function for Lua InputBinding %s", name);

	const int nameHash = StringId24(name);
	InputBinding* binding = g_inputCommandBinder->AddBinding(keyStr, name, LuaInputBinding::CommandHandler, reinterpret_cast<void*>(nameHash));

	if (!binding)
		return false;

	LuaInputBinding& ref = *LuaInputBinding::s_bindings.insert(nameHash);
	ref.name = CString::DuplicateNew(name);
	ref.funcRef = cmdFunc;
	ref.inputCmd = binding;
	return true;
}

static void L_Input_RemoveBinding(const char* name)
{
	const int nameHash = StringId24(name);
	LuaInputBinding::s_bindings.remove(nameHash);
}

//---------------------------------------------------

struct LuaInputVectorAction
{
	~LuaInputVectorAction()
	{
		g_inputCommandBinder->RemoveVectorAction(name);
		SAFE_DELETE_ARRAY(name);
	}

	esl::LuaFunctionRef funcRef;
	const char*			name{ nullptr };

	static Map<int, LuaInputVectorAction> s_actions;
	static void ClearAll()
	{
		s_actions.clear(true);
	}

	static void CommandHandler(void* userData, const Vector3D& value)
	{
		const int nameHash = reinterpret_cast<int>(userData);
		auto it = s_actions.find(nameHash);
		ASSERT_MSG(!it.atEnd(), "LuaInputAxisAction CommandHandler ref is invalid");

		const LuaInputVectorAction& ref = it.value();

		using AxisFunc = esl::runtime::FunctionCall<void, Vector3D>;
		auto callResult = AxisFunc::Invoke(ref.funcRef, value);
		LUA_CHECK_CALL(callResult, ref.name);
	}
};
Map<int, LuaInputVectorAction> LuaInputVectorAction::s_actions{ PP_SL };

static int L_Input_CreateVectorAction(char const* name, esl::LuaFunctionRef cmdFunc, int axisCount)
{
	// register con. command function reference
	ASSERT_MSG(cmdFunc.IsValid() == true, "Not valid function for Lua AxisAction %s", name);

	const int nameHash = StringId24(name);
	g_inputCommandBinder->CreateVectorAction( name, LuaInputVectorAction::CommandHandler, axisCount, reinterpret_cast<void*>(nameHash));

	LuaInputVectorAction& ref = *LuaInputVectorAction::s_actions.insert(nameHash);
	ref.name = CString::DuplicateNew(name);
	ref.funcRef = cmdFunc;

	return nameHash;
}

static void L_Input_RemoveVectorAction(int id)
{
	LuaInputVectorAction::s_actions.remove(id);
}

//---------------------------------------------------

static esl::LuaTable L_Input_GetControllers(const esl::ScriptState& state)
{
	esl::LuaTable retTable = state.CreateTable();
	GameControllerList controllers = CEqGameControllerSDL::GetControllers();
	for (CEqGameControllerSDL* ctrl : controllers)
	{
		retTable.Set(CEqGameControllerSDL::GetControllerIndex(ctrl), ctrl->GetName());
	}
	return retTable;
}

static esl::LuaTable L_Input_GetCommandBindings(const esl::ScriptState& state, const char* commandName)
{
	esl::LuaTable bindingsTable = state.CreateTable();

	InputBinding* binding = nullptr;
	int bindIdx = 0;
	while (binding = g_inputCommandBinder->FindBindingByCommandName(commandName, nullptr, binding))
	{
		esl::LuaTable table = state.CreateTable();
		bindingsTable.Set(++bindIdx, table);

		esl::LuaTable modTable = state.CreateTable();
		int keyIds[3];
		int keyIdCount = UTIL_GetBindingKeyIds(keyIds, binding);
		for (int i = 0; i < keyIdCount; ++i)
			modTable.Set(i+1, keyIds[i]);

		table.Set("keyIds", modTable);
		table.Set("command", binding->commandString);
		table.Set("argument", binding->argumentString);
	}

	return bindingsTable;
}

static void L_Input_UnbindCommand(const esl::ScriptState& state, const char* commandName)
{
	g_inputCommandBinder->UnbindCommandByName(commandName);
}

static void L_Input_UnregisterCommand(const esl::ScriptState& state, ConCommandBase* cmdBase)
{
	g_inputCommandBinder->UnregisterCommand(cmdBase);
}

static void L_Input_BindAction(const esl::ScriptState& state, const char* keysStr, const char* actionName, const char* args)
{
	const EqStringRef inputActionName = actionName;

	EqString actionCategoryName;
	const int catIdx = inputActionName.Find(".");
	if (catIdx != -1)
		actionCategoryName = inputActionName.Left(catIdx);

	InputBinding* existingBinding;
	while (existingBinding = g_inputCommandBinder->FindBinding(keysStr, actionCategoryName))
		g_inputCommandBinder->DeleteBinding(existingBinding);

	g_inputCommandBinder->AddBinding(keysStr, actionName, args);
}

static int L_Input_GetLastInputDeviceUsed()
{
	return g_inputCommandBinder->GetLastInputDeviceUsed();
}

//---------------------------------------------------------------------------------------

static Map<int, EqString> s_luaImguiHandlerNames(PP_SL);

static void EqImGui_AddHandler(const char* name, const esl::LuaFunctionRef& func)
{
#ifdef IMGUI_ENABLED
	s_luaImguiHandlerNames.insert(StringId24(name), name);
	g_imGuiHost->AddDebugHandler(name, [name = EqString(name), funcRef = func](bool& _) {
		using ImGuiFunc = esl::runtime::FunctionCall<void>;
		auto result = ImGuiFunc::Invoke(funcRef);
		LUA_CHECK_CALL(result, name.ToCString());
	});
#endif
}

static void EqImGui_RemoveHandler(const char* name)
{
#ifdef IMGUI_ENABLED
	g_imGuiHost->RemoveDebugHandler(name);
	s_luaImguiHandlerNames.remove(StringId24(name));
#endif
}

static void EqImGui_AddDebugMenu(const char* path, const esl::LuaFunctionRef& func)
{
#ifdef IMGUI_ENABLED
	s_luaImguiHandlerNames.insert(StringId24(path), path);
	g_imGuiHost->AddDebugMenu(path, [name = EqString(path), funcRef = func](bool& visible) {
		using ImGuiFunc = esl::runtime::FunctionCall<bool, bool>;
		auto result = ImGuiFunc::Invoke(funcRef, visible);
		if (LUA_CHECK_CALL(result, name.ToCString()))
			visible = *result;
	});
#endif
}

static void EqImGui_ShowDebugMenu(const char* path, bool enable)
{
#ifdef IMGUI_ENABLED
	g_imGuiHost->ShowDebugMenu(path, enable);
#endif
}

static void EqImGui_ToggleDebugMenu(const char* path)
{
#ifdef IMGUI_ENABLED
	g_imGuiHost->ToggleDebugMenu(path);
#endif
}

// allows to execute Lua code inside in-game console
static bool EqLuaConsoleHandler(const char* cmdText)
{
	EqString cmdFullText(cmdText);

	if (*cmdText == '=')
		cmdFullText = EqString::Format("ConsolePrint(%s)", cmdText + 1);

	const esl::ScriptState state = eslSys::GetScriptState();
	if (!state.RunChunk(cmdFullText, "console"))
	{
		MsgError("%s\n", esl::runtime::GetLastError(state));
	}

	return true;
}

bool eslSysInputInit(const esl::ScriptState& state)
{
	esl::LuaTable inputTable = state.CreateTable();
	inputTable.Set("AddBinding", EQSCRIPT_CFUNC(L_Input_AddBinding));
	inputTable.Set("RemoveBinding", EQSCRIPT_CFUNC(L_Input_RemoveBinding));

	inputTable.Set("CreateVectorAction", EQSCRIPT_CFUNC(L_Input_CreateVectorAction));
	inputTable.Set("RemoveVectorAction", EQSCRIPT_CFUNC(L_Input_RemoveVectorAction));

	inputTable.Set("GetControllers", EQSCRIPT_CFUNC(L_Input_GetControllers));

	inputTable.Set("GetCommandBindings", EQSCRIPT_CFUNC(L_Input_GetCommandBindings));
	inputTable.Set("UnbindCommand", EQSCRIPT_CFUNC(L_Input_UnbindCommand));
	inputTable.Set("UnregisterCommand", EQSCRIPT_CFUNC(L_Input_UnregisterCommand));
	inputTable.Set("BindAction", EQSCRIPT_CFUNC(L_Input_BindAction));
	inputTable.Set("GetLastInputDeviceUsed", EQSCRIPT_CFUNC(L_Input_GetLastInputDeviceUsed));

	inputTable.Set("KeyStringToKeyCode", EQSCRIPT_CFUNC(KeyStringToKeyCode));
	inputTable.Set("KeyCodeToHumanReadableString", EQSCRIPT_CFUNC(KeyCodeToHumanReadableString));
	inputTable.Set("KeyCodeToString", EQSCRIPT_CFUNC(KeyCodeToString));

	state.SetGlobal("input", inputTable);
	return true;
}

void eslSysInputTerm()
{
	LuaInputBinding::ClearAll();
	LuaInputVectorAction::ClearAll();

#ifdef IMGUI_ENABLED
	for (auto it = s_luaImguiHandlerNames.begin(); !it.atEnd(); ++it)
		g_imGuiHost->RemoveDebugHandler(it.value());
#endif
}

bool eslSysSoundEmitterSystemInit(const esl::ScriptState& state)
{
	LUA_SET_GLOBAL_ENUMCONST_3(state, IEqAudioSource::PLAYING, EMITTER_PLAYING);
	LUA_SET_GLOBAL_ENUMCONST_3(state, IEqAudioSource::STOPPED, EMITTER_STOPPED);
	LUA_SET_GLOBAL_ENUMCONST_3(state, IEqAudioSource::PAUSED, EMITTER_PAUSED);

	LUA_SET_GLOBAL_CONST(state, INPUTDEV_KEYBOARD);
	LUA_SET_GLOBAL_CONST(state, INPUTDEV_MOUSE);
	LUA_SET_GLOBAL_CONST(state, INPUTDEV_CONTROLLER);
	LUA_SET_GLOBAL_CONST(state, INPUTDEV_TOUCHPAD);

	LUA_SET_GLOBAL_CONST(state, EMITSOUND_FLAG_FORCE_CACHED);
	LUA_SET_GLOBAL_CONST(state, EMITSOUND_FLAG_FORCE_2D);
	LUA_SET_GLOBAL_CONST(state, EMITSOUND_FLAG_STARTSILENT);
	LUA_SET_GLOBAL_CONST(state, EMITSOUND_FLAG_START_ON_UPDATE);
	LUA_SET_GLOBAL_CONST(state, EMITSOUND_FLAG_RANDOM_PITCH);

	state.RegisterClass<CSoundingObject>();
	state.RegisterClassStatic<CSoundingObject>("ID_ALL", CSoundingObject::ID_ALL);
	state.RegisterClass<EmitParams>();

	{
		state.RegisterClass<CSoundEmitterSystem>();
		esl::LuaTable classTable = state.GetClassTable<CSoundEmitterSystem>();
		classTable.Set("GetAllSoundsList", EQSCRIPT_CFUNC(S_CSoundEmitterSystem_GetAllSoundsList));
	}

	CSoundEmitterSystem* soundEmitterSystem = g_sounds.GetInstancePtr();
	state.SetGlobal("sounds", soundEmitterSystem);

#ifdef IMGUI_ENABLED
	g_imGuiHost->AddDebugMenu("ENGINE/SOUND/SCRIPT EDITOR UI", SoundScriptEditorUIDraw);
#endif

	return true;
}

bool eslSysStateManagerInit(const esl::ScriptState& state)
{
	esl::LuaTable eqStateTable = state.CreateTable();
	eqStateTable.Set("ChangeStateType", EQSCRIPT_CFUNC(eqAppStateMng::ChangeStateType));
	eqStateTable.Set("SetCurrentStateType", EQSCRIPT_CFUNC(eqAppStateMng::SetCurrentStateType));
	eqStateTable.Set("GetCurrentStateType", EQSCRIPT_CFUNC(eqAppStateMng::GetCurrentStateType));
	eqStateTable.Set("GetNextStateType", EQSCRIPT_CFUNC(eqAppStateMng::GetNextStateType));
	eqStateTable.Set("ScheduleNextStateType", EQSCRIPT_CFUNC(eqAppStateMng::ScheduleNextStateType));
	state.SetGlobal("EqStateMgr", eqStateTable);

	return true;
}

bool eslSysHostInit(const esl::ScriptState& state)
{
	esl::LuaTable hostTable = state.CreateTable();
	hostTable.Set("GetFrameTime", EQSCRIPT_CFUNC(S_Host_GetFrameTime));
	hostTable.Set("RequestTextInput", EQSCRIPT_CFUNC(S_Host_RequestTextInput));
	hostTable.Set("ReleaseTextInput", EQSCRIPT_CFUNC(S_Host_ReleaseTextInput));
	hostTable.Set("IsTextInputShown", EQSCRIPT_CFUNC(S_Host_IsTextInputShown));
	hostTable.Set("GetVideoModes", EQSCRIPT_CFUNC(S_Host_GetVideoModes));
	hostTable.Set("GetWindowSize", EQSCRIPT_CFUNC(S_Host_GetWindowSize));
	hostTable.Set("ApplyVideoMode", EQSCRIPT_CFUNC(S_Host_ApplyVideoMode));
	state.SetGlobal("host", hostTable);

	esl::LuaTable buildInfoTbl = state.CreateTable();
	buildInfoTbl.Set("engine", ENGINE_NAME);
	buildInfoTbl.Set("engineVer", ENGINE_VERSION);
	buildInfoTbl.Set("buildNumber", GetEngineBuildNumber());
	buildInfoTbl.Set("configuration", COMPILE_CONFIGURATION);
	buildInfoTbl.Set("platform", COMPILE_PLATFORM);
	buildInfoTbl.Set("date", COMPILE_DATE);
	buildInfoTbl.Set("time", COMPILE_TIME);
	state.SetGlobal("buildInfo", buildInfoTbl);

#ifndef _RETAIL
	g_consoleInput->SetAlternateHandler(EqLuaConsoleHandler);
#endif

	return true;
}

bool eslSysDebugDrawingInit(const esl::ScriptState& state)
{
	state.RegisterClass<IDebugOverlay>();
	state.SetGlobal("debugoverlay", debugoverlay);

	state.RegisterClass<DDText3D>();
	state.RegisterClass<DDBox>();
	state.RegisterClass<DDOrientedBox>();
	state.RegisterClass<DDSphere>();
	state.RegisterClass<DDCylinder>();
	state.RegisterClass<DDLine>();
	state.RegisterClass<DDPoly>();

	state.SetGlobal("DbgText3D", EQSCRIPT_CFUNC(S_DbgText3D, esl::ToLua<DDText3D>, void));
	state.SetGlobal("DbgBox", EQSCRIPT_CFUNC(S_DbgBox, esl::ToLua<DDBox>, void));
	state.SetGlobal("DbgOriBox", EQSCRIPT_CFUNC(S_DbgOriBox, esl::ToLua<DDOrientedBox>, void));
	state.SetGlobal("DbgSphere", EQSCRIPT_CFUNC(S_DbgSphere, esl::ToLua<DDSphere>, void));
	state.SetGlobal("DbgCylinder", EQSCRIPT_CFUNC(S_DbgCylinder, esl::ToLua<DDCylinder>, void));
	state.SetGlobal("DbgLine", EQSCRIPT_CFUNC(S_DbgLine, esl::ToLua<DDLine>, void));
	state.SetGlobal("DbgPoly", EQSCRIPT_CFUNC(S_DbgPoly, esl::ToLua<DDPoly>, void));

#ifdef IMGUI_ENABLED
	imGuilState = state;
	LoadImguiBindings();
#endif // IMGUI_ENABLED

	esl::LuaTable eqImGuiTable = state.CreateTable();
	eqImGuiTable.Set("AddHandler", EQSCRIPT_CFUNC(EqImGui_AddHandler));
	eqImGuiTable.Set("RemoveHandler", EQSCRIPT_CFUNC(EqImGui_RemoveHandler));
	eqImGuiTable.Set("AddDebugMenu", EQSCRIPT_CFUNC(EqImGui_AddDebugMenu));
	eqImGuiTable.Set("ShowDebugMenu", EQSCRIPT_CFUNC(EqImGui_ShowDebugMenu));
	eqImGuiTable.Set("ToggleDebugMenu", EQSCRIPT_CFUNC(EqImGui_ToggleDebugMenu));
	state.SetGlobal("eqImGui", eqImGuiTable);

	return true;
}
