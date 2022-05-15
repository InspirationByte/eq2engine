//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) - a C++ memory allocation debugger
//				designed to detect memory leaks and allocation errors
//
//				See ppmem_core.cpp for main implementation
//////////////////////////////////////////////////////////////////////////////////

#include "core/ppmem.h"
#include <malloc.h>

constexpr int PPSL_ADDR_BITS = 48;		// 64 bit arch only use 48 bits of pointers

PPSourceLine PPSourceLine::Make(const char* filename, int line)
{
	return { (uint64(filename) | uint64(line) << PPSL_ADDR_BITS) };
}

const char* PPSourceLine::GetFileName() const
{
	return (const char*)(data & ((1ULL << PPSL_ADDR_BITS) - 1)); 
}

int	PPSourceLine::GetLine() const 
{
	return int((data >> PPSL_ADDR_BITS) & ((1 << 16) - 1));
}

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

void operator delete(void* ptr)
{
	PPFree(ptr);
}

void operator delete(void* ptr, size_t alignment)
{
	PPFree(ptr);
}

void operator delete[](void* ptr)
{
	PPFree(ptr);
}

void* operator new(size_t size, PPSourceLine sl)
{
	return PPDAlloc(size, sl);
}

void* operator new[](size_t size, PPSourceLine sl)
{
	return PPDAlloc(size, sl);
}

void operator delete(void* ptr, PPSourceLine sl)
{
	PPFree(ptr);
}

void operator delete[](void* ptr, PPSourceLine sl)
{
	PPFree(ptr);
}