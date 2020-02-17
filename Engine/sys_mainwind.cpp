//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium main entry point
//////////////////////////////////////////////////////////////////////////////////

#include "sys_enginehost.h"
#include "Utils/eqthread.h"
#include <mmsystem.h> // Joystick

using namespace Threading;

bool s_bActive = true;
CEqMutex	g_inputMutex;

#ifdef PLAT_SDL

extern void InputCommands_SDL(SDL_Event* event);

void EQHandleSDLEvents(SDL_Event* event)
{
	switch (event->type)
	{
		case SDL_APP_TERMINATING:
		case SDL_QUIT:
		{
			g_pEngineHost->SetQuitting(IEngineHost::QUIT_TODESKTOP);
			break;
		}
		case SDL_WINDOWEVENT:
		{
			if(event->window.event == SDL_WINDOWEVENT_CLOSE)
			{
				g_pEngineHost->SetQuitting(IEngineHost::QUIT_TODESKTOP);
			}
			else if(event->window.event == SDL_WINDOWEVENT_RESIZED)
			{
				if(event->window.data1 > 0 && event->window.data2 > 0)
				{
					g_pEngineHost->SetWindowSize(event->window.data1, event->window.data2);
				}
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
			{
				s_bActive = true;
			}
			else if(event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
			{
				s_bActive = false;
			}

			break;
		}
		default:
		{
			InputCommands_SDL(event);
		}
	}
}

#elif PLAT_WIN

extern void InputCommands_MSW(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
			PAINTSTRUCT paint;
			BeginPaint(hwnd, &paint);
			s_bActive = !IsRectEmpty(&paint.rcPaint);
			EndPaint(hwnd, &paint);
			break;
			/*
		case WM_KILLFOCUS:
			s_bActive = FALSE;
		case WM_SETFOCUS:
			s_bActive = TRUE;
			
		case WM_ACTIVATE:
			s_bActive = !(wParam == WA_INACTIVE);
			DefWindowProc(hwnd, message, wParam, lParam);
			ShowWindow(hwnd, SW_SHOW);
			break;
			*/
		case WM_SIZE:
			{
				int w = LOWORD(lParam);
				int h = HIWORD(lParam);

				if(w > 0 && h > 0)
					g_pEngineHost->SetWindowSize(w, h);

				break;
			}
			/*
		case WM_WINDOWPOSCHANGED:
			if ((((LPWINDOWPOS) lParam)->flags & SWP_NOSIZE) == 0)
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				int w = rect.right - rect.left;
				int h = rect.bottom - rect.top;

				if (w > 0 && h > 0)
				{
					g_pEngineHost->SetWindowSize(w, h);
					ShowWindow(hwnd, SW_SHOW);
				}
			}
			break;
			*/
		case WM_CREATE:
			{
				ShowWindow(hwnd, SW_SHOW);

				RECT rc;
				GetWindowRect ( hwnd, &rc ) ;

				int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right)/2;
				int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom)/2;

				SetWindowPos( hwnd, HWND_DESKTOP, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
			}

			break;
		case WM_CLOSE:
			g_pEngineHost->SetQuitting(IEngineHost::QUIT_TODESKTOP);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			ProcessRecordedInputCommands( hwnd, message, wParam, lParam);

			return DefWindowProc(hwnd, message, wParam, lParam);

	}

	return 0;
}

