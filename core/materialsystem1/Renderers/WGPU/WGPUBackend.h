#pragma once

#include <webgpu/webgpu.h>

#define _WSTR(x) {(x), WGPU_STRLEN}

struct WGPUDeviceErrorContext;
extern thread_local WGPUDeviceErrorContext* g_currentErrorDeviceContext;

struct WGPUDeviceErrorContext
{
	WGPUDeviceErrorContext()
	{
		g_currentErrorDeviceContext = this;
	}
	~WGPUDeviceErrorContext()
	{
		g_currentErrorDeviceContext = nullptr;
	}

#ifndef _RETAIL
	template<typename F>
	WGPUDeviceErrorContext(F onError)
		: onError(onError)
	{
		g_currentErrorDeviceContext = this;
	}
	using OnErrorFunc = EqFunction<void()>;
	OnErrorFunc onError;
#else
	template<typename F>
	WGPUDeviceErrorContext(F onError)
	{
		g_currentErrorDeviceContext = this;
	}
#endif

	bool hasError = false;
};

static const char* GetWGPUBackendTypeStr(WGPUBackendType backendType)
{
	switch (backendType)
	{
	case WGPUBackendType_D3D11:
		return "D3D11";
	case WGPUBackendType_D3D12:
		return "D3D12";
	case WGPUBackendType_Metal:
		return "Metal";
	case WGPUBackendType_Vulkan:
		return "Vulkan";
	case WGPUBackendType_OpenGL:
		return "OpenGL";
	case WGPUBackendType_OpenGLES:
		return "OpenGLES";
	}
	return "Unknown";
}