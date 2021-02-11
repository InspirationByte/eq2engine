//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Keys
//////////////////////////////////////////////////////////////////////////////////

#ifndef IN_KEYS_IDENT_H
#define IN_KEYS_IDENT_H

//#define MOU_MOVE   0x0080

#include "core/platform/Platform.h"

#ifdef PLAT_SDL
#include <SDL_keycode.h>
#include <SDL_gamecontroller.h>

// SDL_Scancode

// Platform independent key codes
#define KEY_0			SDL_SCANCODE_0
#define KEY_1			SDL_SCANCODE_1
#define KEY_2			SDL_SCANCODE_2
#define KEY_3			SDL_SCANCODE_3
#define KEY_4			SDL_SCANCODE_4
#define KEY_5			SDL_SCANCODE_5
#define KEY_6			SDL_SCANCODE_6
#define KEY_7			SDL_SCANCODE_7
#define KEY_8			SDL_SCANCODE_8
#define KEY_9			SDL_SCANCODE_9

#define KEY_LEFT		SDL_SCANCODE_LEFT
#define KEY_RIGHT		SDL_SCANCODE_RIGHT
#define KEY_UP			SDL_SCANCODE_UP
#define KEY_DOWN		SDL_SCANCODE_DOWN
#define KEY_CTRL		SDL_SCANCODE_LCTRL
#define KEY_SHIFT		SDL_SCANCODE_LSHIFT		// fixme
#define KEY_ENTER		SDL_SCANCODE_RETURN
#define KEY_SPACE		SDL_SCANCODE_SPACE
#define KEY_TAB			SDL_SCANCODE_TAB
#define KEY_ESCAPE		SDL_SCANCODE_ESCAPE
#define KEY_BACKSPACE	SDL_SCANCODE_BACKSPACE
#define KEY_HOME		SDL_SCANCODE_HOME
#define KEY_END			SDL_SCANCODE_END
#define KEY_INSERT		SDL_SCANCODE_INSERT
#define KEY_DELETE		SDL_SCANCODE_DELETE
#define KEY_TILDE		SDL_SCANCODE_GRAVE
#define KEY_LALT		SDL_SCANCODE_LALT
#define KEY_RALT		SDL_SCANCODE_RALT
#define KEY_PGDN		SDL_SCANCODE_PAGEDOWN
#define KEY_PGUP		SDL_SCANCODE_PAGEUP

#define KEY_F1			SDL_SCANCODE_F1
#define KEY_F2			SDL_SCANCODE_F2
#define KEY_F3			SDL_SCANCODE_F3
#define KEY_F4			SDL_SCANCODE_F4
#define KEY_F5			SDL_SCANCODE_F5
#define KEY_F6			SDL_SCANCODE_F6
#define KEY_F7			SDL_SCANCODE_F7
#define KEY_F8			SDL_SCANCODE_F8
#define KEY_F9			SDL_SCANCODE_F9
#define KEY_F10			SDL_SCANCODE_F10
#define KEY_F11			SDL_SCANCODE_F12
#define KEY_F12			SDL_SCANCODE_F12

#define KEY_NUMPAD0		SDL_SCANCODE_KP_0
#define KEY_NUMPAD1		SDL_SCANCODE_KP_1
#define KEY_NUMPAD2		SDL_SCANCODE_KP_2
#define KEY_NUMPAD3		SDL_SCANCODE_KP_3
#define KEY_NUMPAD4		SDL_SCANCODE_KP_4
#define KEY_NUMPAD5		SDL_SCANCODE_KP_5
#define KEY_NUMPAD6		SDL_SCANCODE_KP_6
#define KEY_NUMPAD7		SDL_SCANCODE_KP_7
#define KEY_NUMPAD8		SDL_SCANCODE_KP_8
#define KEY_NUMPAD9		SDL_SCANCODE_KP_9

