//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) -  memory allocation tracker
//
//				See ppmem_core.cpp for main implementation
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"

// FIXME: ASAN settings are temporarily here
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#ifdef __cplusplus
extern "C"
#endif
const char *__asan_default_options() {
  // Clang reports ODR Violation errors in mbedtls/library/certs.c.
  // NEED TO REPORT THIS ISSUE
  return "detect_odr_violation=0:alloc_dealloc_mismatch=0";
}
#endif

#if !defined(NO_PPMEM_OP) && !defined(PPMEM_DISABLE)

void* operator new(size_t size)
{
	return malloc(size);
}

void* operator new(size_t size, size_t alignment)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* ptr) PPNOEXCEPT
{
	PPFree(ptr);
}

void operator delete(void* ptr, size_t alignment) PPNOEXCEPT
{
	PPFree(ptr);
}

void operator delete[](void* ptr) PPNOEXCEPT
{
	PPFree(ptr);
}

void* operator new(size_t size, PPSourceLine sl) PPNOEXCEPT
{
	return PPDAlloc(size, sl);
}

void* operator new[](size_t size, PPSourceLine sl) PPNOEXCEPT
{
	return PPDAlloc(size, sl);
}

void operator delete(void* ptr, PPSourceLine sl) PPNOEXCEPT
{
	PPFree(ptr);
}

void operator delete[](void* ptr, PPSourceLine sl) PPNOEXCEPT
{
	PPFree(ptr);
}

#endif // !NO_PPMEM_OP && !PPMEM_DISABLE