bool RegisterWindowClass(HINSTANCE hInst)
{
	WNDCLASS wincl;

	UnregisterClass( Equilibrium_WINDOW_CLASSNAME, hInst );

	// Declare window class
	wincl.hInstance = hInst;
	wincl.lpszClassName = Equilibrium_WINDOW_CLASSNAME;
	wincl.lpfnWndProc = WinProc;
	wincl.style = 0;
	wincl.hIcon = LoadIcon(hInst, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(hInst, IDI_APPLICATION);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = NULL;

	RegisterClass(&wincl);

	return true;
}
#endif // PLAT_SDL

int RunEngine(HINSTANCE hInst)
{
	//// Support for any available game controllers (Not throught DirectInput)
	//JOYCAPS joyCaps;
	//DWORD joyFlags = 0;
	//float xScale = 0, xBias = 0;
	//float yScale = 0, yBias = 0;
	//float zScale = 0, zBias = 0;
	//float rScale = 0, rBias = 0;
	//float uScale = 0, uBias = 0;
	//float vScale = 0, vBias = 0;

	//if (joyGetDevCaps(JOYSTICKID1, &joyCaps, sizeof(joyCaps)) == JOYERR_NOERROR)
	//{
	//	joyFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNBUTTONS;
	//	xScale = 2.0f / float(int(joyCaps.wXmax) - int(joyCaps.wXmin));
	//	xBias  = 1.0f - joyCaps.wXmax * xScale;
	//	yScale = 2.0f / float(int(joyCaps.wYmax) - int(joyCaps.wYmin));
	//	yBias  = 1.0f - joyCaps.wYmax * yScale;

	//	if (joyCaps.wCaps & JOYCAPS_HASZ)
	//	{
	//		joyFlags |= JOY_RETURNZ;
	//		zScale = 2.0f / float(int(joyCaps.wZmax) - int(joyCaps.wZmin));
	//		zBias  = 1.0f - joyCaps.wZmax * zScale;
	//	}
	//	if (joyCaps.wCaps & JOYCAPS_HASR)
	//	{
	//		joyFlags |= JOY_RETURNR;
	//		rScale = 2.0f / float(int(joyCaps.wRmax) - int(joyCaps.wRmin));
	//		rBias  = 1.0f - joyCaps.wRmax * rScale;
	//	}
	//	if (joyCaps.wCaps & JOYCAPS_HASU)
	//	{
	//		joyFlags |= JOY_RETURNU;
	//		uScale = 2.0f / float(int(joyCaps.wUmax) - int(joyCaps.wUmin));
	//		uBias  = 1.0f - joyCaps.wUmax * uScale;
	//	}
	//	if (joyCaps.wCaps & JOYCAPS_HASV)
	//	{
	//		joyFlags |= JOY_RETURNV;
	//		vScale = 2.0f / float(int(joyCaps.wVmax) - int(joyCaps.wVmin));
	//		vBias  = 1.0f - joyCaps.wVmax * vScale;
	//	}
	//}

	// UNDONE: Made to do force the main thread to always run on CPU 0.
	// This is done because on some systems QueryPerformanceCounter returns a bit different counter values
	// on the different CPUs (contrary to what it's supposed to do), which can cause negative frame times
	// if the thread is scheduled on the other CPU in the next frame. This can cause very jerky behavior and
	// appear as if frames return out of order.

	//SetThreadAffinityMask(GetCurrentThread(), 1);

	// ORIGINAL
	Msg("Greetings to original Eq developers and testers: Shurumov Ilya, Seytkamalov Damir, Tsay Maxim\n");

#ifdef PLAT_SDL

	SDL_Event event;

	SDL_StartTextInput();
	do
	{
		while(SDL_PollEvent(&event))
        {
			EQHandleSDLEvents(&event);
        }

		g_pEngineHost->Frame();

		// or yield
		Platform_Sleep(0);
	}
	while(g_pEngineHost->GetQuitting() != IEngineHost::QUIT_TODESKTOP);

#elif PLAT_WIN

	MSG msg;

	do // While engine is not quitting to desktop
	{
		InvalidateRect(g_pEngineHost->GetWindowHandle(), NULL, FALSE);

		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Do update frame if window active
		// UNDONE: Multiplayer stuff
		if (s_bActive /* || g_pEngineHost->IsInMultiplayerGame()*/)
		{
			g_pEngineHost->Frame();

			////Joystick support

			//if (joyFlags)
			//{
			//	static DWORD lastButtons = 0;
			//	static DWORD lastXpos = 0, lastYpos = 0, lastZpos = 0;
			//	static DWORD lastRpos = 0, lastUpos = 0, lastVpos = 0;
			//	JOYINFOEX joyInfo;
			//	joyInfo.dwSize = sizeof(joyInfo);
			//	joyInfo.dwFlags = joyFlags;
			//	if (joyGetPosEx(JOYSTICKID1, &joyInfo) == JOYERR_NOERROR)
			//	{
			//		DWORD changed = lastButtons ^ joyInfo.dwButtons;
			//		if (changed){
			//			for (uint i = 0; i < joyCaps.wNumButtons; i++){
			//				// Only call s_App for buttons that changed
			//				if (changed & 1){
			//					s_App->onJoystickButton(i, ((joyInfo.dwButtons >> i) & 1) != 0);
			//				}
			//				changed >>= 1;
			//			}
			//			lastButtons = joyInfo.dwButtons;
			//		}
			//		if ((joyInfo.dwFlags & JOY_RETURNX) && joyInfo.dwXpos != lastXpos)
			//		{
			//			g_pEngineHost->JoystickAxis(0, joyInfo.dwXpos * xScale + xBias);
			//			lastXpos = joyInfo.dwXpos;
			//		}
			//		if ((joyInfo.dwFlags & JOY_RETURNY) && joyInfo.dwYpos != lastYpos)
			//		{
			//			g_pEngineHost->JoystickAxis(1, joyInfo.dwYpos * yScale + yBias);
			//			lastYpos = joyInfo.dwYpos;
			//		}
			//		if ((joyInfo.dwFlags & JOY_RETURNZ) && joyInfo.dwZpos != lastZpos)
			//		{
			//			g_pEngineHost->JoystickAxis(2, joyInfo.dwZpos * zScale + zBias);
			//			lastZpos = joyInfo.dwZpos;
			//		}
			//		if ((joyInfo.dwFlags & JOY_RETURNR) && joyInfo.dwRpos != lastRpos)
			//		{
			//			g_pEngineHost->JoystickAxis(3, joyInfo.dwRpos * rScale + rBias);
			//			lastRpos = joyInfo.dwRpos;
			//		}
			//		if ((joyInfo.dwFlags & JOY_RETURNU) && joyInfo.dwUpos != lastUpos)
			//		{
			//			g_pEngineHost->JoystickAxis(4, joyInfo.dwUpos * uScale + uBias);
			//			lastUpos = joyInfo.dwUpos;
			//		}
			//		if ((joyInfo.dwFlags & JOY_RETURNV) && joyInfo.dwVpos != lastVpos)
			//		{
			//			g_pEngineHost->JoystickAxis(5, joyInfo.dwVpos * vScale + vBias);
			//			lastVpos = joyInfo.dwVpos;
			//		}
			//	}
			//}
			
		}
		else Platform_Sleep(1000);

		Platform_Sleep(1);
	}
	while(g_pEngineHost->GetQuitting() != IEngineHost::QUIT_TODESKTOP);

#endif // PLAT_SDL

	// shutdown after running
	ShutdownEngineHost();

#ifdef PLAT_SDL
	SDL_Quit();
#elif PLAT_WIN
	UnregisterClass( Equilibrium_WINDOW_CLASSNAME, hInst );
#endif

	return 0;
}