#define KEY_ADD			SDL_SCANCODE_KP_PLUS
#define KEY_SUBTRACT	SDL_SCANCODE_KP_MINUS
#define KEY_MULTIPLY	SDL_SCANCODE_KP_MULTIPLY
#define KEY_DIVIDE		SDL_SCANCODE_KP_DIVIDE
#define KEY_KP_ENTER	SDL_SCANCODE_KP_ENTER
#define KEY_SEPARATOR	SDL_SCANCODE_SEPARATOR
#define KEY_DECIMAL		SDL_SCANCODE_DECIMALSEPARATOR
#define KEY_PAUSE		SDL_SCANCODE_PAUSE
#define KEY_CAPSLOCK	SDL_SCANCODE_CAPSLOCK
#define KEY_COMMA		SDL_SCANCODE_COMMA
#define KEY_PERIOD		SDL_SCANCODE_PERIOD
#define KEY_LBRACKET	SDL_SCANCODE_LEFTBRACKET
#define KEY_RBRACKET	SDL_SCANCODE_RIGHTBRACKET
#define KEY_MINUS		SDL_SCANCODE_MINUS
#define KEY_EQUALS		SDL_SCANCODE_EQUALS
#define KEY_SLASH		SDL_SCANCODE_SLASH
#define KEY_BACKSLASH	SDL_SCANCODE_BACKSLASH

#define KEY_A			SDL_SCANCODE_A
#define KEY_B			SDL_SCANCODE_B
#define KEY_C			SDL_SCANCODE_C
#define KEY_D			SDL_SCANCODE_D
#define KEY_E			SDL_SCANCODE_E
#define KEY_F			SDL_SCANCODE_F
#define KEY_G			SDL_SCANCODE_G
#define KEY_H			SDL_SCANCODE_H
#define KEY_I			SDL_SCANCODE_I
#define KEY_J			SDL_SCANCODE_J
#define KEY_K			SDL_SCANCODE_K
#define KEY_L			SDL_SCANCODE_L
#define KEY_M			SDL_SCANCODE_M
#define KEY_N			SDL_SCANCODE_N
#define KEY_O			SDL_SCANCODE_O
#define KEY_P			SDL_SCANCODE_P
#define KEY_Q			SDL_SCANCODE_Q
#define KEY_R			SDL_SCANCODE_R
#define KEY_S			SDL_SCANCODE_S
#define KEY_T			SDL_SCANCODE_T
#define KEY_U			SDL_SCANCODE_U
#define KEY_V			SDL_SCANCODE_V
#define KEY_W			SDL_SCANCODE_W
#define KEY_X			SDL_SCANCODE_X
#define KEY_Y			SDL_SCANCODE_Y
#define KEY_Z			SDL_SCANCODE_Z

#define JOYSTICK_START_KEYS		SDL_NUM_SCANCODES
#define JOYSTICK_MAX_BUTTONS	32

#define JOYSTICK_START_AXES		(JOYSTICK_START_KEYS+JOYSTICK_MAX_BUTTONS)
#define JOYSTICK_MAX_AXES		32

#define KEY_JOY_A						(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_A)
#define KEY_JOY_B						(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_B)
#define KEY_JOY_X						(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_X)
#define KEY_JOY_Y						(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_Y)
#define KEY_JOY_BACK					(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_BACK)
#define KEY_JOY_GUIDE					(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_GUIDE)
#define KEY_JOY_START					(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_START)
#define KEY_JOY_LSTICK					(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_LEFTSTICK)
#define KEY_JOY_RSTICK					(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_RIGHTSTICK)
#define KEY_JOY_LSHOULDER				(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
#define KEY_JOY_RSHOULDER				(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
#define KEY_JOY_DPAD_UP					(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_DPAD_UP)
#define KEY_JOY_DPAD_DOWN				(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_DPAD_DOWN)
#define KEY_JOY_DPAD_LEFT				(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_DPAD_LEFT)
#define KEY_JOY_DPAD_RIGHT				(JOYSTICK_START_KEYS + SDL_CONTROLLER_BUTTON_DPAD_RIGHT)

// Mouse definitions
enum EMouseCodes
{
	MOU_B1		= JOYSTICK_START_AXES+JOYSTICK_MAX_AXES,
	MOU_B2,
	MOU_B3,
	MOU_B4,
	MOU_B5,
	MOU_WHUP,
	MOU_WHDN,
};

#elif PLAT_WIN

