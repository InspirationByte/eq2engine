#pragma once

#include <webgpu/webgpu.h>

#define _WSTR(x) {(x), WGPU_STRLEN}

struct WGPUDeviceErrorContext
{
	WGPUDeviceErrorContext()
	{
		s_currentErrorDeviceContext = this;
	}
	~WGPUDeviceErrorContext()
	{
		s_currentErrorDeviceContext = nullptr;
	}

#ifndef _RETAIL
	static thread_local WGPUDeviceErrorContext* s_currentErrorDeviceContext;

	template<typename F>
	WGPUDeviceErrorContext(F onError)
		: onError(onError)
	{
		s_currentErrorDeviceContext = this;
	}
	using OnErrorFunc = EqFunction<void()>;
	OnErrorFunc onError;
#else
	template<typename F>
	WGPUCrashDebugContextDumper(F onError)
	{
		s_currentErrorDeviceContext = this;
	}
#endif

	bool hasError = false;
};