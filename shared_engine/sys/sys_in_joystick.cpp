//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium joystick support brought by SDL
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "sys_host.h"
#include "sys_in_joystick.h"

#include "input/in_keys_ident.h"

DECLARE_CVAR(in_joy_debug, "0", "Joystick debug messages", 0);
DECLARE_CVAR(in_joy_repeatDelayInit, "0.5", "Joystick input repeat delay initial", CV_ARCHIVE);
DECLARE_CVAR(in_joy_repeatDelay, "0.1", "Joystick input repeat delay", CV_ARCHIVE);
DECLARE_CVAR(in_joy_rumble, "1", "Rumble", CV_ARCHIVE);
DECLARE_CVAR(in_joy_id, "0", "Joystick to use. -1 means unconnected or unselected", CV_ARCHIVE);
DECLARE_CMD(in_joy_list, "List connected gamepads", 0)
{
	GameControllerList controllers = CEqGameControllerSDL::GetControllers();
	for (CEqGameControllerSDL* ctrl : controllers)
	{
		const int idx = CEqGameControllerSDL::GetControllerIndex(ctrl);
		Msg("%d %s %s\n", idx, in_joy_id.GetInt() == idx ? "***" : "   ", ctrl->GetName());
	}
	Msg("--- %d connected gamepads ---\n", controllers.numElem());
}

#define CONTROLLER_DB_FILENAME "cfg/controllers.db"

#ifdef PLAT_SDL
#include <SDL.h>

DECLARE_CMD(in_joy_addMapping, "Adds joystick mapping in SDL2 format", 0)
{
	if (CMD_ARGC == 0)
	{
		MsgError("mapping required as argument!");
		return;
	}

	const int result = SDL_GameControllerAddMapping(CMD_ARGV(0));
	if (result <= 0)
	{
		if (result == -1)
			MsgError("Failed to add mapping\n");
		else if (result == 0)
			MsgWarning("Mapping already added!\n");

		return;
	}
	
	IFilePtr dbFile = g_fileSystem->Open(CONTROLLER_DB_FILENAME, FS_OPEN_APPEND, SP_DATA);
	if(!dbFile)
		return;

	dbFile->Print("%s\n", CMD_ARGV(0));
}

static CEqGameControllerSDL s_controllers[MAX_CONTROLLERS];

static void JoySetValidControllerId()
{
	bool getNewController = in_joy_id.GetInt() == -1;
	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		CEqGameControllerSDL& jc = s_controllers[i];
		if (!jc.IsConnected())
		{
			if (in_joy_id.GetInt() == i)
			{
				getNewController = true;
				in_joy_id.SetInt(-1);
			}
		}
		else if (getNewController)
		{
			in_joy_id.SetInt(i);
			getNewController = false;
		}
	}
}

void CEqGameControllerSDL::Init()
{
	const int numJoysticks = SDL_NumJoysticks();
	if (numJoysticks)
		MsgWarning("* %d joysticks connected\n", numJoysticks);

	VSSize mappingsSize = 0;
	const char* mappingsBuf = (const char*)g_fileSystem->GetFileBuffer(CONTROLLER_DB_FILENAME, &mappingsSize);
	if (mappingsBuf)
	{
		SDL_RWops* mappingsIO = SDL_RWFromMem((void*)mappingsBuf, mappingsSize);
		int result = SDL_GameControllerAddMappingsFromRW(mappingsIO, 1);

		if (result == -1)
			MsgError("Failed add mappings from '" CONTROLLER_DB_FILENAME "'!\n");
		else
			MsgInfo("Added %d mappings from '" CONTROLLER_DB_FILENAME "'\n", result);
		
		PPFree((void*)mappingsBuf);
	}
	else if (numJoysticks)
		MsgInfo("No '" CONTROLLER_DB_FILENAME "' found, skipping\n");

	JoySetValidControllerId();
}

void CEqGameControllerSDL::Shutdown()
{
	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		CEqGameControllerSDL& jc = s_controllers[i];

		if (!jc.IsConnected())
			continue;

		jc.Close();
	}
}

CEqGameControllerSDL* CEqGameControllerSDL::GetFreeController()
{
	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		CEqGameControllerSDL& jc = s_controllers[i];
		if (!jc.IsConnected())
			return &jc;
	}

	return nullptr;
}

GameControllerList CEqGameControllerSDL::GetControllers()
{
	GameControllerList list;
	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		CEqGameControllerSDL& jc = s_controllers[i];
		if (jc.IsConnected())
			list.append(&jc);
	}
	return list;
}

const char* CEqGameControllerSDL::GetName() const
{
	if (!IsConnected())
		return "disconnected";

	return SDL_GameControllerName(m_gameCont);
}

