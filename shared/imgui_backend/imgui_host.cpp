#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#ifdef IMGUI_ENABLED

#include <imgui.h>
#include <imnodes.h>

#include "utils/KeyValues.h"

#include "imgui_impl_matsystem.h"
#include "imgui_impl_sys.h"
#include "imgui_internal.h"
#endif // IMGUI_ENABLED

#include "imgui_host.h"

static CEqImGuiHost s_imGuiHost;
CEqImGuiHost* g_imGuiHost = &s_imGuiHost;

#ifdef IMGUI_ENABLED

static ImVec4 ParseImGuiColorValue(const KVSection& key)
{
	const char* valueStr = key.ValueAt(0).GetString();
	if (key.ValueCount() == 1 && valueStr[0] == '#')
	{
		MColor hexColor = hexToColor4(valueStr + 1);
		return ImVec4(hexColor.r, hexColor.g, hexColor.b, hexColor.a);
	}

	if (key.ValueCount() < 3)
		MsgWarning("ImGui style: '%s' needs RGB or RGBA values\n", key.GetName());

	ImVec4 color(0.0f, 0.0f, 0.0f, 1.0f);
	key.GetValues(color.x, color.y, color.z, color.w);
	return color;
}

// Descriptor value that writes the parsed entry directly into an ImGuiStyle member
template<typename T>
struct ImGuiStyleValue
{
	T*		ptr{ nullptr };
	void	operator=(const T& value) const { *ptr = value; }
};

