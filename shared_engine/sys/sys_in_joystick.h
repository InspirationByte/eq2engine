//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium joystick support brought by SDL
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_CONTROLLERS 4

union SDL_Event;
struct _SDL_Haptic;

class CEqGameControllerSDL
{
public:
	CEqGameControllerSDL() 
		: m_connected(false), m_gameCont(0), m_instanceId(-1), m_haptic(0)
	{
	}

	static void Init();
	static void Shutdown();

	static CEqGameControllerSDL* GetFreeController();

	static void ProcessConnectionEvent(SDL_Event* event);
	static void ProcessInputEvent(SDL_Event* event);

	static void RepeatEvents(float fDt);

	const char* GetName() const;

private:

	SDL_GameController*	m_gameCont;
	_SDL_Haptic*		m_haptic;
	SDL_JoystickID		m_instanceId;
	bool				m_connected;

	Map<short, float>	m_pressed{ PP_SL };

	static int			GetControllerIndex(SDL_JoystickID instance);

	void				Open(int device);
	void				Close();
};
