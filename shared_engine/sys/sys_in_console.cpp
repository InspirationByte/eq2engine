//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base console interface
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef far
#undef near
#endif

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "utils/KeyValues.h"

#include <SDL_clipboard.h>
#include <SDL_keyboard.h>

#include "render/IDebugOverlay.h"

#include "font/FontLayoutBuilders.h"
#include "font/FontCache.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "input/InputCommandBinder.h"

#include "sys_version.h"
#include "sys_in_console.h"

#ifdef IMGUI_ENABLED
#include <imgui.h>
#include <imnodes.h>

#include "imgui_backend/imgui_impl_matsystem.h"
#include "imgui_backend/imgui_impl_sys.h"
#endif // IMGUI_ENABLED

#if !defined(_RETAIL)
#define EXTENDED_DEVELOPER_CONSOLE
#endif

using namespace Threading;
static CEqMutex s_conInputMutex;

#ifdef _DEBUG
#define CONSOLE_ENGINEVERSION_STR EqString::Format(ENGINE_NAME " DEBUG build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#else
#define CONSOLE_ENGINEVERSION_STR EqString::Format(ENGINE_NAME " build %d (" COMPILE_DATE ")", BUILD_NUMBER_ENGINE)
#endif // _DEBUG

#define CONSOLE_INPUT_STARTSTR ("> ")
#define CURSOR_BLINK_TIME (0.2f)
const float CMDLIST_SYMBOL_SIZE = 15.0f;

// dummy command
DECLARE_CMD(toggleconsole, nullptr, CV_INVISIBLE)	// dummy console command
{
}


DECLARE_CONCOMMAND_FN(con_toggle)
{
	g_consoleInput->SetVisible(!g_consoleInput->IsVisible());

#if defined(PLAT_SDL) && defined(PLAT_ANDROID)
	if (g_consoleInput->IsVisible())
		SDL_StartTextInput();
	else
		SDL_StopTextInput();
#endif
}
DECLARE_CMD_FN_RENAME(con_toggle_p, "+con_toggle", CONCOMMAND_FN(con_toggle), nullptr, CV_INVISIBLE);
DECLARE_CMD_RENAME(con_toggle_m, "-con_toggle", nullptr, CV_INVISIBLE) {}
DECLARE_CVAR(con_suggest, "1", nullptr, CV_ARCHIVE);
DECLARE_CVAR(con_minicon, "0", nullptr, CV_ARCHIVE);

// shows console
DECLARE_CMD(con_show, "Show console", 0)
{
	g_consoleInput->SetVisible(true);
	g_consoleInput->SetLogVisible(false);

#if defined(PLAT_SDL) && defined(PLAT_ANDROID)
	SDL_StartTextInput();
#endif
}

// shows console with log
DECLARE_CMD(con_show_full, "Show console", 0)
{
	g_consoleInput->SetVisible(true);
	g_consoleInput->SetLogVisible(true);
}

// hides console
DECLARE_CMD(con_hide, "Hides console", 0)
{
	g_consoleInput->SetVisible(false);
	g_consoleInput->SetLogVisible(false);

#if defined(PLAT_SDL) && defined(PLAT_ANDROID)
	SDL_StopTextInput();
#endif
}

// spew function for console
DECLARE_CMD(clear, nullptr,0)
{
	CEqConsoleInput::SpewClear();
}

static int CON_SUGGESTIONS_MAX	= 40;
static float CON_MINICON_TIME	= 5.0f;

const float LOG_SCROLL_DELAY_START	= 0.15f;
const float LOG_SCROLL_DELAY_END	= 0.0f;

const float LOG_SCROLL_DELAY_STEP	= 0.01f;
const float LOG_SCROLL_POWER_INC	= 0.05f;

CStaticAutoPtr<CEqConsoleInput> g_consoleInput;

static ColorRGBA s_conBackColor = ColorRGBA(0.15f, 0.25f, 0.25f, 0.85f);
static ColorRGBA s_conInputBackColor = ColorRGBA(0.15f, 0.25f, 0.25f, 0.85f);
static ColorRGBA s_conBorderColor = ColorRGBA(0.05f, 0.05f, 0.1f, 1.0f);
static ColorRGBA s_conListItemSelectedBackground = ColorRGBA(1.0f, 1.0f, 1.0f, 0.8f);

static ColorRGBA s_conBackFastFind = ColorRGBA(s_conBackColor.xyz(), 0.8f);

static ColorRGBA s_conTextColor = ColorRGBA(0.7f,0.7f,0.8f,1);
static ColorRGBA s_conSelectedTextColor = ColorRGBA(0.2f,0.2f,0.2f,1);
static ColorRGBA s_conInputTextColor = ColorRGBA(0.7f,0.7f,0.6f,1);
static ColorRGBA s_conHelpTextColor = ColorRGBA(0.7f,0.7f,0.8f,1);

static ColorRGBA s_conListItemColor = ColorRGBA(0.7f, 0.7f, 0, 1);
static ColorRGBA s_conListItemSelectedColor = Vector4D(0.1f, 0.1f, 0, 1);

static ColorRGBA s_spewColors[] =
{
	Vector4D(1,1,1,1),				// SPEW_NORM
	Vector4D(0.8f,0.8f,0.8f,1),		// SPEW_INFO
	Vector4D(1,1,0,1),				// SPEW_WARNING
	Vector4D(0.8f,0,0,1),			// SPEW_ERROR
	Vector4D(0.2f,1,0.2f,1)			// SPEW_SUCCESS
};

DECLARE_CMD(con_addAutoCompletion,"Adds autocompletion variants", CV_INVISIBLE)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: con_addAutoCompletion <console command/variable> [argument1,argument2,...]\n");
		return;
	}

	ConAutoCompletion_t* newItem = PPNew ConAutoCompletion_t;
	newItem->cmd_name = CMD_ARGV(0);

	xstrsplit(CMD_ARGV(1),",",newItem->args);

	g_consoleInput->AddAutoCompletion(newItem);
}

DECLARE_CMD(help,"Display the help", 0)
{
	MsgInfo("Type \"cvarlist\" to show available console variables\nType \"cmdlist\" to show aviable console commands\nAlso you can press 'Tab' key to find commands by your typing\n");
	Msg("Hold Shift and use arrows for selection\n");
	Msg("Ctrl + C - Copy selection to clipboard\n");
	Msg("Ctrl + X - Cut selection to clipboard\n");
	Msg("Ctrl + V - Paste\n");
	Msg("Ctrl + A - Select All\n");
	Msg(" \nUse 'PageUp', 'PageDown', 'Home' and 'End' keys to navigate in console\n");
}

bool IsInRectangle(int posX, int posY,int rectX,int rectY,int rectW,int rectH)
{
	return ((posX >= rectX) && (posX <= rectX + rectW) && (posY >= rectY) && (posY <= rectY + rectH));
}

struct ConMessage
{
	ESpewType	type;
	EqString	text;
};
static Array<ConMessage*> s_spewMessages(PP_SL);
using ConMessagePool = MemoryPool<ConMessage>;
static ConMessagePool s_spewMessagesPool(PP_SL);