template<typename T>
struct KVPairValuesGetter<ImGuiStyleValue<T>>
{
	using CompGetter = KVPairValuesGetter<T>;
	static float Get(const KVSection& section, int index) { return CompGetter::Get(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<ImGuiStyleValue<ImVec2>>
{
	using CompGetter = KVPairValuesGetter<float>;
	static ImVec2 Get(const KVSection& section, int index)
	{
		return ImVec2(CompGetter::Get(section, index), CompGetter::Get(section, index + 1));
	}
	static const int vcount = 2;
};

template<> struct KVPairValuesGetter<ImGuiStyleValue<ImVec4>>
{
	static ImVec4 Get(const KVSection& section, int index) { return ParseImGuiColorValue(section); }
	static const int vcount = 1;
};

#define IMGUI_STYLE_COLOR_LIST(X) \
	X(Text) \
	X(TextDisabled) \
	X(WindowBg) \
	X(ChildBg) \
	X(PopupBg) \
	X(Border) \
	X(BorderShadow) \
	X(FrameBg) \
	X(FrameBgHovered) \
	X(FrameBgActive) \
	X(TitleBg) \
	X(TitleBgActive) \
	X(TitleBgCollapsed) \
	X(MenuBarBg) \
	X(ScrollbarBg) \
	X(ScrollbarGrab) \
	X(ScrollbarGrabHovered) \
	X(ScrollbarGrabActive) \
	X(CheckMark) \
	X(SliderGrab) \
	X(SliderGrabActive) \
	X(Button) \
	X(ButtonHovered) \
	X(ButtonActive) \
	X(Header) \
	X(HeaderHovered) \
	X(HeaderActive) \
	X(Separator) \
	X(SeparatorHovered) \
	X(SeparatorActive) \
	X(ResizeGrip) \
	X(ResizeGripHovered) \
	X(ResizeGripActive) \
	X(Tab) \
	X(TabHovered) \
	X(TabActive) \
	X(TabUnfocused) \
	X(TabUnfocusedActive) \
	X(PlotLines) \
	X(PlotLinesHovered) \
	X(PlotHistogram) \
	X(PlotHistogramHovered) \
	X(TableHeaderBg) \
	X(TableBorderStrong) \
	X(TableBorderLight) \
	X(TableRowBg) \
	X(TableRowBgAlt) \
	X(TextSelectedBg) \
	X(DragDropTarget) \
	X(NavHighlight) \
	X(NavWindowingHighlight) \
	X(NavWindowingDimBg) \
	X(ModalWindowDimBg)

#define IMGUI_STYLE_FIELD_LIST(X) \
	X(Alpha, float) \
	X(DisabledAlpha, float) \
	X(WindowPadding, ImVec2) \
	X(WindowRounding, float) \
	X(WindowBorderSize, float) \
	X(WindowMinSize, ImVec2) \
	X(WindowTitleAlign, ImVec2) \
	X(WindowMenuButtonPosition, int) \
	X(ChildRounding, float) \
	X(ChildBorderSize, float) \
	X(PopupRounding, float) \
	X(PopupBorderSize, float) \
	X(FramePadding, ImVec2) \
	X(FrameRounding, float) \
	X(FrameBorderSize, float) \
	X(ItemSpacing, ImVec2) \
	X(ItemInnerSpacing, ImVec2) \
	X(CellPadding, ImVec2) \
	X(TouchExtraPadding, ImVec2) \
	X(IndentSpacing, float) \
	X(ColumnsMinSpacing, float) \
	X(ScrollbarSize, float) \
	X(ScrollbarRounding, float) \
	X(GrabMinSize, float) \
	X(GrabRounding, float) \
	X(LogSliderDeadzone, float) \
	X(TabRounding, float) \
	X(TabBorderSize, float) \
	X(TabMinWidthForCloseButton, float) \
	X(ColorButtonPosition, int) \
	X(ButtonTextAlign, ImVec2) \
	X(SelectableTextAlign, ImVec2) \
	X(DisplayWindowPadding, ImVec2) \
	X(DisplaySafeAreaPadding, ImVec2) \
	X(MouseCursorScale, float) \
	X(AntiAliasedLines, bool) \
	X(AntiAliasedLinesUseTex, bool) \
	X(AntiAliasedFill, bool) \
	X(CurveTessellationTol, float) \
	X(CircleTessellationMaxError, float)

#define IMGUI_STYLE_COLOR_DECLARE(name) ImGuiStyleValue<ImVec4> name{ &ImGui::GetStyle().Colors[ImGuiCol_##name] };
#define IMGUI_STYLE_COLOR_DESC(name) KV_DESC_FIELD(name)
#define IMGUI_STYLE_FIELD_DECLARE(name, type) ImGuiStyleValue<type> name{ &ImGui::GetStyle().name };
#define IMGUI_STYLE_FIELD_DESC(name, type) KV_DESC_FIELD(name)

#define IMGUI_STYLE_COLOR_COUNT(name) + 1
static_assert(IMGUI_STYLE_COLOR_LIST(IMGUI_STYLE_COLOR_COUNT) == ImGuiCol_COUNT, "ImGui color list is out of date");
#undef IMGUI_STYLE_COLOR_COUNT

struct ImGuiStyleColorsDesc
{
	DEFINE_KEYVALUES_DESC();

	IMGUI_STYLE_COLOR_LIST(IMGUI_STYLE_COLOR_DECLARE)
};

BEGIN_KEYVALUES_DESC(ImGuiStyleColorsDesc)
	IMGUI_STYLE_COLOR_LIST(IMGUI_STYLE_COLOR_DESC)
END_KEYVALUES_DESC

struct ImGuiStyleDesc
{
	DEFINE_KEYVALUES_DESC();

	ImGuiStyleColorsDesc Colors;

	IMGUI_STYLE_FIELD_LIST(IMGUI_STYLE_FIELD_DECLARE)
};

BEGIN_KEYVALUES_DESC(ImGuiStyleDesc)
	KV_DESC_EMBEDDED(Colors)
	IMGUI_STYLE_FIELD_LIST(IMGUI_STYLE_FIELD_DESC)
END_KEYVALUES_DESC

#undef IMGUI_STYLE_COLOR_DECLARE
#undef IMGUI_STYLE_COLOR_DESC
#undef IMGUI_STYLE_FIELD_DECLARE
#undef IMGUI_STYLE_FIELD_DESC
#undef IMGUI_STYLE_COLOR_LIST
#undef IMGUI_STYLE_FIELD_LIST

static void LoadImGuiStyle(const char* fileName)
{
	KVSection styleKvs;
	if (!KV_LoadFromFile(fileName, SP_DATA | SP_MOD, styleKvs))
		return;

	ImGuiStyleDesc styleDesc;
	KeyValues::ParseDesc(styleDesc, styleKvs);
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
#endif // IMGUI_ENABLED

void CEqImGuiHost::Init(EQWNDHANDLE window)
{
#ifdef IMGUI_ENABLED
	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImNodes::CreateContext();

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplMatSystem_Init();

	static const char* fontFileName = "resources/imgui/Inter-Regular.ttf";
	IFileStreamPtr fontFile = g_fileSystem->Open(fontFileName, FS_OPEN_READ, SP_DATA);
	if (fontFile)
	{
		const int length = fontFile->GetSize();
		char* buffer = (char*)IM_ALLOC(length);
		fontFile->Read(buffer, 1, length);
		fontFile = nullptr;

		ImGuiIO& io = ImGui::GetIO();

		ImFontConfig font_cfg;
		io.Fonts->AddFontFromMemoryTTF(buffer, length, 16.0f, &font_cfg, io.Fonts->GetGlyphRangesCyrillic());
	}

	LoadImGuiStyle("resources/imgui/style.txt");

	// TODO: ImGui networked console port (for Android)
	if(window)
	{
		ImGui_ImplEq_InitForSDL(window);
		m_sdlInit = true;
	}
#endif // IMGUI_ENABLED
}

void CEqImGuiHost::Shutdown()
{
#ifdef IMGUI_ENABLED
	ImGui_ImplMatSystem_Shutdown();
	if(m_sdlInit)
		ImGui_ImplEq_Shutdown();

	ImNodes::DestroyContext();
	ImGui::DestroyContext();

	m_sdlInit = false;
#endif // IMGUI_ENABLED
}

bool CEqImGuiHost::IsShown() const
{
	for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
	{
		if (it.value().enabled)
		{
			return true;
			break;
		}
	}
	return false;
}

void CEqImGuiHost::InputKeyPress(int key, bool down)
{
#ifdef IMGUI_ENABLED
	if (!m_sdlInit)
		return;
	ImGui_ImplEq_InputKeyPress(key, down);
#endif
}

void CEqImGuiHost::InputText(const char* keyInput)
{
#ifdef IMGUI_ENABLED
	if (!m_sdlInit)
		return;
	ImGui_ImplEq_InputText(keyInput);
#endif
}

void CEqImGuiHost::InputMousePress(int Button, bool pressed)
{
#ifdef IMGUI_ENABLED
	if (!m_sdlInit)
		return;
	ImGui_ImplEq_InputMousePress(Button, pressed);
#endif
}

void CEqImGuiHost::InputMouseWheel(int h, int v)
{
#ifdef IMGUI_ENABLED
	if (!m_sdlInit)
		return;
	ImGui_ImplEq_InputMouseWheel(h, v);
#endif
}

void CEqImGuiHost::BeginFrame(bool menuVisible)
{
#ifdef IMGUI_ENABLED
	if (!GImGui)
		return;

	const bool isReallyShown = menuVisible || IsShown();

	if (isReallyShown && m_sdlInit)
	{
		const bool anyItemShown = ImGui_ImplEq_AnyItemShown();

		// Start the Dear ImGui frame
		ImGui_ImplEq_NewFrame();
		if (anyItemShown)
		{
			ImGui_ImplEq_UpdateMousePosAndButtons();
			ImGui_ImplEq_UpdateGamepads();
		}
	}

	ImGui_ImplMatSystem_NewFrame();
	ImGui::NewFrame();
	m_imguiDrawStart = true;

	if (isReallyShown)
	{
		for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
		{
			Menu& handler = *it;
			handler.func(handler.enabled);
		}
	}

	if (!menuVisible)
		return;

	static bool s_showDemoWindow = false;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("ENGINE"))
		{
			if (ImGui::BeginMenu("FPS"))
			{
				IMGUI_MENUITEM_CONVAR_BOOL("SHOW FPS", r_showFPS);
				IMGUI_MENUITEM_CONVAR_BOOL("SHOW GRAPH", r_showFPSGraph);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::BeginMenu("EQUI"))
			{
				IMGUI_MENUITEM_CONVAR_BOOL("DEBUG RENDER", equi_debug);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("MATSYSTEM"))
			{
				IMGUI_MENUITEM_CONVAR_BOOL("OVERDRAW MODE", r_overdraw);
				IMGUI_MENUITEM_CONVAR_BOOL("WIREFRAME MODE", r_wireframe);
				ImGui::Separator();
				IMGUI_MENUITEM_CONCMD("RELOAD ALL MATERIALS", mat_reload, cmd_noArgs);
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("DEBUG OVERLAYS"))
		{
			IMGUI_MENUITEM_CONVAR_BOOL("SHOW FRAME STATS", r_debugDrawFrameStats);
			IMGUI_MENUITEM_CONVAR_BOOL("SHOW GRAPHS", r_debugDrawGraphs);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("IMGUI"))
		{
			ImGui::MenuItem("DEMO", nullptr, &s_showDemoWindow);
			ImGui::EndMenu();
		}

		for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
		{
			Menu& handler = *it;
			if (handler.path.Length())
				ImGuiBeginMenuPath(handler.path, handler.enabled);
		}

		ImGui::EndMainMenuBar();
	}

	if (s_showDemoWindow)
		ImGui::ShowDemoWindow(&s_showDemoWindow);

#undef IMGUI_CONVAR_BOOL
#endif // IMGUI_ENABLED
}

void CEqImGuiHost::EndFrame(int width, int height, IGPURenderPassRecorder* rendPassRecorder)
{
#ifdef IMGUI_ENABLED
	if (m_imguiDrawStart)
	{
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.DisplaySize = ImVec2((float)width, (float)height);

		// Rendering
		ImGui::EndFrame();

		ImGui::Render();
		ImGui_ImplMatSystem_RenderDrawData(ImGui::GetDrawData(), rendPassRecorder);
		m_imguiDrawStart = false;
	}
#endif // IMGUI_ENABLED
}

bool CEqImGuiHost::IsImGuiItemsInFocus() const
{
#ifdef IMGUI_ENABLED
	if (!GImGui)
		return false;
	ImGuiIO& io = ImGui::GetIO();
	return ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused() || (io.WantCaptureMouse || io.WantCaptureKeyboard);
#endif
	return false;
}

bool CEqImGuiHost::IsTextInputActive() const
{
#ifdef IMGUI_ENABLED
	if (!GImGui)
		return false;
	return ImGui::GetIO().WantTextInput;
#endif
	return false;
}

void CEqImGuiHost::AddDebugHandler(const char* name, IMGUI_HANDLER func)
{
#ifdef IMGUI_ENABLED
	ASSERT(func);
	const int nameHash = StringId24(name);
	Menu& handler = m_imguiMenus[nameHash];
	handler.func = func;
	handler.enabled = true; // non-menu are always enabled
#endif
}

void CEqImGuiHost::RemoveDebugHandler(const char* name)
{
#ifdef IMGUI_ENABLED
	const int nameHash = StringId24(name);
	m_imguiMenus.remove(nameHash);
#endif
}

void CEqImGuiHost::AddDebugMenu(const char* path, IMGUI_HANDLER func)
{
#ifdef IMGUI_ENABLED
	ASSERT(func);

	const int nameHash = StringId24(path);
	Menu& handler = m_imguiMenus[nameHash];
	handler.path = path;
	handler.func = func;
#endif
}

void CEqImGuiHost::ShowDebugMenu(const char* path, bool enable)
{
#ifdef IMGUI_ENABLED
	const int nameHash = StringId24(path);
	auto it = m_imguiMenus.find(nameHash);
	if (it.atEnd())
		return;
	(*it).enabled = enable;
#endif
}

void CEqImGuiHost::ToggleDebugMenu(const char* path)
{
#ifdef IMGUI_ENABLED
	const int nameHash = StringId24(path);
	auto it = m_imguiMenus.find(nameHash);
	if (it.atEnd())
		return;
	(*it).enabled = !(*it).enabled;
#endif
}