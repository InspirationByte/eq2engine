#pragma once

#if __has_include("d3d12.h") || (_MSC_VER >= 1900)
#	define DAWN_ENABLE_BACKEND_D3D12
#endif
#if __has_include("vulkan/vulkan.h") && (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
#	define DAWN_ENABLE_BACKEND_VULKAN
#endif

#include <dawn/native/NullBackend.h>
#ifdef DAWN_ENABLE_BACKEND_D3D12
#	include <dawn/native/D3D12Backend.h>
#endif
#ifdef DAWN_ENABLE_BACKEND_VULKAN
#	include <dawn/native/VulkanBackend.h>
#endif

