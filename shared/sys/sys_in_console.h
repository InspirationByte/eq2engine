//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IEqFont;
class ConCommandBase;
class IGPURenderPassRecorder;

struct ConAutoCompletion
{
	EqString name;
	Array<EqString> args{ PP_SL };
};

typedef bool (*CONSOLE_ALTERNATE_HANDLER)(const char* commandText);

class CEqConsoleInput
{
public:
	friend class CFont;

	static void		SpewFunc(ESpewType type,const char* pMsg);

	static void		SpewClear();
	static void		SpewInit();
	static void		SpewUninstall();

	CEqConsoleInput() = default;

	void			Init();
	void			Shutdown();

	// useful for scripts
	void			SetAlternateHandler( CONSOLE_ALTERNATE_HANDLER handler ) {m_alternateHandler = handler;}

	void			BeginFrame();
	void			EndFrame(int width, int height, float frameTime, IGPURenderPassRecorder* rendPassRecorder);

	void			SetLastLine();
	void			AddToLinePos(int num);

	void			SetHostCursorActive(bool value)	{ m_hostCursorActive = value; }

	void			SetVisible(bool value);
	bool			IsVisible() const			{ return m_visible; }

	void			SetLogVisible(bool value);
	bool			IsLogVisible() const		{ return m_logVisible; }

	// events
	bool			KeyPress(int key, bool pressed);
	bool			KeyChar(const char* utfChar);
	bool			MouseEvent(const Vector2D &pos, int Button,bool pressed);
	bool			MouseWheel(int hscroll, int vscroll);

	bool			IsShiftPressed() const { return m_shiftModifier; }
	bool			IsCtrlPressed() const { return m_ctrlModifier; }

	void			MousePos(const Vector2D &pos);

	void			AddAutoCompletion(EqStringRef name, EqStringRef commaSeparatedArgs);

protected:

	void			DrawSelf(const IAARectangle& viewport, float frameTime, IGPURenderPassRecorder* rendPassRecorder);
	void			DrawListBox(const IVector2D& pos, int width, ArrayCRef<EqString> items, const char* tooltipText, int maxItems, int startItem, int& selection, IGPURenderPassRecorder* rendPassRecorder);

	void			DrawFastFind(float x, float y, float w, IGPURenderPassRecorder* rendPassRecorder);
	void			DrawAutoCompletion(float x, float y, float w, IGPURenderPassRecorder* rendPassRecorder);

	void			DelText(int start, int len);
	void			InsText(const char* text,int pos);
	void			SetText(const char* text, bool quiet = false);

	void			UpdateCommandAutocompletionList(const EqStringRef queryStr);
	void			UpdateVariantsList( const EqString& queryStr );
	void			OnTextUpdate();

	void			ResetLogScroll();

	// returns current statement start and current input text
	int				GetCurrentInputText(EqString& str);

	bool			AutoCompleteSelectVariant();
	void			AutoCompleteSuggestion();

	void			ExecuteCurrentInput();

private:

	IEqFont*						m_font{ nullptr };
	float							m_fontScale{ 8.0f };

	Vector2D						m_mousePosition{ 0 };

	float							m_cursorTime{ 0.0f };
	int								m_maxLines{ 10 };
	int								m_cursorPos{ 0 };
	int								m_startCursorPos{ 0 };

	int								m_logScrollPosition{ 0 };
	float							m_logScrollAccumTime{ 0 };
	float							m_logScrollTime{ 0 };
	int								m_logScrollDir{ 1 };

	bool							m_shiftModifier{ false };
	bool							m_ctrlModifier{ false };

	bool							m_showConsole{ true };
	bool							m_enabled{ false };

	bool							m_cursorVisible{ false };
	bool							m_hostCursorActive{ false };
	bool							m_visible{ false };
	bool							m_logVisible{ false };

	// Input history
	Array<EqString>					m_commandHistory{ PP_SL };
	int								m_histIndex{ -1 };

	Array<const ConCommandBase*>	m_foundCmdList{ PP_SL };
	int								m_cmdSelection{ -1 };

	const ConCommandBase*			m_fastfind_cmdbase;
	Array<EqString>					m_variantList{ PP_SL };
	int								m_variantSelection{ -1 };

	CONSOLE_ALTERNATE_HANDLER		m_alternateHandler{ nullptr };

	// Current input text
	EqString						m_inputText;

	// custom autocompletion
	Array<ConAutoCompletion>		m_customAutocompletion{ PP_SL };
};

extern CStaticAutoPtr<CEqConsoleInput> g_consoleInput;