// Opens the joystick controller
void CEqGameControllerSDL::Open(int device)
{
	char guidStr[64];
	char* mapping = SDL_GameControllerMappingForDeviceIndex(device);
	if (!mapping)
	{
		SDL_Joystick* j = SDL_GameControllerGetJoystick(m_gameCont);
		const int joyDevId = SDL_JoystickInstanceID(j);
		SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(device);
		SDL_JoystickGetGUIDString(guid, guidStr, 64);
		MsgWarning("   '%s':%s - no controller mapping available!\n", SDL_JoystickNameForIndex(joyDevId), guidStr);
		return;
	}

	SDL_free(mapping);

	m_gameCont = SDL_GameControllerOpen(device);
	if (!m_gameCont)
	{
		MsgError("Can't open game controller: %s\n", SDL_GetError());
		return;
	}

	SDL_Joystick *j = SDL_GameControllerGetJoystick(m_gameCont);
	m_instanceId = SDL_JoystickInstanceID(j);

	if (SDL_JoystickIsHaptic(j)) 
	{
		m_haptic = SDL_HapticOpenFromJoystick(j);

		MsgInfo("Haptic - effects: %d, query: %d\n", SDL_HapticNumEffects(m_haptic), SDL_HapticQuery(m_haptic));
		if (SDL_HapticRumbleSupported(m_haptic)) 
		{
			if (SDL_HapticRumbleInit(m_haptic) != 0)
			{
				MsgError("Haptic Rumble Init: %s\n", SDL_GetError());
				SDL_HapticClose(m_haptic);
				m_haptic = nullptr;
			}
		}
		else
		{
			SDL_HapticClose(m_haptic);
			m_haptic = nullptr;
		}
	}

	JoySetValidControllerId();

	m_pressed.setNum(SDL_CONTROLLER_BUTTON_MAX);
	m_stateChanged.resize(SDL_CONTROLLER_BUTTON_MAX);

	for (int i = 0; i < m_pressed.numElem(); ++i)
		m_pressed[i] = -1.0f;

	Msg("* Controller connected: '%s' dev=%d inst=%d\n", GetName(), device, m_instanceId);
}

void CEqGameControllerSDL::Close()
{
	if (m_instanceId == -1)
		return;

	if (m_haptic)
		SDL_HapticClose(m_haptic);

	Msg("* Controller disconnected: '%s'\n", GetName());

	SDL_GameControllerClose(m_gameCont);
	m_gameCont = nullptr;
	m_haptic = nullptr;
	m_instanceId = -1;

	JoySetValidControllerId();
}

int CEqGameControllerSDL::GetControllerIndex(CEqGameControllerSDL* controller)
{
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		if (&s_controllers[i] == controller)
			return i;
	}

	return -1;
}

int CEqGameControllerSDL::GetControllerIndex(SDL_JoystickID instance)
{
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		if (s_controllers[i].m_instanceId == instance)
			return i;
	}

	return -1;
}

void CEqGameControllerSDL::RepeatEvents(float fDt)
{
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		CEqGameControllerSDL& jc = s_controllers[i];
		if (!jc.IsConnected())
			continue;

		for(int button = 0; button < jc.m_pressed.numElem(); ++button)
		{
			if (jc.m_stateChanged[button])
				g_pHost->JoyButton_Event(button, jc.m_pressed[button] >= 0);

			jc.m_stateChanged.setFalse(button);

			if(jc.m_pressed[button] < 0)
				continue; // repeater inactive

			// repeater active
			float timeLeft = jc.m_pressed[button] - fDt;
			if (timeLeft < 0)
			{
				g_pHost->JoyButton_Event(button, true);
				timeLeft = in_joy_repeatDelay.GetFloat();
			}
			jc.m_pressed[button] = timeLeft;
		}
	}
}

void CEqGameControllerSDL::ProcessInputEvent(SDL_Event* event)
{
	switch (event->type) 
	{
		case SDL_CONTROLLERAXISMOTION: 
		{
			SDL_GameControllerAxis axis = (SDL_GameControllerAxis)event->caxis.axis;

			if (in_joy_debug.GetBool())
			{
				Msg("Gamepad %d axis %s value: %d\n",
					event->caxis.which,
					KeyIndexToString(JOYSTICK_START_AXES + axis), event->caxis.value);
			}

			const int cIndex = GetControllerIndex(event->cdevice.which);
			if (cIndex >= 0)
			{
				if (in_joy_id.GetInt() == cIndex)
				{
					CEqGameControllerSDL& jc = s_controllers[cIndex];

					// handle axis motion
					g_pHost->JoyAxis_Event((short)axis, event->caxis.value);
				}
			}

			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP: 
		{
			SDL_GameControllerButton button = (SDL_GameControllerButton)event->cbutton.button;

			const bool down = (event->cbutton.state == SDL_PRESSED);
			if (in_joy_debug.GetBool())
			{
				Msg("Gamepad %d button %s %s\n",
					event->cbutton.which, KeyIndexToString(JOYSTICK_START_KEYS + button), down ? "down" : "up");
			}

			const int cIndex = GetControllerIndex(event->cdevice.which);
			if (cIndex >= 0 && in_joy_id.GetInt() == cIndex)
			{
				CEqGameControllerSDL& jc = s_controllers[cIndex];
				if (down)
				{
					if(jc.m_pressed[button] < 0)
					{
						jc.m_pressed[button] = in_joy_repeatDelayInit.GetFloat();
						jc.m_stateChanged.setTrue(button);
;					}
				}
				else
				{
					if(jc.m_pressed[button] >= 0)
					{
						jc.m_pressed[button] = -1.0f;
						jc.m_stateChanged.setTrue(button);
					}
				}
			}
			
			break;
		}
	}
}

void CEqGameControllerSDL::ProcessConnectionEvent(SDL_Event* event)
{
	switch (event->type)
	{
		case SDL_CONTROLLERDEVICEADDED:
		{
			CEqGameControllerSDL* jc = GetFreeController();
			if (jc)
				jc->Open(event->cdevice.which);

			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			const int cIndex = GetControllerIndex(event->cdevice.which);
			if (cIndex >= 0)
			{
				CEqGameControllerSDL& jc = s_controllers[cIndex];
				jc.Close();
			}
			break;
		}
	}
}

#endif // PLAT_SDL