// Platform independent key codes
#define KEY_0 int('0')
#define KEY_1 int('1')
#define KEY_2 int('2')
#define KEY_3 int('3')
#define KEY_4 int('4')
#define KEY_5 int('5')
#define KEY_6 int('6')
#define KEY_7 int('7')
#define KEY_8 int('8')
#define KEY_9 int('9')

// Mouse definitions
#define MOU_B1			MK_LBUTTON
#define MOU_B2			MK_RBUTTON
#define MOU_B3			MK_MBUTTON

#if defined(_WIN32)
#	if(_WIN32_WINNT >= 0x0500)
#		define MOU_B4   MK_XBUTTON1
#		define MOU_B5   MK_XBUTTON2
#	else
#		define MOU_B4   0x0020
#		define MOU_B5   0x0040
#	endif
#else
#		define MOU_B4   0x0020
#		define MOU_B5   0x0040
#endif

#define MOU_WHUP		0x0060
#define MOU_WHDN		0x0070

#define KEY_LEFT		VK_LEFT
#define KEY_RIGHT		VK_RIGHT
#define KEY_UP			VK_UP
#define KEY_DOWN		VK_DOWN
#define KEY_CTRL		VK_CONTROL
#define KEY_SHIFT		VK_SHIFT
#define KEY_ENTER		VK_RETURN
#define KEY_SPACE		VK_SPACE
#define KEY_TAB			VK_TAB
#define KEY_ESCAPE		VK_ESCAPE
#define KEY_BACKSPACE	VK_BACK
#define KEY_HOME		VK_HOME
#define KEY_END			VK_END
#define KEY_INSERT		VK_INSERT
#define KEY_DELETE		VK_DELETE
#define KEY_TILDE		VK_OEM_3
#define KEY_LALT		VK_LMENU
#define KEY_RALT		VK_RMENU
#define KEY_PGDN		VK_NEXT
#define KEY_PGUP		VK_PRIOR

#define KEY_F1			VK_F1
#define KEY_F2			VK_F2
#define KEY_F3			VK_F3
#define KEY_F4			VK_F4
#define KEY_F5			VK_F5
#define KEY_F6			VK_F6
#define KEY_F7			VK_F7
#define KEY_F8			VK_F8
#define KEY_F9			VK_F9
#define KEY_F10			VK_F10
#define KEY_F11			VK_F11
#define KEY_F12			VK_F12

#define KEY_NUMPAD0		VK_NUMPAD0
#define KEY_NUMPAD1		VK_NUMPAD1
#define KEY_NUMPAD2		VK_NUMPAD2
#define KEY_NUMPAD3		VK_NUMPAD3
#define KEY_NUMPAD4		VK_NUMPAD4
#define KEY_NUMPAD5		VK_NUMPAD5
#define KEY_NUMPAD6		VK_NUMPAD6
#define KEY_NUMPAD7		VK_NUMPAD7
#define KEY_NUMPAD8		VK_NUMPAD8
#define KEY_NUMPAD9		VK_NUMPAD9

#define KEY_ADD			VK_ADD
#define KEY_SUBTRACT	VK_SUBTRACT
#define KEY_MULTIPLY	VK_MULTIPLY
#define KEY_DIVIDE		VK_DIVIDE
#define KEY_SEPARATOR	VK_SEPARATOR
#define KEY_DECIMAL		VK_DECIMAL
#define KEY_PAUSE		VK_PAUSE
#define KEY_CAPSLOCK	VK_CAPITAL
#define KEY_KP_ENTER	VK_RETURN

#define KEY_A			int('A')
#define KEY_B			int('B')
#define KEY_C			int('C')
#define KEY_D			int('D')
#define KEY_E			int('E')
#define KEY_F			int('F')
#define KEY_G			int('G')
#define KEY_H			int('H')
#define KEY_I			int('I')
#define KEY_J			int('J')
#define KEY_K			int('K')
#define KEY_L			int('L')
#define KEY_M			int('M')
#define KEY_N			int('N')
#define KEY_O			int('O')
#define KEY_P			int('P')
#define KEY_Q			int('Q')
#define KEY_R			int('R')
#define KEY_S			int('S')
#define KEY_T			int('T')
#define KEY_U			int('U')
#define KEY_V			int('V')
#define KEY_W			int('W')
#define KEY_X			int('X')
#define KEY_Y			int('Y')
#define KEY_Z			int('Z')