void CEqConsoleInput::SpewFunc(ESpewType type, const char* pMsg)
{
	// print out to std console
	printf("%s", pMsg );

#ifdef _WIN32
	// debug print only for Windows
	OutputDebugStringA(pMsg);
#endif // _WIN32

	char* pc = (char*)pMsg;
	char* lineStart = pc;

	ConMessage* currentSpewLine = nullptr;

	for(;;pc++)
	{
		if(!(*pc == '\n' || *pc == '\0'))
			continue;

		const int length = pc-lineStart;
		if(length > 0 || *pc == '\n')	// print non empty text and newlines
		{
			CScopedMutex m(s_conInputMutex);
			currentSpewLine = new(s_spewMessagesPool.allocate()) ConMessage;
			currentSpewLine->type = type;
			currentSpewLine->text.Assign(lineStart, length);

			// output to debugoverlay if enabled
			if(con_minicon.GetBool() && debugoverlay != nullptr)
				debugoverlay->TextFadeOut(0, s_spewColors[type], CON_MINICON_TIME, currentSpewLine->text);

			{
				s_spewMessages.append(currentSpewLine);
			}
		}

		// if not a zero, there is line start
		if(*pc != '\0')
			lineStart = pc+1;
		else
			break;
	}
}

void CEqConsoleInput::SpewClear()
{
	CScopedMutex m(s_conInputMutex);

	for(ConMessage* message: s_spewMessages)
	{
		message->~ConMessage();
		s_spewMessagesPool.deallocate(message);
	}

	s_spewMessages.clear();
}

void CEqConsoleInput::SpewInit()
{
	SetSpewFunction(CEqConsoleInput::SpewFunc);
}

void CEqConsoleInput::SpewUninstall()
{
	SpewClear();
	SetSpewFunction(nullptr);
}

//-------------------------------------------------------------------------

CEqConsoleInput::CEqConsoleInput()
{
	// release build needs false
	m_visible = false;
	m_logVisible = false;

	m_showConsole = true;

	m_cursorTime = 0.0f;
	m_maxLines = 10;

	m_font = nullptr;

	m_shiftModifier = false;
	m_ctrlModifier = false;

	m_width = 512;
	m_height = 512;
	m_enabled = false;

	m_cursorPos = 0;
	m_startCursorPos = 0;

	m_logScrollPosition = 0;
	m_logScrollDelay = LOG_SCROLL_DELAY_START;
	m_logScrollDir = 0;
	m_logScrollPower = 1.0f;

	m_histIndex = 0;
	m_fastfind_cmdbase = nullptr;
	m_variantSelection = -1;

	m_alternateHandler = nullptr;
}

void CEqConsoleInput::Initialize(EQWNDHANDLE window)
{
#ifdef IMGUI_ENABLED
	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImNodes::CreateContext();

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplEq_InitForSDL(window);
	ImGui_ImplMatSystem_Init();
#endif // IMGUI_ENABLED

	// TODO: ImGui networked console port (for Android)
	KVSection* consoleSettings = g_eqCore->GetConfig()->FindSection("Console");

	const char* consoleFontName = KV_GetValueString(consoleSettings ? consoleSettings->FindSection("Font") : nullptr, 0, "console");

	m_font = g_fontCache->GetFont(consoleFontName, 16);
	m_enabled = KV_GetValueBool(consoleSettings ? consoleSettings->FindSection("Enable") : nullptr);
	m_fontScale = KV_GetValueFloat(consoleSettings ? consoleSettings->FindSection("FontScale") : nullptr);
}

#ifdef IMGUI_ENABLED
static bool IsImGuiItemsInFocus()
{
	return ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused() || ImGui_ImplEq_AnyWindowInFocus();
}
#endif

void CEqConsoleInput::Shutdown()
{
#ifdef IMGUI_ENABLED
	ImGui_ImplMatSystem_Shutdown();
	ImGui_ImplEq_Shutdown();

	ImNodes::DestroyContext();
	ImGui::DestroyContext();
#endif // IMGUI_ENABLED
}

static void ImGuiBeginMenuPath(const char* path, bool& selected)
{
	char tmpName[128] = { 0 };
	int depth = 0;
	const char* tok = path;
	while (true)
	{
		const char* nextTok = strchr(tok, '/');

		if (nextTok)
		{
			const int len = nextTok - tok;
			strncpy(tmpName, tok, len);
			tmpName[len] = 0;

			if (!ImGui::BeginMenu(tmpName))
				break;
			++depth;
		}
		else
		{
			const int len = strlen(tok);
			strncpy(tmpName, tok, len);
			tmpName[len] = 0;

			ImGui::MenuItem(tok, "", &selected);
		}

		if (!nextTok)
			break;

		tok = nextTok+1;
	}

	while(depth--)
		ImGui::EndMenu();
}

void CEqConsoleInput::BeginFrame()
{
#ifdef IMGUI_ENABLED
	bool imGuiVisible = m_visible;
	for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
	{
		if (it.value().enabled)
		{
			imGuiVisible = true;
			break;
		}
	}

	if (imGuiVisible)
	{
		const bool anyItemShown = ImGui_ImplEq_AnyItemShown();

		// Start the Dear ImGui frame
		ImGui_ImplMatSystem_NewFrame();
		ImGui_ImplEq_NewFrame();
		ImGui::NewFrame();

		if (anyItemShown)
		{
			ImGui_ImplEq_UpdateMousePosAndButtons();
			ImGui_ImplEq_UpdateGamepads();
		}

		for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
		{
			EqImGui_Menu& handler = *it;
			handler.func(handler.enabled);
		}
		m_imguiDrawStart = true;
	}

	if (!m_visible)
		return;

	static bool s_showDemoWindow = false;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("ENGINE"))
		{
			ImGui::MenuItem("Show console", "", &m_showConsole);
			if (ImGui::BeginMenu("FPS"))
			{
				IMGUI_MENUITEM_CONVAR_BOOL("Show FPS", r_showFPS);
				IMGUI_MENUITEM_CONVAR_BOOL("Show Graph", r_showFPSGraph);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::BeginMenu("EqUI"))
			{
				IMGUI_MENUITEM_CONVAR_BOOL("Debug Render", equi_debug);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("MatSystem"))
			{
				IMGUI_MENUITEM_CONVAR_BOOL("Overdraw Mode", r_overdraw);
				IMGUI_MENUITEM_CONVAR_BOOL("Wireframe Mode", r_wireframe);
				ImGui::Separator();
				IMGUI_MENUITEM_CONCMD("Reload All Materials", mat_reload, cmd_noArgs);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("DEBUG OVERLAYS"))
		{
			IMGUI_MENUITEM_CONVAR_BOOL("Show Frame Stats", r_debugDrawFrameStats);
			IMGUI_MENUITEM_CONVAR_BOOL("Show Graphs", r_debugDrawGraphs);
			IMGUI_MENUITEM_CONVAR_BOOL("Show 3D Shapes", r_debugDrawShapes);
			IMGUI_MENUITEM_CONVAR_BOOL("Show 3D Lines", r_debugDrawLines);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("IMGUI"))
		{
			ImGui::MenuItem("Demo", nullptr, &s_showDemoWindow);
			ImGui::EndMenu();
		}

		for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
		{
			EqImGui_Menu& handler = *it;
			if(handler.path.Length())
				ImGuiBeginMenuPath(handler.path, handler.enabled);
		}

		ImGui::EndMainMenuBar();
	}

	if (s_showDemoWindow)
		ImGui::ShowDemoWindow(&s_showDemoWindow);

