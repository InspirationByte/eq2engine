#pragma once

class IGPURenderPassRecorder;

#ifdef IMGUI_ENABLED

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

class CEqImGuiHost
{
public:
	using IMGUI_HANDLER = EqFunction<void(bool& visible)>;

	void			Initialize();
	void			Shutdown();

	bool			IsShown() const;
	void			BeginFrame(bool menuVisible);
	void			EndFrame(int width, int height, IGPURenderPassRecorder* rendPassRecorder);

	void			AddDebugHandler(const char* name, IMGUI_HANDLER func);
	void			AddDebugMenu(const char* path, IMGUI_HANDLER func);
	void			ShowDebugMenu(const char* path, bool enable);
	void			ToggleDebugMenu(const char* path);

	// removes both menus and handlers
	void			RemoveDebugHandler(const char* name);

	bool			IsImGuiItemsInFocus() const;

private:
	struct Menu
	{
		EqString		path;
		IMGUI_HANDLER	func;
		int				flags;
		bool			enabled{ false };
	};

	Map<int, Menu>			m_imguiMenus{ PP_SL };
	bool					m_imguiDrawStart{ false };
};

extern CEqImGuiHost* g_imGuiHost;