#define KEY_COMMA		int(',')
#define KEY_PERIOD		int('.')
#define KEY_LBRACKET	int('[')
#define KEY_RBRACKET	int(']')
#define KEY_MINUS		int('-')
#define KEY_EQUALS		int('=')
#define KEY_SLASH		int('/')
#define KEY_BACKSLASH	int('\\')

#define	JOYSTICK_START_AXES	512

#elif defined(LINUX)

// Platform independent key codes
#define KEY_0 int('0')
#define KEY_1 int('1')
#define KEY_2 int('2')
#define KEY_3 int('3')
#define KEY_4 int('4')
#define KEY_5 int('5')
#define KEY_6 int('6')
#define KEY_7 int('7')
#define KEY_8 int('8')
#define KEY_9 int('9')

#define KEY_LEFT      XK_Left
#define KEY_RIGHT     XK_Right
#define KEY_UP        XK_Up
#define KEY_DOWN      XK_Down
#define KEY_CTRL      XK_Control_R
#define KEY_SHIFT     XK_Shift_R
#define KEY_ENTER     XK_Return
#define KEY_SPACE     XK_space
#define KEY_TAB       XK_Tab
#define KEY_ESCAPE    XK_Escape
#define KEY_BACKSPACE XK_BackSpace
#define KEY_HOME      XK_Home
#define KEY_END       XK_End
#define KEY_INSERT    XK_Insert
#define KEY_DELETE    XK_Delete

#define KEY_F1  XK_F1
#define KEY_F2  XK_F2
#define KEY_F3  XK_F3
#define KEY_F4  XK_F4
#define KEY_F5  XK_F5
#define KEY_F6  XK_F6
#define KEY_F7  XK_F7
#define KEY_F8  XK_F8
#define KEY_F9  XK_F9
#define KEY_F10 XK_F10
#define KEY_F11 XK_F11
#define KEY_F12 XK_F12

#define KEY_NUMPAD0 XK_KP_0
#define KEY_NUMPAD1 XK_KP_1
#define KEY_NUMPAD2 XK_KP_2
#define KEY_NUMPAD3 XK_KP_3
#define KEY_NUMPAD4 XK_KP_4
#define KEY_NUMPAD5 XK_KP_5
#define KEY_NUMPAD6 XK_KP_6
#define KEY_NUMPAD7 XK_KP_7
#define KEY_NUMPAD8 XK_KP_8
#define KEY_NUMPAD9 XK_KP_9

#define KEY_ADD        XK_KP_Add
#define KEY_SUBTRACT   XK_KP_Subtract
#define KEY_MULTIPLY   XK_KP_Multiply
#define KEY_DIVIDE     XK_KP_Divide
#define KEY_SEPARATOR  XK_KP_Separator
#define KEY_DECIMAL    XK_KP_Decimal
#define KEY_PAUSE      XK_Pause

#define KEY_A int('a')
#define KEY_B int('b')
#define KEY_C int('c')
#define KEY_D int('d')
#define KEY_E int('e')
#define KEY_F int('f')
#define KEY_G int('g')
#define KEY_H int('h')
#define KEY_I int('i')
#define KEY_J int('j')
#define KEY_K int('k')
#define KEY_L int('l')
#define KEY_M int('m')
#define KEY_N int('n')
#define KEY_O int('o')
#define KEY_P int('p')
#define KEY_Q int('q')
#define KEY_R int('r')
#define KEY_S int('s')
#define KEY_T int('t')
#define KEY_U int('u')
#define KEY_V int('v')
#define KEY_W int('w')
#define KEY_X int('x')
#define KEY_Y int('y')
#define KEY_Z int('z')

#elif defined(__APPLE__)

