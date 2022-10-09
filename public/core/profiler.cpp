//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Core profiling utilities
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

constexpr int PPSL_ADDR_BITS = 48;		// 64 bit arch only use 48 bits of pointers

PPSourceLine PPSourceLine::Empty()
{
	return {};
}

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
