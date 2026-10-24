//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - Equilibrium Graphics File viewer on sysLib
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IEqParallelJobs.h"
#include "core/ICommandLine.h"
#include "core/ConVar.h"

#include "sys/sys_host.h"
#include "imgui_backend/imgui_host.h"
#include "studio/StudioCache.h"
#include "dkphysics/DkPhysicsInit.h"

#include "egfman2_filedialog.h"
#include "egfman2_viewer.h"

#define EGFMAN2_WINDOW_TITLE	"EGFMan2 - Entity Geometry File viewer"
#define EGFMAN2_USER_CFG		"cfg/egfman2.cfg"

extern ConVar user_cfg;

namespace eqAppStateMng
{
static CAppStateBase* s_appStates[APP_STATE_COUNT] = { nullptr };

const char* GetAppNameTitle()
{
	return EGFMAN2_WINDOW_TITLE;
}

CAppStateBase* GetAppStateByType(int stateType)
{
	return s_appStates[stateType];
}

bool InitAppStates()
{
	// don't overwrite game user configuration on exit
	if (g_cmdLine->Find("-user_cfg") == -1)
		user_cfg.SetValue(EGFMAN2_USER_CFG);

	s_appStates[APP_STATE_VIEWER] = g_stateViewer;

	dkPhysicsLibInit();
	g_studioCache->Init(g_parallelJobs->GetJobMng());

	FileDialogInit();

	g_imGuiHost->AddDebugHandler("EGFMan2", [](bool& visible) {
		g_stateViewer->DrawUI(visible);
	});

	eqAppStateMng::SetCurrentStateType(APP_STATE_VIEWER);

	return true;
}

void ShutdownAppStates()
{
	for (int i = 0; i < APP_STATE_COUNT; ++i)
		s_appStates[i] = nullptr;

	g_imGuiHost->RemoveDebugHandler("EGFMan2");

	FileDialogShutdown();

	g_studioCache->Shutdown();
	dkPhysicsLibShutdown();
	g_parallelJobs->Shutdown();
}

bool IsPauseAllowed()
{
	return false;
}

void SignalPause()
{
}

}
