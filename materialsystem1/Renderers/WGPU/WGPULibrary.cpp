//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include <webgpu/webgpu.h>

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

DECLARE_CVAR(wgpu_report_errors, "1", nullptr, 0);
DECLARE_CVAR(wgpu_break_on_error, "1", nullptr, 0);

static void OnWGPUDeviceError(WGPUErrorType /*type*/, const char* message, void*)
{
	if (wgpu_break_on_error.GetBool())
	{
		ASSERT_FAIL("WGPU device validation error:\n\n%s", message);
	}

	if (wgpu_report_errors.GetBool())
		MsgError("[WGPU] %s\n", message);

}

static void OnWGPUAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata)
{
	if (status != WGPURequestAdapterStatus_Success)
	{
		// cannot find adapter?
		ErrorMsg("%s", message);
	}
	else
	{
		// use first adapter provided
		WGPUAdapter* result = static_cast<WGPUAdapter*>(userdata);
		if (*result == nullptr)
			*result = adapter;
	}
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
	//WGPUInstanceDescriptor instDesc;
	// optionally use WGPUInstanceDescriptor::nextInChain for WGPUDawnTogglesDescriptor
	// with various toggles enabled or disabled: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Toggles.cpp

	m_instance = wgpuCreateInstance(nullptr);
	if (!m_instance)
		return false;

	return true;
}

IShaderAPI* CWGPURenderLib::GetRenderer() const
{
	return g_renderAPI;
}

bool CWGPURenderLib::InitAPI(const ShaderAPIParams& params)
{
	WGPUAdapter adapter = nullptr;
	WGPURequestAdapterOptions options{};
	//options.compatibleSurface = surface;
	wgpuInstanceRequestAdapter(m_instance, &options, &OnWGPUAdapterRequestEnded, &adapter);

	if (!adapter)
		return false;

	{
		WGPUAdapterProperties properties = { 0 };
		wgpuAdapterGetProperties(adapter, &properties);

		Msg("WGPU adapter: %s %s %s\n", properties.name, properties.driverDescription, properties.vendorName);
	}

	{
		WGPUSupportedLimits supLimits = { 0 };
		wgpuAdapterGetLimits(adapter, &supLimits);

		// extra features: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Features.cpp
		WGPUDeviceDescriptor desc{};
		WGPURequiredLimits reqLimits;
		reqLimits.limits = supLimits.limits;
		desc.requiredLimits = &reqLimits;

		m_rhiDevice = wgpuAdapterCreateDevice(adapter, &desc);
		if (!m_rhiDevice)
			return false;
	}

	wgpuDeviceSetUncapturedErrorCallback(m_rhiDevice, &OnWGPUDeviceError, nullptr);

	m_deviceQueue = wgpuDeviceGetQueue(m_rhiDevice);
	m_defaultSwapChain = PPNew CWGPUSwapChain(this, params.windowInfo);
	m_defaultSwapChain->SetBackbufferSize(800, 600);

	return true;
}

void CWGPURenderLib::ExitAPI()
{
	SAFE_DELETE(m_defaultSwapChain);
}

void CWGPURenderLib::BeginFrame(IEqSwapChain* swapChain)
{
	m_currentSwapChain = swapChain ? static_cast<CWGPUSwapChain*>(swapChain) : m_defaultSwapChain;

	// process all internal async events or error callbacks
	// this is dawn specific functionality, because in browser's WebGPU everything works with JS event loop
	wgpuDeviceTick(m_rhiDevice);
}

void CWGPURenderLib::EndFrame()
{
	CWGPUSwapChain* currentSwapChain = m_currentSwapChain;
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(currentSwapChain->m_swapChain);

	WGPURenderPassColorAttachment colorDesc = {};
	colorDesc.view = backBufView;
	colorDesc.loadOp = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;
	colorDesc.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };

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

IEqSwapChain* CWGPURenderLib::CreateSwapChain(void* window, bool windowed)
{
	return nullptr;
}

void CWGPURenderLib::DestroySwapChain(IEqSwapChain* swapChain)
{

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