#undef IMGUI_CONVAR_BOOL
#endif // IMGUI_ENABLED
}

void CEqConsoleInput::EndFrame(int width, int height, float frameTime)
{
	IGPURenderPassRecorderPtr rendPassRecorder = g_renderAPI->BeginRenderPass(
		Builder<RenderPassDesc>()
		.ColorTarget(g_matSystem->GetCurrentBackbuffer())
		.End()
	);

	if (m_visible)
	{
		if (m_showConsole)
			DrawSelf(width, height, frameTime, rendPassRecorder);

		FontStyleParam versionTextStl;
		versionTextStl.styleFlag = TEXT_STYLE_SHADOW | TEXT_STYLE_FROM_CAP;
		versionTextStl.align = TEXT_ALIGN_HCENTER;
		versionTextStl.textColor = ColorRGBA(1, 1, 1, 0.5f);
		versionTextStl.scale = m_fontScale;

		m_font->SetupRenderText(CONSOLE_ENGINEVERSION_STR, Vector2D(width / 2, height - m_fontScale - 25.0f), versionTextStl, rendPassRecorder);
	}

#ifdef IMGUI_ENABLED
	if (m_imguiDrawStart)
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.DisplaySize = ImVec2((float)width, (float)height);

		//static bool show_demo_window = true;
		//ImGui::ShowDemoWindow(&show_demo_window);

		// Rendering
		ImGui::EndFrame();

		ImGui::Render();
		ImGui_ImplMatSystem_RenderDrawData(ImGui::GetDrawData(), rendPassRecorder);
		m_imguiDrawStart = false;
	}
#endif // IMGUI_ENABLED

	g_matSystem->QueueCommandBuffer(rendPassRecorder->End());
}

void CEqConsoleInput::AddAutoCompletion(ConAutoCompletion_t* newItem)
{
	for(int i = 0; i < m_customAutocompletion.numElem();i++)
	{
		if(!m_customAutocompletion[i]->cmd_name.CompareCaseIns(newItem->cmd_name))
		{
			delete newItem;
			return;
		}
	}

	m_customAutocompletion.append(newItem);
}

#ifdef IMGUI_ENABLED

void CEqConsoleInput::AddDebugHandler(const char* name, CONSOLE_IMGUI_HANDLER func)
{
	ASSERT(func);
	const int nameHash = StringToHash(name);
	EqImGui_Menu& handler = m_imguiMenus[nameHash];
	handler.func = func;
	handler.enabled = true; // non-menu are always enabled
}

void CEqConsoleInput::RemoveDebugHandler(const char* name)
{
	const int nameHash = StringToHash(name);
	m_imguiMenus.remove(nameHash);
}

void CEqConsoleInput::AddDebugMenu(const char* path, CONSOLE_IMGUI_HANDLER func)
{
	ASSERT(func);

	const int nameHash = StringToHash(path);
	EqImGui_Menu& handler = m_imguiMenus[nameHash];
	handler.path = path;
	handler.func = func;
}

void CEqConsoleInput::ShowDebugMenu(const char* path, bool enable)
{
	const int nameHash = StringToHash(path);
	auto it = m_imguiMenus.find(nameHash);
	if (it.atEnd())
		return;
	(*it).enabled = enable;
}

void CEqConsoleInput::ToggleDebugMenu(const char* path)
{
	const int nameHash = StringToHash(path);
	auto it = m_imguiMenus.find(nameHash);
	if (it.atEnd())
		return;
	(*it).enabled = !(*it).enabled;
}

#endif // IMGUI_ENABLED

void CEqConsoleInput::DelText(int start, int len)
{
	if(uint(start+len) > m_inputText.Length())
		return;

	m_inputText.Remove(start,len);
	ASSERT(m_inputText.IsValid());
	OnTextUpdate();
}

void CEqConsoleInput::InsText(const char* text,int pos)
{
	m_inputText.Insert(text, pos);
	ASSERT(m_inputText.IsValid());
	OnTextUpdate();
}

void CEqConsoleInput::SetText(const char* text, bool quiet /*= false*/)
{
	m_inputText = text;
	m_cursorPos = m_startCursorPos = m_inputText.Length();

	if (!quiet)
		OnTextUpdate();
}

