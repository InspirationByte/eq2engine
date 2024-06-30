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

struct ConAutoCompletion_t
{
	EqString cmd_name;
	Array<EqString> args{ PP_SL };
};

typedef bool (*CONSOLE_ALTERNATE_HANDLER)(const char* commandText);

#ifdef IMGUI_ENABLED
enum EqImGuiHandleTypes : int
{
	IMGUI_HANDLE_NONE = 0,
	IMGUI_HANDLE_MENU = (1 << 0)
};
using CONSOLE_IMGUI_HANDLER = EqFunction<void(const char* name, EqImGuiHandleTypes type)>;

#define IMGUI_MENUITEM_CONVAR_BOOL(label, name) { \
		HOOK_TO_CVAR(name); \
		bool value = name ? name->GetBool() : false; \
		ImGui::MenuItem(label, "", &value); \
		if(name) name->SetBool(value); \
	}

static Array<EqStringRef> cmd_noArgs(PP_SL);
#define IMGUI_MENUITEM_CONCMD(label, name, args) { \
		HOOK_TO_CMD(name); \
		if(ImGui::MenuItem(label)) \
			name->DispatchFunc(args); \
	}

#endif // IMGUI_ENABLED

class CEqConsoleInput
{
public:
	friend class CFont;

	static void		SpewFunc(ESpewType type,const char* pMsg);

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

	void			MousePos(const Vector2D &pos);

	void			AddAutoCompletion(ConAutoCompletion_t* item);

#ifdef IMGUI_ENABLED
	void			AddImGuiHandle(const char* name, CONSOLE_IMGUI_HANDLER func);
	void			RemoveImGuiHandle(const char* name);
#endif // IMGUI_ENABLED

protected:

	void			DrawSelf(int width, int height, float frameTime, IGPURenderPassRecorder* rendPassRecorder);
	void			DrawListBox(const IVector2D& pos, int width, Array<EqString>& items, const char* tooltipText, int maxItems, int startItem, int& selection, IGPURenderPassRecorder* rendPassRecorder);

	void			DrawFastFind(float x, float y, float w, IGPURenderPassRecorder* rendPassRecorder);
	void			DrawAutoCompletion(float x, float y, float w, IGPURenderPassRecorder* rendPassRecorder);

	void			DelText(int start, int len);
	void			InsText(const char* text,int pos);
	void			SetText(const char* text, bool quiet = false);

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
	bool							m_hostCursorActive;
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

	Array<const ConCommandBase*>	m_foundCmdList{ PP_SL };
	int								m_cmdSelection;

	const ConCommandBase*			m_fastfind_cmdbase;
	Array<EqString>					m_variantList{ PP_SL };
	int								m_variantSelection;

#ifdef IMGUI_ENABLED
	struct EqImGui_Handle
	{
		EqString name;
		CONSOLE_IMGUI_HANDLER handleFunc;
		int flags;
	};

	Map<int, EqImGui_Handle>		m_imguiHandles{ PP_SL };
	bool							m_imguiDrawStart{ false };
#endif // IMGUI_ENABLED
	CONSOLE_ALTERNATE_HANDLER		m_alternateHandler;

	// Current input text
	EqString						m_inputText;

	// custom autocompletion
	Array<ConAutoCompletion_t*>		m_customAutocompletion{ PP_SL };
};

extern CStaticAutoPtr<CEqConsoleInput> g_consoleInput;