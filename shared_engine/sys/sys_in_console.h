//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#if !defined(_RETAIL)
#define IMGUI_ENABLED
#endif

class IEqFont;
class ConCommandBase;

struct ConAutoCompletion_t
{
	EqString cmd_name;
	Array<EqString> args{ PP_SL };
};

typedef bool (*CONSOLE_ALTERNATE_HANDLER)(const char* commandText);

#ifdef IMGUI_ENABLED
using CONSOLE_IMGUI_HANDLER = EqFunction<void(const char* name)>;
#endif // IMGUI_ENABLED

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

#ifdef IMGUI_ENABLED
	void			AddImGuiMenuHandler(const char* name, CONSOLE_IMGUI_HANDLER func);
	void			RemoveImGuiMenuHandler(const char* name);
#endif // IMGUI_ENABLED

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
	Array<EqString>					m_commandHistory{ PP_SL };
	int								m_histIndex;

	Array<ConCommandBase*>			m_foundCmdList{ PP_SL };
	int								m_cmdSelection;

	ConCommandBase*					m_fastfind_cmdbase;
	Array<EqString>					m_variantList{ PP_SL };
	int								m_variantSelection;

#ifdef IMGUI_ENABLED
	struct EqImGui_Handler
	{
		EqString name;
		CONSOLE_IMGUI_HANDLER handlerFunc;
	};

	Map<int, EqImGui_Handler>		m_menuHandlers{ PP_SL };
#endif // IMGUI_ENABLED
	CONSOLE_ALTERNATE_HANDLER		m_alternateHandler;

	// Current input text
	EqString						m_inputText;

	// custom autocompletion
	Array<ConAutoCompletion_t*>		m_customAutocompletion{ PP_SL };
};

extern CEqConsoleInput* g_consoleInput;