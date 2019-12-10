//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium joystick support brought by SDL
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"

#include "sys_host.h"
#include "sys_in_joystick.h"

#include "in_keys_ident.h"

#include "IFileSystem.h"

#include "ConVar.h"

ConVar in_joy_debug("in_joy_debug", "0", "Joystick debug messages", 0);

ConVar in_joy_repeatDelayInit("in_joy_repeatDelayInit", "1", "Joystick input repeat delay initial", CV_ARCHIVE);
ConVar in_joy_repeatDelay("in_joy_repeatDelay", "0.2", "Joystick input repeat delay", CV_ARCHIVE);
ConVar in_joy_rumble("in_joy_rumble", "1", "Rumble", CV_ARCHIVE);

static CEqGameControllerSDL s_controllers[MAX_CONTROLLERS];

void CEqGameControllerSDL::Init()
{
	const char* mappingBuf = g_fileSystem->GetFileBuffer("resources/gamecontrollerdb.txt");
	if (mappingBuf)
	{
		if(SDL_GameControllerAddMapping(mappingBuf) == -1)
			MsgError("Failed add mappings from 'resources/gamecontrollerdb.txt'!\n");

		PPFree((void*)mappingBuf);
	}
	else
		MsgError("Failed to load 'resources/gamecontrollerdb.txt'!\n");

	int numJoysticks = SDL_NumJoysticks();
	MsgWarning("* %d gamepads connected\n", numJoysticks);
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

		MsgInfo("Haptic effects: %d\n", SDL_HapticNumEffects(m_haptic));
		MsgInfo("Haptic query: %x\n", SDL_HapticQuery(m_haptic));

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

		for (const auto& button : jc.m_pressed) {

			float val = button.second - fDt;
			val -= fDt;

			if (val > 0.0f)
			{
				jc.m_pressed[button.first] = val;
				continue;
			}

			jc.m_pressed[button.first] = in_joy_repeatDelay.GetFloat();

			g_pHost->TrapJoyButton_Event(button.first, true);

			//SDL_HapticRumblePlay(jc.m_haptic, 1.0, 50);
		}
	}
}

int CEqGameControllerSDL::ProcessEvent(SDL_Event* event)
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
					KeyIndexToString(JOYSTICK_START_AXES + event->caxis.axis), event->caxis.value);
			}

			// handle axis motion
			g_pHost->TrapJoyAxis_Event((short)axis, event->caxis.value);
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
					event->cbutton.which, KeyIndexToString(JOYSTICK_START_KEYS + event->cbutton.button), down ? "down" : "up");
			}

			CEqGameControllerSDL& jc = s_controllers[event->cdevice.which];

			if (down)
				jc.m_pressed[button] = in_joy_repeatDelayInit.GetFloat();
			else
				jc.m_pressed.erase(button);

			// handle button up/down
			g_pHost->TrapJoyButton_Event((short)button, down);
			break;
		}
		case SDL_CONTROLLERDEVICEADDED: 
		{
			if (event->cdevice.which < MAX_CONTROLLERS) 
			{
				CEqGameControllerSDL& jc = s_controllers[event->cdevice.which];
				jc.Open(event->cdevice.which);

				Msg("* Controller connected: '%s'\n", jc.GetName());
			}
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED: 
		{
			int cIndex = GetControllerIndex(event->cdevice.which);

			if (cIndex < 0)
				return 0; // unknown controller?

			CEqGameControllerSDL& jc = s_controllers[cIndex];

			Msg("* Controller disconnected: '%s'\n", jc.GetName());
			jc.Close();

			break;
		}
	}

	return 0;
}