void DrawAlphaFilledRectangle(const AARectangle &rect, const ColorRGBA &color1, const ColorRGBA &color2, IGPURenderPassRecorder* rendPassRecorder)
{
	const Vector2D r0[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.leftTop.x, rect.rightBottom.y, -1) };
	const Vector2D r1[] = { MAKEQUAD(rect.rightBottom.x, rect.leftTop.y,rect.rightBottom.x, rect.rightBottom.y, -1) };
	const Vector2D r2[] = { MAKEQUAD(rect.leftTop.x, rect.rightBottom.y,rect.rightBottom.x, rect.rightBottom.y, -1) };
	const Vector2D r3[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.rightBottom.x, rect.leftTop.y, -1) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;

	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		// put main rectangle
		meshBuilder.Color4fv(color1);
		meshBuilder.Quad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop());

		// put borders
		meshBuilder.Color4fv(color2);
		meshBuilder.Quad2(r0[0], r0[1], r0[2], r0[3]);
		meshBuilder.Quad2(r1[0], r1[1], r1[2], r1[3]);
		meshBuilder.Quad2(r2[0], r2[1], r2[2], r2[3]);
		meshBuilder.Quad2(r3[0], r3[1], r3[2], r3[3]);
	if(meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

void CEqConsoleInput::DrawListBox(const IVector2D& pos, int width, Array<EqString>& items, const char* tooltipText, int maxItems, int startItem, int& selection, IGPURenderPassRecorder* rendPassRecorder)
{
	FontStyleParam tooltipStyle;
	tooltipStyle.textColor = s_conHelpTextColor;
	tooltipStyle.styleFlag = TEXT_STYLE_FROM_CAP;
	tooltipStyle.scale = m_fontScale;

	FontStyleParam itemStyle(tooltipStyle);
	itemStyle.textColor = s_conListItemColor;

	FontStyleParam selectedItemStyle(itemStyle);
	selectedItemStyle.textColor = s_conListItemSelectedColor;

	const int linesToDraw = min(items.numElem() - startItem, maxItems);	// +1 because tooltip

	if (width == 0)
	{
		width = tooltipText ? m_font->GetStringWidth(tooltipText, tooltipStyle) : 35;

		for (int cnt = 0; cnt < linesToDraw; cnt++)
		{
			int itemIdx = cnt + startItem;
			width = max(width, (int)m_font->GetStringWidth(items[itemIdx], itemStyle));
		}

		width += m_font->GetStringWidth(" ", itemStyle);
	}

	const int tooltipLines = tooltipText ? 1 : 0;

	const int topDotsLines = startItem > 0;
	const int bottomDotsLines = (maxItems+startItem) < items.numElem();

	const int boxLines = linesToDraw + tooltipLines + topDotsLines + bottomDotsLines;

	const float tooltipLineHeight = m_font->GetLineHeight(tooltipStyle);
	const float itemLineHeight = m_font->GetLineHeight(itemStyle);

	AARectangle rect((float)pos.x, (float)pos.y, (float)(pos.x+width), (float)pos.y + boxLines * tooltipLineHeight);
	DrawAlphaFilledRectangle(rect, s_conBackColor, s_conBorderColor, rendPassRecorder);

	m_font->SetupRenderText(tooltipText, rect.GetLeftTop() + Vector2D(5, 4), tooltipStyle, rendPassRecorder);

	if (topDotsLines)
		m_font->SetupRenderText("...", rect.GetLeftTop() + Vector2D(5, 4 + tooltipLineHeight * tooltipLines), tooltipStyle, rendPassRecorder);

	if (bottomDotsLines)
		m_font->SetupRenderText("...", rect.GetLeftTop() + Vector2D(5, 4 + tooltipLineHeight * (linesToDraw + topDotsLines + tooltipLines)), tooltipStyle, rendPassRecorder);


	// draw lines
	for (int cnt = 0; cnt < linesToDraw; cnt++)
	{
		int itemIdx = cnt + startItem;
		EqString& item = items[itemIdx];

		int textYPos = itemLineHeight * (cnt + tooltipLines + topDotsLines);

		if (IsInRectangle(m_mousePosition.x, m_mousePosition.y, pos.x, rect.GetLeftTop().y + textYPos, rect.GetSize().x, itemLineHeight))
			selection = itemIdx;

		if (selection == itemIdx)
		{
			Vertex2D selrect[] = { MAKETEXQUAD((float)pos.x, rect.GetLeftTop().y + textYPos, (float)(pos.x + width), rect.GetLeftTop().y + textYPos + 15 , 0) };

			MatSysDefaultRenderPass defaultRender;
			defaultRender.blendMode = SHADER_BLEND_TRANSLUCENT;
			g_matSystem->SetupDrawDefaultUP(PRIM_TRIANGLE_STRIP, ArrayCRef(selrect), RenderPassContext(rendPassRecorder, &defaultRender));
		}

		m_font->SetupRenderText(item, rect.GetLeftTop() + Vector2D(5, 4 + textYPos), (selection == itemIdx) ? selectedItemStyle : itemStyle, rendPassRecorder);
	}
}

void CEqConsoleInput::DrawFastFind(float x, float y, float w, IGPURenderPassRecorder* rendPassRecorder)
{
	FontStyleParam helpTextParams;
	helpTextParams.textColor = s_conHelpTextColor;
	helpTextParams.styleFlag = TEXT_STYLE_FROM_CAP;
	helpTextParams.scale = m_fontScale;

	// draw history box
	if(m_histIndex != -1 && m_commandHistory.numElem())
	{
		int displayStart = 0;

		while (m_histIndex - displayStart > CON_SUGGESTIONS_MAX - 1)
			displayStart += CON_SUGGESTIONS_MAX;

		DrawListBox(IVector2D(x,y), 0, m_commandHistory, "Last executed command: (cycle: Up/Down, press Enter to repeat)", CON_SUGGESTIONS_MAX, displayStart, m_histIndex, rendPassRecorder);
		return;
	}

#ifdef EXTENDED_DEVELOPER_CONSOLE
	if (!con_suggest.GetBool())
		return;

	// show command info
	if(m_fastfind_cmdbase)
	{
		int numLines = 1;
		{
			const char* descStr = m_fastfind_cmdbase->GetDesc();
			char c = descStr[0];
			do
			{
				if (c == '\0')
					break;

				if (c == '\n')
					numLines++;
			} while (c = *descStr++);
		}

		EqString displayString(_Es(m_fastfind_cmdbase->GetName()) + _Es(" - ") + m_fastfind_cmdbase->GetDesc());
		if(m_fastfind_cmdbase->IsConVar())
		{
			const ConVar* pVar = static_cast<const ConVar*>(m_fastfind_cmdbase);
			if(pVar->HasClamp())
			{
				displayString.Append(EqString::Format(" \n\nValue in range [%g..%g]\n", pVar->GetMinClamp(), pVar->GetMaxClamp()));
				numLines += 2;
			}
		}

		// draw as autocompletion
		AARectangle rect(x,y,w,y + numLines * m_font->GetLineHeight(helpTextParams) + 2);
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor, rendPassRecorder);

		m_font->SetupRenderText(displayString.GetData(), rect.GetLeftTop() + Vector2D(5,4), helpTextParams, rendPassRecorder);

		// draw autocompletion if available
		DrawAutoCompletion(x, rect.rightBottom.y, w, rendPassRecorder);
	}

	if(m_foundCmdList.numElem() >= CON_SUGGESTIONS_MAX)
	{
		AARectangle rect(x,y,w,y + m_font->GetLineHeight(helpTextParams) + 2 );
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor, rendPassRecorder);

		// Set color
		m_font->SetupRenderText(EqString::Format("%d commands found (too many to show here)", m_foundCmdList.numElem()), rect.GetLeftTop() + Vector2D(5,4), helpTextParams, rendPassRecorder);
	}
	else if(m_foundCmdList.numElem() > 0)
	{
		FontStyleParam variantsTextParams;
		variantsTextParams.styleFlag = TEXT_STYLE_FROM_CAP;
		variantsTextParams.scale = m_fontScale;

		const int numElemsToDraw = m_foundCmdList.numElem();

		int maxStringLen = 35;
		for(int i = 0; i < numElemsToDraw; i++)
			maxStringLen = max((uint)maxStringLen, strlen(m_foundCmdList[i]->GetName()));

		AARectangle rect(x,y,x+maxStringLen*CMDLIST_SYMBOL_SIZE,y+numElemsToDraw*m_font->GetLineHeight(helpTextParams)+2);
		DrawAlphaFilledRectangle(rect, s_conBackFastFind, s_conBorderColor, rendPassRecorder);

		for(int i = 0; i < numElemsToDraw;i++)
		{
			const ConCommandBase* cmdBase = m_foundCmdList[i];

			bool bHasAutocompletion = cmdBase->HasVariants();
			if(!bHasAutocompletion)
			{
				// find custom autocompletion
				for(int j = 0;j < m_customAutocompletion.numElem();j++)
				{
					if(!CString::CompareCaseIns(cmdBase->GetName(), m_customAutocompletion[j]->cmd_name.GetData()))
					{
						bHasAutocompletion = true;
						break;
					}
				}
			}

			bool bSelected = false;

			const float textYPos = (y + i * m_font->GetLineHeight(helpTextParams)) + 4;
			if(IsInRectangle(m_mousePosition.x,m_mousePosition.y,x,textYPos+2, rect.rightBottom.x-rect.leftTop.x,12) || m_cmdSelection == i)
			{
				Vertex2D selrect[] = { MAKETEXQUAD(x, textYPos, x+maxStringLen*CMDLIST_SYMBOL_SIZE, textYPos + 15 , 0) };

				MatSysDefaultRenderPass defaultRender;
				defaultRender.blendMode = SHADER_BLEND_TRANSLUCENT;
				defaultRender.drawColor = MColor(1.0f, 1.0f, 1.0f, 0.8f);
				g_matSystem->SetupDrawDefaultUP(PRIM_TRIANGLE_STRIP, ArrayCRef(selrect), RenderPassContext(rendPassRecorder, &defaultRender));

				m_cmdSelection = i;

				bSelected = true;
			}

			ColorRGBA textColor = Vector4D(0.7f,0.7f,0,1);
			if(cmdBase->IsConCommand())
				textColor = Vector4D(0.7f,0.7f,0.8f,1);

			variantsTextParams.textColor = bSelected ? Vector4D(0.1f, 0.1f, 0, 1) : textColor;;

			EqString str;
			if(cmdBase->IsConVar())
			{
				ConVar *cv = (ConVar*)cmdBase;

				if(bHasAutocompletion)
					str = EqString::Format("%s [%s] ->", cv->GetName(), cv->GetString());
				else
					str = EqString::Format("%s [%s]", cv->GetName(), cv->GetString());
			}
			else
			{
				if(bHasAutocompletion)
					str = EqString::Format("%s () ->",cmdBase->GetName());
				else
					str = EqString::Format("%s ()",cmdBase->GetName());
			}

			m_font->SetupRenderText( str, Vector2D(x+5, textYPos),variantsTextParams, rendPassRecorder);

			const char* cstr = xstristr(cmdBase->GetName(), m_inputText.GetData());
			if (!cstr)
				continue;

			// Draw matching rectangle
			const int ofs = cstr - cmdBase->GetName();
			const int len = m_inputText.Length();

			const float lookupStrStart = m_font->GetStringWidth(cmdBase->GetName(), variantsTextParams, ofs);
			const float lookupStrEnd = lookupStrStart + m_font->GetStringWidth(cmdBase->GetName()+ofs, variantsTextParams, len);

			Vertex2D rectVerts[] = { MAKETEXQUAD(x+5 + lookupStrStart, textYPos-2, x+5 + lookupStrEnd, textYPos+12, 0) };

			MatSysDefaultRenderPass defaultRender;
			defaultRender.blendMode = SHADER_BLEND_TRANSLUCENT;
			defaultRender.drawColor = MColor(1.0f, 1.0f, 1.0f, 0.3f);
			g_matSystem->SetupDrawDefaultUP(PRIM_TRIANGLE_STRIP, ArrayCRef(rectVerts), RenderPassContext(rendPassRecorder, &defaultRender));
		} // for i
	}
