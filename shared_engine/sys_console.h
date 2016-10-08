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

typedef bool (*CONSOLE_ALTERNATE_HANDLER)(const char* commandText);

class ConCommandBase;

class CEqSysConsole
{
public:
	friend class CFont;

			CEqSysConsole();

	void	Initialize();

	// useful for scripts
	void	SetAlternateHandler( CONSOLE_ALTERNATE_HANDLER handler ) {m_alternateHandler = handler;}

	void	DrawSelf(bool transparent, int width, int height, float curTime);

	void	SetLastLine();
	void	AddToLinePos(int num);
	void	SetText( const char* text );

	void	SetVisible(bool bVisible)		{ m_visible = bVisible; }
	bool	IsVisible() const				{return m_visible;}

	void	SetLogVisible( bool bVisible )	{ m_logVisible = bVisible; }
	bool	IsLogVisible() const			{ return m_logVisible; }

	bool	IsShiftPressed() const			{return m_shiftModifier;}
	bool	IsCtrlPressed() const			{return m_ctrlModifier;}

	// events
	bool	KeyPress(int key, bool pressed);
	bool	KeyChar(int ch);
	bool	MouseEvent(const Vector2D &pos, int Button,bool pressed);
	void	MousePos(const Vector2D &pos);

	void	AddAutoCompletionNode(AutoCompletionNode_s *pNode);

protected:
	void	DrawFastFind(float x, float y, float w);
	int		DrawAutoCompletion(float x, float y, float w);

	void	consoleInsText(char* text,int pos);
	void	consoleRemTextInRange(int start,int len);

private:

	IEqFont*						m_font;

	Vector2D						m_mousePosition;
	bool							m_visible;
	bool							m_logVisible;

	bool							m_cursorVisible;
	float							m_cursorTime;
	int								m_maxLines;

	bool							m_shiftModifier;
	bool							m_ctrlModifier;

	int								m_width;
	int								m_height;
	bool							fullscreen;

	uint							m_cursorPos;
	uint							m_startCursorPos;

	int								m_logScrollPosition;

	int								con_histIndex;
	int								con_valueindex;
	ConCommandBase*					con_fastfind_cmdbase;
	DkList<EqString>				autocompletionList;
	int								con_fastfind_selection_autocompletion_val_index;

	CONSOLE_ALTERNATE_HANDLER		m_alternateHandler;

	// Current input text
	EqString						con_Text;

	// Input history
	DkList<EqString>				commandHistory;

	// Autocompletion collection
	DkList<AutoCompletionNode_s*>	m_hAutoCompletionNodes;
};

extern CEqSysConsole* g_pSysConsole;

#endif //CONSOLE_H
