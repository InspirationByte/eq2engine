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

#include "utils/eqstring.h"
#include "utils/DkList.h"
#include "IFont.h"

struct ConAutoCompletion_t
{
	EqString cmd_name;
	DkList<EqString> args;
};

typedef bool (*CONSOLE_ALTERNATE_HANDLER)(const char* commandText);

class ConCommandBase;

class CEqSysConsole
{
public:
	friend class CFont;

	static void		SpewFunc(SpewType_t type,const char* pMsg);

	static void		SpewClear();
	static void		SpewInit();
	static void		SpewUninstall();


					CEqSysConsole();

	void			Initialize();

	// useful for scripts
	void			SetAlternateHandler( CONSOLE_ALTERNATE_HANDLER handler ) {m_alternateHandler = handler;}

	void			DrawSelf(int width, int height, float frameTime);

	void			SetLastLine();
	void			AddToLinePos(int num);
	void			SetText( const char* text, bool quiet = false );

	void			SetVisible(bool bVisible);
	bool			IsVisible() const				{return m_visible;}

	void			SetLogVisible( bool bVisible )	{ m_logVisible = bVisible; }
	bool			IsLogVisible() const			{ return m_logVisible; }

	bool			IsShiftPressed() const			{return m_shiftModifier;}
	bool			IsCtrlPressed() const			{return m_ctrlModifier;}

	// events
	bool			KeyPress(int key, bool pressed);
	bool			KeyChar(int ch);
	bool			MouseEvent(const Vector2D &pos, int Button,bool pressed);
	void			MousePos(const Vector2D &pos);

	void			AddAutoCompletion(ConAutoCompletion_t* item);

protected:
	void			DrawFastFind(float x, float y, float w);
	int				DrawAutoCompletion(float x, float y, float w);

	void			consoleInsText(char* text,int pos);
	void			consoleRemTextInRange(int start,int len);

	void			UpdateCommandAutocompletionList(const EqString& queryStr);
	void			UpdateVariantsList( const EqString& queryStr );
	void			OnTextUpdate();

	// returns current statement start and current input text
	int				GetCurrentInputText(EqString& str);

	bool			AutoCompleteSelectVariant();
	void			AutoCompleteSuggestion();

	void			ExecuteCurrentInput();

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

	bool							m_enabled;

	int								m_cursorPos;
	int								m_startCursorPos;

	int								m_logScrollPosition;

	float							m_logScrollDelay;
	float							m_logScrollPower;
	int								m_logScrollDir;

	float							m_logScrollNextTime;

	// Input history
	DkList<EqString>				m_commandHistory;
	int								m_histIndex;

	DkList<ConCommandBase*>			m_foundCmdList;
	int								m_cmdSelection;

	ConCommandBase*					m_fastfind_cmdbase;
	DkList<EqString>				m_variantList;
	int								m_variantSelection;

	CONSOLE_ALTERNATE_HANDLER		m_alternateHandler;

	// Current input text
	EqString						m_inputText;

	// custom autocompletion
	DkList<ConAutoCompletion_t*>	m_customAutocompletion;
};

extern CEqSysConsole* g_pSysConsole;

#endif //CONSOLE_H