#endif // EXTENDED_DEVELOPER_CONSOLE
}

void CEqConsoleInput::ResetLogScroll()
{
	m_logScrollDelay = LOG_SCROLL_DELAY_START;
	m_logScrollPower = 1.0f;
	m_logScrollNextTime = 0.0f;
	m_logScrollDir = 0;
}

void CEqConsoleInput::OnTextUpdate()
{
	m_histIndex = -1;
	
	EqString inputText;
	GetCurrentInputText(inputText);

	// update command variants
	UpdateCommandAutocompletionList( inputText );

	// detect space and push variants
	const int spaceIdx = inputText.Find(" ");
	if(spaceIdx != -1)
	{
		EqString firstArguemnt( inputText.Left(spaceIdx) );
		m_fastfind_cmdbase = g_consoleCommands->FindBase( firstArguemnt );

		if(m_fastfind_cmdbase)
		{
			EqString nextArguemnt(inputText.Mid(spaceIdx + 1, inputText.Length() - (spaceIdx + 1)));
			UpdateVariantsList( nextArguemnt );
		}
		else
		{
			m_variantSelection = -1;
			m_variantList.clear(false);
		}
	}
	else
	{
		m_fastfind_cmdbase = nullptr;
	}
}

int CEqConsoleInput::GetCurrentInputText(EqString& str)
{
	int currentStatementStart = 0;

	while(true)
	{
		int nextStatementIdx = m_inputText.Find(";", false, currentStatementStart);

		if(nextStatementIdx == -1)
			break;

		currentStatementStart = nextStatementIdx+1;
	};
	
	str = m_inputText.Mid(currentStatementStart, m_inputText.Length() - currentStatementStart);

	return currentStatementStart;
}

bool CEqConsoleInput::AutoCompleteSelectVariant()
{
	EqString inputText;
	int currentStatementStart = GetCurrentInputText(inputText);

	if(m_fastfind_cmdbase && m_variantSelection != -1)
	{
		// remove from this position
		m_inputText.Remove(currentStatementStart, inputText.Length());
		m_inputText.Append(m_fastfind_cmdbase->GetName() + _Es(" \"") + m_variantList[m_variantSelection] + _Es("\""));

		m_cursorPos = m_startCursorPos = m_inputText.Length();
		OnTextUpdate();
		return true;
	}
	else if(m_foundCmdList.numElem() && m_cmdSelection != -1)
	{
		// remove from this position
		m_inputText.Remove(currentStatementStart, inputText.Length());
		m_inputText.Append(m_foundCmdList[m_cmdSelection]->GetName() + _Es(" "));

		m_cursorPos = m_startCursorPos = m_inputText.Length();
		OnTextUpdate();
		return true;
	}

	return false;
}

void CEqConsoleInput::AutoCompleteSuggestion()
{
	EqString inputText;
	const int currentStatementStart = GetCurrentInputText(inputText);

	int maxMatchingChars = -1;
	EqString matchingStr;

	if(m_fastfind_cmdbase)
	{
		if(m_variantList.numElem() == 1)
		{
			// remove from this position
			m_inputText.Remove(currentStatementStart, inputText.Length());
			m_inputText.Append(m_fastfind_cmdbase->GetName() + _Es(" ") + m_variantList[0]);

			m_cursorPos = m_startCursorPos = m_inputText.Length();
			OnTextUpdate();
			return;
		}

		EqString queryStr;

		const int spaceIdx = inputText.Find(" ");
		if(spaceIdx != -1)
			queryStr = inputText + spaceIdx + 1;

		for(int i = 0; i < m_variantList.numElem(); i++)
		{
			const int charIndex = m_variantList[i].Find(queryStr, true);
			if (charIndex == -1)
				continue;

			if(maxMatchingChars == -1)
			{
				matchingStr = m_variantList[i];
				maxMatchingChars = matchingStr.Length();
			}
			else
			{
				const int cmpLen = matchingStr.GetMathingChars( m_variantList[i] );
				if(cmpLen < maxMatchingChars)
				{
					matchingStr = matchingStr.Left(cmpLen);
					maxMatchingChars = cmpLen;
				}
			}
		}

		if (maxMatchingChars > queryStr.Length() && queryStr.CompareCaseIns(matchingStr))
		{
			m_inputText.Remove(spaceIdx+1, inputText.Length() - (spaceIdx+1));
			m_inputText.Append(matchingStr);
			m_cursorPos = m_startCursorPos = m_inputText.Length();

			OnTextUpdate();
			m_variantSelection = -1;
		}

		// multiple variants are trying to match string beginning
		bool anyFound = false;
		for (int i = m_variantSelection + 1; i < m_variantList.numElem(); i++)
		{
			const EqString& variant = m_variantList[i];
			const int charIndex = variant.Find(queryStr);
			if (charIndex == 0)
			{
				m_variantSelection = i;
				anyFound = true;
				break;
			}
		}

		if (!anyFound)
			m_variantSelection = -1;
	}
	else
	{
		// single variant just autocompletes
		if (m_foundCmdList.numElem() == 1)
		{
			// remove from this position
			m_inputText.Remove(currentStatementStart, inputText.Length());
			m_inputText.Append(EqString::Format("%s ", m_foundCmdList[0]->GetName()));

			m_cursorPos = m_startCursorPos = m_inputText.Length();
			OnTextUpdate();
			return;
		}

		// extending input string
		for (const ConCommandBase* cmdBase : m_foundCmdList)
		{
			const EqString conVarName(cmdBase->GetName());

			if (maxMatchingChars == -1)
			{
				matchingStr = conVarName;
				maxMatchingChars = matchingStr.Length();
			}
			else
			{
				const int cmpLen = matchingStr.GetMathingChars(conVarName);
				if (cmpLen < maxMatchingChars)
				{
					matchingStr = matchingStr.Left(cmpLen);
					maxMatchingChars = cmpLen;
				}
			}
		}

		// maximize text input
		if (maxMatchingChars > m_inputText.Length() && m_inputText.CompareCaseIns(matchingStr))
		{
			m_inputText.Assign(matchingStr);
			m_cursorPos = m_startCursorPos = m_inputText.Length();
			
			OnTextUpdate();
			m_cmdSelection = -1;
		}

		// multiple variants are trying to match string beginning
		bool anyFound = false;

		for (int i = m_cmdSelection + 1; i < m_foundCmdList.numElem(); i++)
		{
			const ConCommandBase* cmdBase = m_foundCmdList[i];
			const EqString conVarName(cmdBase->GetName());

			const int charIndex = conVarName.Find(inputText);
			if (charIndex == 0)
			{
				m_cmdSelection = i;
				anyFound = true;
				break;
			}
		}

		if (!anyFound)
			m_cmdSelection = -1;
	}
}

