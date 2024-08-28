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

#include <SDL.h>

#include "sys_host.h"
#include "sys_in_joystick.h"

#include "input/in_keys_ident.h"

DECLARE_CVAR(in_joy_debug, "0", "Joystick debug messages", 0);
DECLARE_CVAR(in_joy_repeatDelayInit, "1", "Joystick input repeat delay initial", CV_ARCHIVE);
DECLARE_CVAR(in_joy_repeatDelay, "0.2", "Joystick input repeat delay", CV_ARCHIVE);
DECLARE_CVAR(in_joy_rumble, "1", "Rumble", CV_ARCHIVE);
DECLARE_CVAR(in_joy_id, "-1", "Joystick to use. -1 is for all", CV_ARCHIVE);

#define CONTROLLER_DB_FILENAME "cfg/controllers.db"

#ifdef PLAT_SDL

DECLARE_CMD(in_joy_addMapping, "Adds joystick mapping in SDL2 format", 0)
{
	if (CMD_ARGC == 0)
	{
		MsgError("mapping required as argument!");
		return;
	}

	int result = SDL_GameControllerAddMapping(CMD_ARGV(0).ToCString());

	if (result <= 0)
	{
		if (result == -1)
			MsgError("Failed to add mapping\n");
		else if (result == 0)
			MsgWarning("Mapping already added!\n");

		return;
	}
	
	IFilePtr dbFile = g_fileSystem->Open(CONTROLLER_DB_FILENAME, "wt+", SP_DATA);
	if(!dbFile)
		return;

	dbFile->Print(CMD_ARGV(0).ToCString());
	dbFile->Print("\n");
}

static CEqGameControllerSDL s_controllers[MAX_CONTROLLERS];

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

	char guidStr[64];

	for (int i = 0; i < numJoysticks; i++)
	{
		char* mapping = SDL_GameControllerMappingForDeviceIndex(i);

		if (mapping)
		{
			int cIndex = GetControllerIndex(i);

			// perform adding event
			if (cIndex == -1)
			{
				SDL_Event deviceevent;
				deviceevent.type = SDL_CONTROLLERDEVICEADDED;
				deviceevent.cdevice.which = i;
				SDL_PushEvent(&deviceevent);
			}
		}
		else
		{
			SDL_JoystickGUID guid = SDL_JoystickGetDeviceGUID(i);

			SDL_JoystickGetGUIDString(guid, guidStr, 64);
			MsgWarning("   '%s':%s - no controller mapping available!\n", SDL_JoystickNameForIndex(i), guidStr);
		}

		SDL_free(mapping);
	}
}

void CEqGameControllerSDL::Shutdown()
{
	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		CEqGameControllerSDL& jc = s_controllers[i];

		if (!jc.m_connected)
			continue;

		jc.Close();
	}
}

CEqGameControllerSDL* CEqGameControllerSDL::GetFreeController()
{
	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		CEqGameControllerSDL& jc = s_controllers[i];

		if (!jc.m_connected)
			return &jc;
	}

	return nullptr;
}

const char* CEqGameControllerSDL::GetName() const
{
	if (!m_connected)
		return "disconnected";

	return SDL_GameControllerName(m_gameCont);
}

// Opens the joystick controller
void CEqGameControllerSDL::Open(int device)
{
	m_gameCont = SDL_GameControllerOpen(device);

	SDL_Joystick *j = SDL_GameControllerGetJoystick(m_gameCont);
	m_instanceId = SDL_JoystickInstanceID(j);

	m_connected = true;

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
}

void CEqGameControllerSDL::Close()
{
	if (!m_connected)
		return;

	m_connected = false;

	if (m_haptic)
	{
		SDL_HapticClose(m_haptic);
		m_haptic = nullptr;
	}

	SDL_GameControllerClose(m_gameCont);
	m_gameCont = nullptr;
}

int CEqGameControllerSDL::GetControllerIndex(SDL_JoystickID instance)
{
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		if (s_controllers[i].m_connected &&
			s_controllers[i].m_instanceId == instance)
			return i;
	}

	return -1;
}

void CEqGameControllerSDL::RepeatEvents(float fDt)
{
	for (int i = 0; i < MAX_CONTROLLERS; ++i)
	{
		CEqGameControllerSDL& jc = s_controllers[i];

		if (!jc.m_connected)
			continue;

		for (auto it = jc.m_pressed.begin(); !it.atEnd(); ++it) {

			float val = *it - fDt;
			val -= fDt;

			if (val > 0.0f)
			{
				jc.m_pressed[it.key()] = val;
				continue;
			}

			*it = in_joy_repeatDelay.GetFloat();
			g_pHost->TrapJoyButton_Event(it.key(), true);

			//SDL_HapticRumblePlay(jc.m_haptic, 1.0, 50);
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
			{
				jc->Open(event->cdevice.which);
				Msg("* Controller connected: '%s'\n", jc->GetName());
			}
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			int cIndex = GetControllerIndex(event->cdevice.which);

			if (cIndex >= 0)
			{
				CEqGameControllerSDL& jc = s_controllers[cIndex];

				Msg("* Controller disconnected: '%s'\n", jc.GetName());
				jc.Close();
			}

			break;
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
				if (in_joy_id.GetInt() == -1 || in_joy_id.GetInt() == cIndex)
				{
					CEqGameControllerSDL& jc = s_controllers[cIndex];

					// handle axis motion
					g_pHost->TrapJoyAxis_Event((short)axis, event->caxis.value);
				}
			}

			break;
		}
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP: 
		{
			SDL_GameControllerButton button = (SDL_GameControllerButton)event->cbutton.button;

			bool down = (event->cbutton.state == SDL_PRESSED);

			if (in_joy_debug.GetBool())
			{
				Msg("Gamepad %d button %s %s\n",
					event->cbutton.which, KeyIndexToString(JOYSTICK_START_KEYS + button), down ? "down" : "up");
			}

			int cIndex = GetControllerIndex(event->cdevice.which);

			if (cIndex >= 0)
			{
				if (in_joy_id.GetInt() == -1 || in_joy_id.GetInt() == cIndex)
				{
					CEqGameControllerSDL& jc = s_controllers[cIndex];

					if (down)
						jc.m_pressed[button] = in_joy_repeatDelayInit.GetFloat();
					else
						jc.m_pressed.remove(button);

					// handle button up/down
					g_pHost->TrapJoyButton_Event((short)button, down);
				}
			}
			
			break;
		}
	}
}

#endif // PLAT_SDL