#define KEY_LEFT      0x7B
#define KEY_RIGHT     0x7C
#define KEY_UP        0x7E
#define KEY_DOWN      0x7D
#define KEY_CTRL      0x3B
#define KEY_SHIFT     0x38
#define KEY_ENTER     0x24
#define KEY_SPACE     0x31
#define KEY_TAB       0x30
#define KEY_ESCAPE    0x35
#define KEY_BACKSPACE 0x33
#define KEY_HOME      0x73
#define KEY_END       0x77
#define KEY_INSERT    0x72
#define KEY_DELETE    0x33

#define KEY_F1  0x7A
#define KEY_F2  0x78
#define KEY_F3  0x63
#define KEY_F4  0x76
#define KEY_F5  0x60
#define KEY_F6  0x61
#define KEY_F7  0x62
#define KEY_F8  0x64
#define KEY_F9  0x65
#define KEY_F10 0x6D
#define KEY_F11 0x67
#define KEY_F12 0x6F

#define KEY_NUMPAD0 0x52
#define KEY_NUMPAD1 0x53
#define KEY_NUMPAD2 0x54
#define KEY_NUMPAD3 0x55
#define KEY_NUMPAD4 0x56
#define KEY_NUMPAD5 0x57
#define KEY_NUMPAD6 0x58
#define KEY_NUMPAD7 0x59
#define KEY_NUMPAD8 0x5B
#define KEY_NUMPAD9 0x5C

#define KEY_ADD        0x45
#define KEY_SUBTRACT   0x4E
#define KEY_MULTIPLY   0x43
#define KEY_DIVIDE     0x4B
#define KEY_SEPARATOR  0x2B
#define KEY_DECIMAL    0x41
//#define KEY_PAUSE      ????

#define KEY_A 0x00
#define KEY_B 0x0B
#define KEY_C 0x08
#define KEY_D 0x02
#define KEY_E 0x0E
#define KEY_F 0x03
#define KEY_G 0x05
#define KEY_H 0x04
#define KEY_I 0x22
#define KEY_J 0x26
#define KEY_K 0x28
#define KEY_L 0x25
#define KEY_M 0x2E
#define KEY_N 0x2D
#define KEY_O 0x1F
#define KEY_P 0x23
#define KEY_Q 0x0C
#define KEY_R 0x0F
#define KEY_S 0x01
#define KEY_T 0x11
#define KEY_U 0x20
#define KEY_V 0x09
#define KEY_W 0x0D
#define KEY_X 0x07
#define KEY_Y 0x10
#define KEY_Z 0x06

#else

#define JOYSTICK_START_KEYS 0

#endif

#ifndef JOYSTICK_START_KEYS
#define JOYSTICK_START_KEYS 0
#endif // JOYSTICK_START_KEYS

// the name associations
struct keyNameMap_t
{
	const char*	name;
	int		    keynum;
	const char* hrname;		// human-readable name
};