void CEqConsoleInput::ExecuteCurrentInput()
{
	MsgInfo("> %s\n",m_inputText.GetData());

	g_consoleCommands->ResetCounter();
	g_consoleCommands->SetCommandBuffer(m_inputText);

	Array<EqString> failedCmds(PP_SL);
	bool execStatus = g_consoleCommands->ExecuteCommandBuffer(nullptr, m_alternateHandler != nullptr, &failedCmds);
	bool hasFailed = failedCmds.numElem() > 0;

	if( (execStatus == false || hasFailed) && m_alternateHandler != nullptr)
	{
		hasFailed = !(*m_alternateHandler)(m_inputText);
		execStatus = true;
	}

	if(!execStatus)
		MsgError("Failed to execute '%s'\n",m_inputText.GetData());

	if(hasFailed)
	{
		for(int i = 0; i < failedCmds.numElem(); i++)
			MsgError( "Unknown command or variable: '%s'\n", failedCmds[i].ToCString() );
	}

	// Compare the last command with current and add history if needs
	if(m_commandHistory.numElem() > 0)
	{
		if(m_commandHistory.back().CompareCaseIns(m_inputText))
			m_commandHistory.append(m_inputText);
	}
	else
		m_commandHistory.append(m_inputText);
}

void CEqConsoleInput::UpdateCommandAutocompletionList(const EqString& queryStr)
{
	m_foundCmdList.clear();
	m_cmdSelection = -1;

	const ConCommandListRef cmdList = g_consoleCommands->GetAllCommands();

	for(int i = 0; i < cmdList.numElem();i++)
	{
		ConCommandBase* cmdBase = cmdList[i];

		EqString conVarName(cmdBase->GetName());

		// find by input text
		if(conVarName.Find(queryStr) != -1)
		{
			if(cmdBase->GetFlags() & CV_INVISIBLE)
				continue;

			m_foundCmdList.append(cmdBase);
		}
	}
}

void CEqConsoleInput::UpdateVariantsList( const EqString& queryStr )
{
	if(!con_suggest.GetBool())
		return;

	m_variantSelection = -1;
	m_variantList.clear(false);

	Array<EqString> allAutoCompletion(PP_SL);

	for(int i = 0; i < m_customAutocompletion.numElem(); i++)
	{
		ConAutoCompletion_t* item = m_customAutocompletion[i];

		if(item->cmd_name.CompareCaseIns(m_fastfind_cmdbase->GetName()))
			continue;

		for(int j = 0; j < item->args.numElem(); j++)
			allAutoCompletion.append( item->args[j] );

		break;
	}

	if( m_fastfind_cmdbase->HasVariants() )
		m_fastfind_cmdbase->GetVariants(allAutoCompletion, queryStr);

	// filter out the list
	for(int i = 0; i < allAutoCompletion.numElem(); i++)
	{
		if(queryStr.Length() == 0 || allAutoCompletion[i].Find(queryStr) != -1)
			m_variantList.append(allAutoCompletion[i]);
	}
}

void CEqConsoleInput::DrawAutoCompletion(float x, float y, float w, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!(con_suggest.GetBool() && m_fastfind_cmdbase))
		return;

	if (!m_variantList.numElem())
		return;

	int displayStart = 0;

	while (m_variantSelection - displayStart > CON_SUGGESTIONS_MAX - 1)
		displayStart += CON_SUGGESTIONS_MAX;

	DrawListBox(IVector2D(x, y), 0, m_variantList, "Possible variants: ", CON_SUGGESTIONS_MAX, displayStart, m_variantSelection, rendPassRecorder);
}

void CEqConsoleInput::SetLastLine()
{
	CScopedMutex m(s_conInputMutex);
	int totalLines = s_spewMessages.numElem();

	m_logScrollPosition = totalLines - m_maxLines;
	m_logScrollPosition = max(0,m_logScrollPosition);
}

void CEqConsoleInput::AddToLinePos(int num)
{
	CScopedMutex m(s_conInputMutex);
	int totalLines = s_spewMessages.numElem();
	m_logScrollPosition += num;
	m_logScrollPosition = min(totalLines,m_logScrollPosition);
}

