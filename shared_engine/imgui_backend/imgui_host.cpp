#include "core/core_common.h"
#include "imgui_host.h"

#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#ifdef IMGUI_ENABLED
#include <imgui.h>
#include <imnodes.h>

#include "imgui_backend/imgui_impl_matsystem.h"
#include "imgui_internal.h"
#endif // IMGUI_ENABLED

static CEqImGuiHost s_imGuiHost;
CEqImGuiHost* g_imGuiHost = &s_imGuiHost;

#ifdef IMGUI_ENABLED
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

void CEqImGuiHost::Initialize()
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
	ImGui_ImplMatSystem_Init();
#endif // IMGUI_ENABLED
}

void CEqImGuiHost::Shutdown()
{
#ifdef IMGUI_ENABLED
	ImGui_ImplMatSystem_Shutdown();

	ImNodes::DestroyContext();
	ImGui::DestroyContext();
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

void CEqImGuiHost::BeginFrame(bool menuVisible)
{
#ifdef IMGUI_ENABLED
	if (menuVisible || IsShown())
	{
		ImGui_ImplMatSystem_NewFrame();
		ImGui::NewFrame();
		for (auto it = m_imguiMenus.begin(); !it.atEnd(); ++it)
		{
			Menu& handler = *it;
			handler.func(handler.enabled);
		}
		m_imguiDrawStart = true;
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
			IMGUI_MENUITEM_CONVAR_BOOL("SHOW 3D SHAPES", r_debugDrawShapes);
			IMGUI_MENUITEM_CONVAR_BOOL("SHOW 3D LINES", r_debugDrawLines);

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

		//static bool show_demo_window = true;
		//ImGui::ShowDemoWindow(&show_demo_window);

		// Rendering
		ImGui::EndFrame();

		ImGui::Render();
		ImGui_ImplMatSystem_RenderDrawData(ImGui::GetDrawData(), rendPassRecorder);
		m_imguiDrawStart = false;
	}
#endif // IMGUI_ENABLED
}

#ifdef IMGUI_ENABLED
bool ImGui_ImplEq_AnyItemShown()
{
	ImGuiContext& ctx = *ImGui::GetCurrentContext();
	return ctx.WindowsActiveCount > 1 || ctx.BeginMenuCount > 0 || ctx.BeginPopupStack.size() || ctx.OpenPopupStack.size();
}

bool ImGui_ImplEq_AnyWindowInFocus()
{

	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureMouse || io.WantCaptureKeyboard;
}
#endif

bool CEqImGuiHost::IsImGuiItemsInFocus() const
{
#ifdef IMGUI_ENABLED
	return ImGui::IsAnyItemHovered() || ImGui::IsAnyItemActive() || ImGui::IsAnyItemFocused() || ImGui_ImplEq_AnyWindowInFocus();
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