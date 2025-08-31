
#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"

#include "RenderManager.h"
#include "renderers/IShaderAPI.h"

#define RHI_NULL      0
#define RHI_NVRHI     1
#define RHI_WGPU      2

#if RENDERER_TYPE == RHI_NULL

#include "Empty/emptyLibrary.h"
static CEmptyRenderLib  s_EmptyRenderLib;

#elif RENDERER_TYPE == RHI_NVRHI

#ifdef _WIN32
#include "NVRHI/NVRHILibraryD3D11.h"
#include "NVRHI/NVRHILibraryD3D12.h"
static CNVRHIRenderLibD3D11  s_NVRHIRenderLibD3D11;
static CNVRHIRenderLibD3D12  s_NVRHIRenderLibD3D12;
#endif

#include "NVRHI/NVRHILibraryVK.h"
static CNVRHIRenderLibVK	s_NVRHIRenderLibVK;

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

IRenderLibrary* CEqRenderManager::CreateRenderer(const ShaderAPIParams& params) const
{
#if RENDERER_TYPE == RHI_NULL
	s_currentRenderLib = &s_EmptyRenderLib;
	return s_currentRenderLib;
#elif RENDERER_TYPE == RHI_NVRHI
	s_currentRenderLib = &s_NVRHIRenderLibD3D11;
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