void CEqConsoleInput::DrawSelf(int width,int height, float frameTime, IGPURenderPassRecorder* rendPassRecorder)
{
	CScopedMutex m(s_conInputMutex);
	m_cursorTime -= frameTime;

	if(m_cursorTime < 0.0f)
	{
		m_cursorVisible = !m_cursorVisible;
		m_cursorTime = CURSOR_BLINK_TIME;
	}

#ifdef IMGUI_ENABLED
	if (IsImGuiItemsInFocus())
		m_cursorVisible = false;
#endif

	if (!m_logVisible)
		ResetLogScroll();

#ifdef EXTENDED_DEVELOPER_CONSOLE
	if(m_logScrollDir != 0)
	{
		int maxScroll = s_spewMessages.numElem()-1;

		m_logScrollNextTime -= frameTime;
		if(m_logScrollNextTime <= 0.0f)
		{
			m_logScrollNextTime = m_logScrollDelay;

			m_logScrollPosition += m_logScrollDir*floor(m_logScrollPower);

			m_logScrollPosition = min(m_logScrollPosition, maxScroll);
			m_logScrollPosition = max(m_logScrollPosition, 0);
		}
	}
#endif // #ifdef EXTENDED_DEVELOPER_CONSOLE

	const AARectangle inputTextEntryRect(64, 26, width - 64, 46);
	{
		const Vector2D inputTextPos(inputTextEntryRect.leftTop.x + 4, inputTextEntryRect.rightBottom.y - 6);

		FontStyleParam inputTextStyle;
		inputTextStyle.textColor = s_conInputTextColor;
		inputTextStyle.scale = m_fontScale;

		DrawAlphaFilledRectangle(inputTextEntryRect, s_conInputBackColor, s_conBorderColor, rendPassRecorder);
		m_font->SetupRenderText(CONSOLE_INPUT_STARTSTR + m_inputText, inputTextPos, inputTextStyle, rendPassRecorder);

		MatSysDefaultRenderPass defaultRender;
		defaultRender.blendMode = SHADER_BLEND_TRANSLUCENT;
		RenderPassContext defaultPassContext(rendPassRecorder, &defaultRender);

		const float inputGfxOfs = m_font->GetStringWidth(CONSOLE_INPUT_STARTSTR, inputTextStyle);
		const float cursorPosition = inputGfxOfs + m_font->GetStringWidth(m_inputText, inputTextStyle, m_cursorPos);

		// render selection
		if (m_startCursorPos != m_cursorPos)
		{
			float selStartPosition = inputGfxOfs + m_font->GetStringWidth(m_inputText, inputTextStyle, m_startCursorPos);

			Vertex2D rect[] = { MAKETEXQUAD(inputTextPos.x + selStartPosition,
												inputTextPos.y - 10,
												inputTextPos.x + cursorPosition,
												inputTextPos.y + 4, 0) };

			defaultRender.drawColor = MColor(1.0f, 1.0f, 1.0f, 0.3f);
			g_matSystem->SetupDrawDefaultUP(PRIM_TRIANGLE_STRIP, ArrayCRef(rect), defaultPassContext);
		}

		// render cursor
		if (m_cursorVisible)
		{
			Vertex2D rect[] = { MAKETEXQUAD(inputTextPos.x + cursorPosition,
												inputTextPos.y - 10,
												inputTextPos.x + cursorPosition + 1,
												inputTextPos.y + 4, 0) };

			defaultRender.drawColor = color_white;
			g_matSystem->SetupDrawDefaultUP(PRIM_TRIANGLE_STRIP, ArrayCRef(rect), defaultPassContext);
		}
	}

#ifdef EXTENDED_DEVELOPER_CONSOLE
	if (m_logVisible)
	{
		FontStyleParam fontStyle;
		fontStyle.scale = m_fontScale;

		AARectangle outputRectangle(64.0f, inputTextEntryRect.rightBottom.y + 26, width - 64.0f, height - inputTextEntryRect.leftTop.y);
		DrawAlphaFilledRectangle(outputRectangle, s_conBackColor, s_conBorderColor, rendPassRecorder);

		m_maxLines = floor(height / m_font->GetLineHeight(fontStyle)) - 2;
		if (m_maxLines < -4)
			return;
		m_maxLines = (outputRectangle.GetSize().y / m_font->GetLineHeight(fontStyle)) - 1;

		int numDrawn = 0;

		FontStyleParam hasLinesStyle;
		hasLinesStyle.textColor = ColorRGBA(0.5f, 0.5f, 1.0f, 1.0f);
		hasLinesStyle.scale = m_fontScale;

		outputRectangle.leftTop.x += 5.0f;
		outputRectangle.leftTop.y += m_font->GetLineHeight(fontStyle);

		static CRectangleTextLayoutBuilder rectLayout;
		rectLayout.SetRectangle(outputRectangle);

		FontStyleParam outputTextStyle;
		outputTextStyle.scale = m_fontScale;
		outputTextStyle.layoutBuilder = &rectLayout;

		const int firstLine = m_logScrollPosition;
		const int maxLinesToRender = s_spewMessages.numElem();

		for (int i = firstLine; i < maxLinesToRender; i++, numDrawn++)
		{
			outputTextStyle.textColor = s_spewColors[s_spewMessages[i]->type];
			m_font->SetupRenderText(s_spewMessages[i]->text, outputRectangle.leftTop, outputTextStyle, rendPassRecorder);

			outputRectangle.leftTop.y += m_font->GetLineHeight(fontStyle) * rectLayout.GetProducedLines();//cnumLines;
			if (rectLayout.HasNotDrawnLines() || i - firstLine >= m_maxLines)
			{
				m_font->SetupRenderText("^ ^ ^ ^ ^ ^", Vector2D(outputRectangle.leftTop.x, outputRectangle.rightBottom.y), hasLinesStyle, rendPassRecorder);
				break;
			}
		}
	}
#endif // EXTENDED_DEVELOPER_CONSOLE

	DrawFastFind(inputTextEntryRect.leftTop.x + 15, inputTextEntryRect.leftTop.y + 25, width - 128, rendPassRecorder);
}

void CEqConsoleInput::MousePos(const Vector2D &pos)
{
	m_mousePosition = pos;
}

bool CEqConsoleInput::KeyChar(const char* utfChar)
{
	if (!m_hostCursorActive)
		return false;

#ifdef IMGUI_ENABLED
	ImGui_ImplEq_InputText(utfChar);
	if (IsImGuiItemsInFocus())
		return true;
#endif

	if (!m_visible)
		return false;

	if (!m_showConsole)
		return true;

	if(!m_font)
		return false;

	const int ch = utfChar[0];
	if(ch < 32 || ch == '~' || ch == '`')
		return false;

	if(m_font->GetFontCharById(ch).advX == 0.0f)
		return false;

	const char chr[2]{ ch, 0 };
	InsText(chr, m_cursorPos++);
	m_startCursorPos = m_cursorPos;

	return true;
}

bool CEqConsoleInput::MouseEvent(const Vector2D &pos, int Button,bool pressed)
{
	if (!m_hostCursorActive)
		return false;

#ifdef IMGUI_ENABLED
	ImGui_ImplEq_InputMousePress(Button, pressed);
	if (IsImGuiItemsInFocus())
		return true;
#endif

	if (!m_visible)
		return false;

	if (!m_showConsole)
		return true;

	if (pressed)
	{
		if (Button == MOU_B1)
		{
			AutoCompleteSelectVariant();
		}
	}

	return true;
}

bool CEqConsoleInput::MouseWheel(int hscroll, int vscroll)
{
	if (!m_hostCursorActive)
		return false;

#ifdef IMGUI_ENABLED
	ImGui_ImplEq_InputMouseWheel(hscroll, vscroll);
	if (IsImGuiItemsInFocus())
		return true;
#endif

	if (!m_visible)
		return false;

	return true;
}

void CEqConsoleInput::SetLogVisible(bool bVisible)
{
	m_logVisible = bVisible;
	ResetLogScroll();
}

void CEqConsoleInput::SetVisible(bool bVisible)
{
	if(!m_enabled)
	{
		m_visible = false;
		return;
	}

	ResetLogScroll();

	m_visible = bVisible;

	if(!m_visible)
	{
		m_histIndex = -1;
	}
}

