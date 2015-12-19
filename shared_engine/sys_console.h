//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdio.h>
#include "DebugInterface.h"
#include "math/Vector.h"

#include "Font.h"
//#include "IEngineHost.h"

//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
//-*-*-*-*-*-*- Autocompletion for fastfind -*-*-*-*-*-*-*-
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

// int argumentType definitions
#define ARG_TYPE_GENERIC			0
#define ARG_TYPE_CVARLIST			1
#define ARG_TYPE_CMDLIST			2
#define ARG_TYPE_MAPLIST			3
#define ARG_TYPE_CFGLIST			4

// Comparsion-only definitions
#define ARG_TYPE_CVARLIST_STRING	"$cvarlist$"
#define ARG_TYPE_CMDLIST_STRING		"$cmdlist$"
#define ARG_TYPE_MAPLIST_STRING		"$map_list$"
#define ARG_TYPE_CFGLIST_STRING		"$cfg_list$"


struct AutoCompletionNode_s
{
	AutoCompletionNode_s()
	{
		argumentType = ARG_TYPE_GENERIC;
	}

	EqString cmd_name;
	DkList<EqString> args;

	int argumentType;
};

//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// Console main class
//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-

#ifdef KeyPress
#undef KeyPress
#endif // KeyPress

class CEqSysConsole
{
public:
	friend class CFont;

			CEqSysConsole();

	void	DrawSelf(bool transparent,int width,int height, IEqFont* font, float curTime);

	bool	KeyPress(int key, bool pressed);
	void	MouseEvent(const Vector2D &pos, int Button,bool pressed);
	void	MousePos(const Vector2D &pos);
	void	KeyChar(int ch);
	void	SetLastLine();
	void	AddToLinePos(int num);

	void	SetVisible(bool bVisible) { bShowConsole = bVisible; }
	bool	IsVisible() {return bShowConsole;}
	void	SetText( const char* text ) { con_Text = text; con_cursorPos = con_Text.GetLength();}

	void	AddAutoCompletionNode(AutoCompletionNode_s *pNode);

	bool	IsShiftPressed() {return m_bshiftHolder;}

	bool	con_full_enabled;

	EQWNDHANDLE		m_hwnd;

	IEqFont*		con_font;

protected:
	void	drawFastFind(float x, float y, float w, IEqFont* fontid);
	int		DrawAutoCompletion(float x, float y, float w, IEqFont* fontid);

	void	consoleInsText(char* text,int pos);
	void	consoleRemTextInRange(int start,int len);

private:

	Vector2D						mousePosition;
	bool							bShowConsole;

	float							con_cursorTime;
	int								drawcount;

	bool							m_bshiftHolder;
	bool							m_bCtrlHolder;

	int								win_wide;
	int								win_tall;
	bool							fullscreen;

	uint							con_cursorPos;
	uint							con_cursorPos_locked;

	int								conLinePosition;

	int								con_histIndex;
	int								con_valueindex;
	int								con_fastfind_selection_index;
	int								con_fastfind_selection_autocompletion_index;
	int								con_fastfind_selection_autocompletion_val_index;
	bool							con_fastfind_isbeingselected;
	bool							con_fastfind_isbeingselected_autoc;

	// Current input text
	EqString						con_Text;

	// Input history
	DkList<EqString>				commandHistory;

	// Autocompletion collection
	DkList<AutoCompletionNode_s*>	m_hAutoCompletionNodes;
};

extern CEqSysConsole* g_pSysConsole;

#endif //CONSOLE_H
