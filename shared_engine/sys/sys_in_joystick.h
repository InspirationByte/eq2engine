//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium joystick support brought by SDL
//////////////////////////////////////////////////////////////////////////////////

#pragma once

static constexpr int MAX_CONTROLLERS = 4;

union SDL_Event;
typedef struct _SDL_Haptic SDL_Haptic;
typedef struct _SDL_GameController SDL_GameController;
typedef signed int SDL_JoystickID;

class CEqGameControllerSDL;

using GameControllerList = FixedArray<CEqGameControllerSDL*, MAX_CONTROLLERS>;

class CEqGameControllerSDL
{
public:
	const char*		GetName() const;
	bool			IsConnected() const { return m_instanceId >= 0; }

	static void		Init();
	static void		Shutdown();
	static void		ProcessConnectionEvent(SDL_Event* event);
	static void		ProcessInputEvent(SDL_Event* event);
	static void		RepeatEvents(float fDt);

	static CEqGameControllerSDL*	GetFreeController();
	static GameControllerList		GetControllers();
	static int						GetControllerIndex(CEqGameControllerSDL* controller);

private:

	SDL_GameController* m_gameCont{ nullptr };
	_SDL_Haptic*		m_haptic{ nullptr };
	SDL_JoystickID		m_instanceId{ -1 };

	Map<short, float>	m_pressed{ PP_SL };

	static int			GetControllerIndex(SDL_JoystickID instance);

	void				Open(int device);
	void				Close();
};