bool CEqConsoleInput::KeyPress(int key, bool pressed)
{
	if (pressed) // catch "DOWN" event
	{
		InputBinding* tgBind = g_inputCommandBinder->FindBindingByCommand(&toggleconsole);

		if (tgBind && s_keyMapList[tgBind->keyIdx].keynum == key)
		{
			if (IsVisible() && IsShiftPressed())
			{
				SetLogVisible(!IsLogVisible());
				return false;
			}

			SetVisible(!IsVisible());
			return false;
		}
	}

	if (!m_hostCursorActive)
		return false;

#ifdef IMGUI_ENABLED
	ImGui_ImplEq_InputKeyPress(key, pressed);
	if (IsImGuiItemsInFocus())
		return true;
#endif

	if (!m_visible)
		return false;

	if (!m_showConsole)
		return true;

	if(pressed)
	{
		switch (key)
		{
			case KEY_BACKSPACE:
			case KEY_DELETE:
			{
				if(m_startCursorPos != m_cursorPos)
				{
					const int selStart = min(m_startCursorPos, m_cursorPos);
					const int selEnd = max(m_startCursorPos, m_cursorPos);

					DelText(selStart, selEnd - selStart);

					m_cursorPos = selStart;
					m_startCursorPos = m_cursorPos;

					m_histIndex = -1;
					break;
				}

				if(key == KEY_BACKSPACE)
				{
					if(m_cursorPos > 0)
					{
						m_cursorPos--;
						m_startCursorPos = m_cursorPos;

						DelText(m_cursorPos, 1);
						m_histIndex = -1;
					}
				}
				else
				{
					DelText(m_cursorPos, 1);
					m_histIndex = -1;
				}

				break;
			}
			case KEY_SHIFT:
			{
				m_shiftModifier = true;

				break;
			}
			case KEY_CTRL:
			{
				m_ctrlModifier = true;

				break;
			}
			case KEY_LEFTARROW:
			{
				m_cursorPos--;
				m_cursorPos = max(m_cursorPos, 0);

				if(!m_shiftModifier)	// drop secondary cursor position
					m_startCursorPos = m_cursorPos;

				break;
			}
			case KEY_RIGHTARROW:
			{
				m_cursorPos++;
				m_cursorPos = min(m_cursorPos, (int)m_inputText.Length());

				if(!m_shiftModifier)	// drop secondary cursor position
					m_startCursorPos = m_cursorPos;

				break;
			}
			case KEY_HOME:
				m_logScrollPosition = 0;
				break;
			case KEY_END:
				SetLastLine();
				break;
			case KEY_A:
				if(m_ctrlModifier)
				{
					m_cursorPos = m_inputText.Length();
					m_startCursorPos = 0;
				}
				break;
#ifdef PLAT_SDL
			case KEY_C:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != m_cursorPos)
					{
						const int selStart = min(m_startCursorPos, m_cursorPos);
						const int selEnd = max(m_startCursorPos, m_cursorPos);

						SDL_SetClipboardText( m_inputText.Mid(selStart, selEnd - selStart) );
					}
				}
				break;
			case KEY_X:
				if(m_ctrlModifier)
				{
					if(m_startCursorPos != m_cursorPos)
					{
						const int selStart = min(m_startCursorPos, m_cursorPos);
						const int selEnd = max(m_startCursorPos, m_cursorPos);

						SDL_SetClipboardText( m_inputText.Mid(selStart, selEnd - selStart) );

						DelText(selStart, selEnd - selStart);

						m_cursorPos = selStart;
						m_startCursorPos = m_cursorPos;
					}
				}
				break;
			case KEY_V:
				if(m_ctrlModifier)
				{
					char* clipBoardText = SDL_GetClipboardText();

					if (!clipBoardText)
						break;

					const int selStart = min(m_startCursorPos, m_cursorPos);

					if(m_startCursorPos != m_cursorPos)
					{
						const int selEnd = max(m_startCursorPos, m_cursorPos);
						DelText(selStart, selEnd - selStart);
					}

					InsText(clipBoardText, selStart);
					m_cursorPos = selStart + strlen(clipBoardText);
					m_startCursorPos = m_cursorPos;

					SDL_free(clipBoardText);
				}
				break;
#endif // PLAT_SDL
			case KEY_ENTER:
				if (m_inputText.Length() > 0)
				{
					if(AutoCompleteSelectVariant())
						break;

					ExecuteCurrentInput();
				}
				else
				{
					if(m_histIndex != -1 && m_commandHistory.numElem())
					{
						SetText(m_commandHistory[m_histIndex]);
						m_histIndex = -1;
						break;
					}

					Msg("]\n");
				}

				SetText("");

				break;

			case KEY_TAB:
				{
					AutoCompleteSuggestion();
				}
				break;
#ifdef EXTENDED_DEVELOPER_CONSOLE
			case KEY_PGUP:
				m_logScrollDir = -1;

				m_logScrollDelay -= LOG_SCROLL_DELAY_STEP;
				m_logScrollPower += LOG_SCROLL_POWER_INC;
				m_logScrollDelay = max(m_logScrollDelay, LOG_SCROLL_DELAY_END);

				break;
			case KEY_PGDN:
				m_logScrollDir = 1;

				m_logScrollDelay -= LOG_SCROLL_DELAY_STEP;
				m_logScrollPower += LOG_SCROLL_POWER_INC;
				m_logScrollDelay = max(m_logScrollDelay, LOG_SCROLL_DELAY_END);

				break;
#endif // EXTENDED_DEVELOPER_CONSOLE
			case KEY_DOWNARROW: // FIXME: invalid indices
#ifdef EXTENDED_DEVELOPER_CONSOLE
				if(m_fastfind_cmdbase && m_variantList.numElem())
				{
					m_variantSelection++;
					m_variantSelection = min(m_variantSelection, m_variantList.numElem()-1);
					break;
				}
				else
				{
					if(m_foundCmdList.numElem())
					{
						m_cmdSelection++;
						m_cmdSelection = min(m_cmdSelection, m_foundCmdList.numElem()-1);
						break;
					}
				}
#endif // EXTENDED_DEVELOPER_CONSOLE
				if(!m_commandHistory.numElem())
					break;

				m_histIndex++;
				m_histIndex = min(m_histIndex, m_commandHistory.numElem()-1);

				if(m_commandHistory.numElem() > 0)
					SetText(m_commandHistory[m_histIndex], true);

				break;
			case KEY_UPARROW:
#ifdef EXTENDED_DEVELOPER_CONSOLE
				if(m_fastfind_cmdbase && m_variantList.numElem())
				{
					m_variantSelection--;
					m_variantSelection = max(m_variantSelection, 0);
					break;
				}
				else
				{
					if(m_foundCmdList.numElem())
					{
						m_cmdSelection--;
						m_cmdSelection = max(m_cmdSelection, 0);
						break;
					}
				}
#endif // EXTENDED_DEVELOPER_CONSOLE
				if(!m_commandHistory.numElem())
					break;

				if(m_histIndex == -1)
					m_histIndex = m_commandHistory.numElem();

				m_histIndex--;
				m_histIndex = max(m_histIndex, 0);

				if(m_commandHistory.numElem() > 0)
					SetText(m_commandHistory[m_histIndex], true);

				break;

			case KEY_ESCAPE:
				if(m_histIndex != -1)
					SetText("", true);

			default:

				if(m_startCursorPos != m_cursorPos)
				{
					const int selStart = min(m_startCursorPos, m_cursorPos);
					const int selEnd = max(m_startCursorPos, m_cursorPos);
					m_inputText.Remove(selStart, selEnd - selStart);

					m_cursorPos = selStart;
					m_startCursorPos = m_cursorPos;
				}

				m_histIndex = -1; // others and escape
				m_startCursorPos = m_cursorPos;
				break;
		}
	}
	else
	{
		switch (key)
		{
			case KEY_SHIFT:
				m_shiftModifier = false;
				break;
			case KEY_CTRL:
				m_ctrlModifier = false;
				break;
			case KEY_PGUP:
			case KEY_PGDN:
				ResetLogScroll();
				break;
		}
	}

	return true;
}