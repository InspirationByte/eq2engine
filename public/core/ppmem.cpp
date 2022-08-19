//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: PPMem (Pee-Pee Memory) -  memory allocation tracker
//
//				See ppmem_core.cpp for main implementation
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"

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