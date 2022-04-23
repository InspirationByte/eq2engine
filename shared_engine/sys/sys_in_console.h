//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CONSOLE_H
#define CONSOLE_H

#include <stdio.h>
#include "core/DebugInterface.h"
#include "core/platform/Platform.h"

#include "ds/eqstring.h"
#include "ds/Array.h"

#include "font/IFont.h"

struct ConAutoCompletion_t
{
	EqString cmd_name;
	Array<EqString> args;
};

typedef bool (*CONSOLE_ALTERNATE_HANDLER)(const char* commandText);

class ConCommandBase;

class CEqConsoleInput
{
public:
	friend class CFont;

	static void		SpewFunc(SpewType_t type,const char* pMsg);

	static void		SpewClear();
	static void		SpewInit();
	static void		SpewUninstall();


					CEqConsoleInput();

	void			Initialize(EQWNDHANDLE window);
	void			Shutdown();

	// useful for scripts
	void			SetAlternateHandler( CONSOLE_ALTERNATE_HANDLER handler ) {m_alternateHandler = handler;}

	void			BeginFrame();
	void			EndFrame(int width, int height, float frameTime);

	void			SetLastLine();
	void			AddToLinePos(int num);
	void			SetText( const char* text, bool quiet = false );

	void			SetVisible(bool bVisible);
	bool			IsVisible() const				{return m_visible;}

	void			SetLogVisible(bool bVisible);
	bool			IsLogVisible() const			{ return m_logVisible; }

	// events
	bool			KeyPress(int key, bool pressed);
	bool			KeyChar(const char* utfChar);
	bool			MouseEvent(const Vector2D &pos, int Button,bool pressed);
	bool			MouseWheel(int hscroll, int vscroll);

	void			MousePos(const Vector2D &pos);

	void			AddAutoCompletion(ConAutoCompletion_t* item);

protected:

	void			DrawSelf(int width, int height, float frameTime);
	void			DrawListBox(const IVector2D& pos, int width, Array<EqString>& items, const char* tooltipText, int maxItems, int startItem, int& selection);

	void			DrawFastFind(float x, float y, float w);
	void			DrawAutoCompletion(float x, float y, float w);

	void			consoleInsText(char* text,int pos);
	void			consoleRemTextInRange(int start,int len);

	void			UpdateCommandAutocompletionList(const EqString& queryStr);
	void			UpdateVariantsList( const EqString& queryStr );
	void			OnTextUpdate();

	void			ResetLogScroll();

	bool			IsShiftPressed() const { return m_shiftModifier; }
	bool			IsCtrlPressed() const { return m_ctrlModifier; }

	// returns current statement start and current input text
	int				GetCurrentInputText(EqString& str);

	bool			AutoCompleteSelectVariant();
	void			AutoCompleteSuggestion();

	void			ExecuteCurrentInput();

private:

	IEqFont*						m_font;
	float							m_fontScale;

	Vector2D						m_mousePosition;
	bool							m_visible;
	bool							m_logVisible;

	bool							m_showConsole;

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
	Array<EqString>				m_commandHistory;
	int								m_histIndex;

	Array<ConCommandBase*>			m_foundCmdList;
	int								m_cmdSelection;

	ConCommandBase*					m_fastfind_cmdbase;
	Array<EqString>				m_variantList;
	int								m_variantSelection;

	CONSOLE_ALTERNATE_HANDLER		m_alternateHandler;

	// Current input text
	EqString						m_inputText;

	// custom autocompletion
	Array<ConAutoCompletion_t*>	m_customAutocompletion;
};

extern CEqConsoleInput* g_consoleInput;

#endif //CONSOLE_H
