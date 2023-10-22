//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////
#include <dawn/dawn_proc.h>
#include <webgpu/webgpu_cpp.h>

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "WGPUBackend.h"

#include "WGPULibrary.h"
#include "WGPURenderAPI.h"
#include "WGPUSwapChain.h"

HOOK_TO_CVAR(r_screen);

WGPURenderAPI s_renderApi;
IShaderAPI* g_renderAPI = &s_renderApi;

#pragma optimize("", off)

DECLARE_CVAR(wgpu_report_errors, "0", nullptr, 0);
DECLARE_CVAR(wgpu_break_on_error, "1", nullptr, 0);

static void DawnErrorHandler(WGPUErrorType /*type*/, const char* message, void*)
{
	if (wgpu_break_on_error.GetBool())
	{
		_DEBUG_BREAK;
	}

	if(wgpu_report_errors.GetBool())
		MsgError("[WGPU] %s\n", message);
}

static dawn::native::Adapter RequestWGPUAdapter(WGPUBackendType type1st, WGPUBackendType type2nd = WGPUBackendType_Null)
{
	static dawn::native::Instance instance;
	instance.DiscoverDefaultAdapters();

	wgpu::AdapterProperties properties;
	std::vector<dawn::native::Adapter> adapters = instance.GetAdapters();
	for (auto it = adapters.begin(); it != adapters.end(); ++it)
	{
		it->GetProperties(&properties);
		if (static_cast<WGPUBackendType>(properties.backendType) == type1st)
			return *it;
	}
	if (type2nd > WGPUBackendType_Null)
	{
		for (auto it = adapters.begin(); it != adapters.end(); ++it)
		{
			it->GetProperties(&properties);
			if (static_cast<WGPUBackendType>(properties.backendType) == type2nd)
				return *it;
		}
	}
	return dawn::native::Adapter();
}

static WGPUDevice CreateDevice(WGPUBackendType& backendType)
{
	if (backendType > WGPUBackendType_OpenGLES) {
#ifdef DAWN_ENABLE_BACKEND_VULKAN
		backendType = WGPUBackendType_Vulkan;
#elif defined(DAWN_ENABLE_BACKEND_D3D12)
		backendType = WGPUBackendType_D3D12;
#else
#endif
	}
	
	dawn::native::Adapter adapter = RequestWGPUAdapter(backendType);
	if (!adapter)
		return nullptr;

	wgpu::AdapterProperties properties;
	adapter.GetProperties(&properties);

	backendType = static_cast<WGPUBackendType>(properties.backendType);
	return adapter.CreateDevice();
}

CWGPURenderLib::CWGPURenderLib()
{
	m_windowed = true;
}

CWGPURenderLib::~CWGPURenderLib()
{
}

bool CWGPURenderLib::InitCaps()
{
	WGPUBackendType backendType = WGPUBackendType_Force32;
	WGPUDevice rhiDevice = CreateDevice(backendType);
	if (!rhiDevice)
		return false;
	m_backendType = backendType;
	m_rhiDevice = rhiDevice;

	DawnProcTable procs(dawn::native::GetProcs());
	procs.deviceSetUncapturedErrorCallback(m_rhiDevice, DawnErrorHandler, nullptr);

	dawnProcSetProcs(&procs);

	return true;
}

IShaderAPI* CWGPURenderLib::GetRenderer() const
{
	return g_renderAPI;
}

bool CWGPURenderLib::InitAPI(const ShaderAPIParams &params)
{
	m_window = params.windowInfo.get(RenderWindowInfo::WINDOW);

	m_deviceQueue = wgpuDeviceGetQueue(m_rhiDevice);

	m_defaultSwapChain = PPNew CWGPUSwapChain(this, m_window);

	return true;
}

void CWGPURenderLib::ExitAPI()
{
	SAFE_DELETE(m_defaultSwapChain);
}

void CWGPURenderLib::BeginFrame(IEqSwapChain* swapChain)
{
	m_currentSwapChain = swapChain ? static_cast<CWGPUSwapChain*>(swapChain) : m_defaultSwapChain;
}

void CWGPURenderLib::EndFrame()
{
	CWGPUSwapChain* currentSwapChain = m_currentSwapChain;
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(currentSwapChain->m_swapChain);

	WGPURenderPassColorAttachment colorDesc = {};
	colorDesc.view = backBufView;
	colorDesc.loadOp = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;

	// Dawn has both clearValue/clearColor but only Color works; Emscripten only has Value
	colorDesc.clearColor = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

	// my first command buffer
	{
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(m_rhiDevice, nullptr);
		{
			WGPURenderPassDescriptor renderPass = {};
			renderPass.colorAttachmentCount = 1;
			renderPass.colorAttachments = &colorDesc;

			WGPURenderPassEncoder renderPassEnc = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass);	
			wgpuRenderPassEncoderEnd(renderPassEnc);
			wgpuRenderPassEncoderRelease(renderPassEnc);
		}
		{
			WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
			wgpuCommandEncoderRelease(encoder);

			wgpuQueueSubmit(m_deviceQueue, 1, &commands);
			wgpuCommandBufferRelease(commands);	
		}
	}

	wgpuTextureViewRelease(backBufView);

	currentSwapChain->SwapBuffers();
}

void CWGPURenderLib::SetBackbufferSize(const int w, const int h)
{
	m_defaultSwapChain->SetBackbufferSize(w, h);
}

// changes fullscreen mode
bool CWGPURenderLib::SetWindowed(bool enabled)
{
	m_windowed = enabled;
	return true;
}

// speaks for itself
bool CWGPURenderLib::IsWindowed() const
{
	return m_windowed;
}

bool CWGPURenderLib::CaptureScreenshot(CImage &img)
{
	return false;
}
