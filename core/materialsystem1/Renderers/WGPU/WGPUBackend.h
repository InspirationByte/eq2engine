#pragma once

#include <webgpu/webgpu.h>

#define _WSTR(x) {(x), WGPU_STRLEN}

struct WGPUDeviceErrorContext;
thread_local WGPUDeviceErrorContext* g_currentErrorDeviceContext;

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
	WGPUCrashDebugContextDumper(F onError)
	{
		g_currentErrorDeviceContext = this;
	}
#endif

	bool hasError = false;
};