static keyNameMap_t s_keyMapList[] =
{
	//{"MOUSE_MOVE", MOU_MOVE},

	{"TAB", KEY_TAB, nullptr},
	{"ENTER", KEY_ENTER, nullptr},
	{"ESCAPE", KEY_ESCAPE, nullptr},
	{"SPACE", KEY_SPACE, nullptr},
	{"BACKSPACE", KEY_BACKSPACE, nullptr},

	{"UPARROW", KEY_UP, nullptr},
	{"DOWNARROW", KEY_DOWN, nullptr},
	{"LEFTARROW", KEY_LEFT, nullptr},
	{"RIGHTARROW", KEY_RIGHT, nullptr},

	{"LALT", KEY_LALT, nullptr},
	{"RALT", KEY_RALT, nullptr},
	{"CTRL", KEY_CTRL, nullptr},
	{"SHIFT", KEY_SHIFT, nullptr},

	{"CAPSLOCK", KEY_CAPSLOCK, nullptr},

	{"F1", KEY_F1, nullptr},
	{"F2", KEY_F2, nullptr},
	{"F3", KEY_F3, nullptr},
	{"F4", KEY_F4, nullptr},
	{"F5", KEY_F5, nullptr},
	{"F6", KEY_F6, nullptr},
	{"F7", KEY_F7, nullptr},
	{"F8", KEY_F8, nullptr},
	{"F9", KEY_F9, nullptr},
	{"F10", KEY_F10, nullptr},
	{"F11", KEY_F11, nullptr},
	{"F12", KEY_F12, nullptr},

	{"INS", KEY_INSERT, nullptr},
	{"DEL", KEY_DELETE, nullptr},
	{"PGDN", KEY_PGDN, nullptr},
	{"PGUP", KEY_PGUP, nullptr},
	{"HOME", KEY_HOME, nullptr},
	{"END", KEY_END, nullptr},

	{"MOUSE1", MOU_B1, "#IN_MOU_B1"},
	{"MOUSE2", MOU_B2, "#IN_MOU_B2"},
	{"MOUSE3", MOU_B3, "#IN_MOU_B3"},
	{"MOUSE4", MOU_B4, "#IN_MOU_B4"},
	{"MOUSE5", MOU_B5, "#IN_MOU_B5"},

	{"NUM_1", KEY_1, nullptr},
	{"NUM_2", KEY_2, nullptr},
	{"NUM_3", KEY_3, nullptr},
	{"NUM_4", KEY_4, nullptr},
	{"NUM_5", KEY_5, nullptr},
	{"NUM_6", KEY_6, nullptr},
	{"NUM_7", KEY_7, nullptr},
	{"NUM_8", KEY_8, nullptr},
	{"NUM_9", KEY_9, nullptr},
	{"NUM_0",KEY_0, nullptr},

	{"A", KEY_A, nullptr},
	{"B", KEY_B, nullptr},
	{"C", KEY_C, nullptr},
	{"D", KEY_D, nullptr},
	{"E", KEY_E, nullptr},
	{"F", KEY_F, nullptr},
	{"G", KEY_G, nullptr},
	{"H", KEY_H, nullptr},
	{"I", KEY_I, nullptr},
	{"J", KEY_J, nullptr},
	{"K", KEY_K, nullptr},
	{"L", KEY_L, nullptr},
	{"M", KEY_M, nullptr},
	{"N", KEY_N, nullptr},
	{"O", KEY_O, nullptr},
	{"P", KEY_P, nullptr},
	{"Q", KEY_Q, nullptr},
	{"R", KEY_R, nullptr},
	{"S", KEY_S, nullptr},
	{"T", KEY_T, nullptr},
	{"U", KEY_U, nullptr},
	{"V", KEY_V, nullptr},
	{"W", KEY_W, nullptr},
	{"X", KEY_X, nullptr},
	{"Y", KEY_Y, nullptr},
	{"Z", KEY_Z, nullptr},

	{"KP_HOME",			KEY_NUMPAD7, nullptr },
	{"KP_UPARROW",		KEY_NUMPAD8, nullptr },
	{"KP_PGUP",			KEY_NUMPAD9, nullptr },
	{"KP_LEFTARROW",	KEY_NUMPAD4, nullptr },
	{"KP_5",			KEY_NUMPAD5, nullptr },
	{"KP_RIGHTARROW",	KEY_NUMPAD6, nullptr },
	{"KP_END",			KEY_NUMPAD1, nullptr },
	{"KP_DOWNARROW",	KEY_NUMPAD2, nullptr },
	{"KP_PGDN",			KEY_NUMPAD3, nullptr },
	{"CAPSLOCK",		KEY_CAPSLOCK, nullptr },
	{"KP_ADD",			KEY_ADD, nullptr},
	{"KP_SUB",			KEY_SUBTRACT, nullptr},
	{"KP_MUL",			KEY_MULTIPLY, nullptr},
	{"KP_DIV",			KEY_DIVIDE, nullptr},
	{"KP_ENTER",		KEY_KP_ENTER, nullptr},

	{"MWHEELUP",		MOU_WHUP, "#IN_MOU_WHUP" },
	{"MWHEELDOWN",		MOU_WHDN, "#IN_MOU_WHDN" },
	{"PAUSE",			KEY_PAUSE, nullptr },
	{"SEMICOLON",		';', nullptr },	// because a raw semicolon seperates commands
	{"COMMA",			KEY_COMMA, nullptr },
	{"PERIOD",			KEY_PERIOD, nullptr },
	{"LBRACKET",		KEY_LBRACKET, nullptr },
	{"RBRACKET",		KEY_RBRACKET, nullptr },
	{"MINUS",			KEY_MINUS, nullptr },
	{"EQUALS",			KEY_EQUALS, nullptr },
	{"SLASH",			KEY_SLASH, nullptr },
	{"BACKSLASH",		KEY_BACKSLASH, nullptr },

	{"GRAVE",			KEY_TILDE, nullptr },

#ifdef PLAT_SDL
	{"JOY_A", KEY_JOY_A, "#IN_JOY_A" },
	{"JOY_B", KEY_JOY_B, "#IN_JOY_B" },
	{"JOY_X", KEY_JOY_X, "#IN_JOY_X" },
	{"JOY_Y", KEY_JOY_Y, "#IN_JOY_Y" },
	{"JOY_BACK", KEY_JOY_BACK, "#IN_JOY_BACK" },
	{"JOY_GUIDE", KEY_JOY_GUIDE, "#IN_JOY_GUIDE" },
	{"JOY_START", KEY_JOY_START, "#IN_JOY_START" },
	{"JOY_LSTICK", KEY_JOY_LSTICK, "#IN_JOY_LSTICK" },
	{"JOY_RSTICK", KEY_JOY_RSTICK, "#IN_JOY_RSTICK" },
	{"JOY_LSHOULDER", KEY_JOY_LSHOULDER, "#IN_JOY_LSHOULDER" },
	{"JOY_RSHOULDER", KEY_JOY_RSHOULDER, "#IN_JOY_RSHOULDER" },
	{"JOY_DPAD_UP", KEY_JOY_DPAD_UP, "#IN_JOY_DPAD_UP" },
	{"JOY_DPAD_DOWN", KEY_JOY_DPAD_DOWN, "#IN_JOY_DPAD_DOWN" },
	{"JOY_DPAD_LEFT", KEY_JOY_DPAD_LEFT, "#IN_JOY_DPAD_LEFT" },
	{"JOY_DPAD_RIGHT", KEY_JOY_DPAD_RIGHT, "#IN_JOY_DPAD_RIGHT" },

	{"JOYAXIS_LX", JOYSTICK_START_AXES + SDL_CONTROLLER_AXIS_LEFTX, "#IN_JOYAXIS_LX" },
	{"JOYAXIS_LY", JOYSTICK_START_AXES + SDL_CONTROLLER_AXIS_LEFTY, "#IN_JOYAXIS_LY" },
	{"JOYAXIS_RX", JOYSTICK_START_AXES + SDL_CONTROLLER_AXIS_RIGHTX, "#IN_JOYAXIS_RX" },
	{"JOYAXIS_RY", JOYSTICK_START_AXES + SDL_CONTROLLER_AXIS_RIGHTY, "#IN_JOYAXIS_RY" },
	{"JOYAXIS_TL", JOYSTICK_START_AXES + SDL_CONTROLLER_AXIS_TRIGGERLEFT, "#IN_JOYAXIS_TL" },
	{"JOYAXIS_TR", JOYSTICK_START_AXES + SDL_CONTROLLER_AXIS_TRIGGERRIGHT, "#IN_JOYAXIS_TR" },
#endif
	{ nullptr, 0, nullptr}
};

// Convert string to key
static int KeyStringToKeyIndex(const char *str)
{
	if (!str)
		return -1;

	int keyind = 0;
	for(keyNameMap_t* kn = s_keyMapList; kn->name; kn++)
	{
		if (!stricmp(str,kn->name))
			return keyind;

		keyind++;
	}

	return -1;
}

static const char* KeyIndexToString(int key)
{
	for(keyNameMap_t* kn = s_keyMapList; kn->name; kn++)
	{
		if (kn->keynum == key)
			return kn->name;
	}

	return "UNKNOWN";
}

static const char* KeyIndexToHumanReadableString(int key)
{
	for (keyNameMap_t* kn = s_keyMapList; kn->name; kn++)
	{
		if (kn->keynum == key)
			return kn->hrname;
	}

	return "UNKNOWN";
}


#endif // IN_KEYS_IDENT_H
