
#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"

#include "RenderManager.h"
#include "renderers/IShaderAPI.h"

#define RHI_NULL      0
#define RHI_WGPU      4

#if RENDERER_TYPE == RHI_NULL

#include "Empty/emptyLibrary.h"
static CEmptyRenderLib  s_EmptyRenderLib;
#elif RENDERER_TYPE == RHI_WGPU

#include "WGPU/WGPULibrary.h"
static CWGPURenderLib  s_WGPURenderLib;
#endif

static CEqRenderManager g_renderManager;
static IRenderLibrary* s_currentRenderLib = nullptr;

CEqRenderManager::CEqRenderManager()
{
    g_eqCore->RegisterInterface(this);
}

CEqRenderManager::~CEqRenderManager()
{
	g_eqCore->UnregisterInterface<CEqRenderManager>();
}

IRenderLibrary* CEqRenderManager::CreateRenderer(const ShaderAPIParams &params) const
{
#if RENDERER_TYPE == RHI_NULL
    s_currentRenderLib = &s_EmptyRenderLib;
    return s_currentRenderLib;
#elif RENDERER_TYPE == RHI_WGPU
    s_currentRenderLib = &s_WGPURenderLib;
    return s_currentRenderLib;
#endif
    return s_currentRenderLib;
}

IRenderLibrary* CEqRenderManager::GetRenderer() const
{
    return s_currentRenderLib;
}