//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq Engine CPU services
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "cpuInstrDefs.h"

#define CPUSERVICES_INTERFACE_VERSION		"CORE_CPUCaps_001"

//
// CPU information interface
//
class IEqCPUCaps : public IEqCoreModule
{
public:
						~IEqCPUCaps() {}

	// CPU frequency in Hz
	virtual	uint64		GetFrequency() const = 0;

	virtual	int			GetCPUCount() const = 0;

	virtual	int			GetCPUFamily() const = 0;
	virtual	const char*	GetCPUVendor() const = 0;
	virtual	const char*	GetBrandName() const = 0;

	
//----------------------------------------------------------
// instruction set capabilities

	virtual	bool		IsCPUHasCMOV() const = 0;

	// mostly 3dnow and 3dnow+ dropped
	virtual	bool		IsCPUHas3DNow() const = 0;
	virtual	bool		IsCPUHas3DNowExt() const = 0;

	//
	// all modern processors has support
	//

	virtual	bool		IsCPUHasMMX() const = 0;
	virtual	bool		IsCPUHasMMXExt() const = 0;

	virtual	bool		IsCPUHasSSE() const = 0;
	virtual	bool		IsCPUHasSSE2() const = 0;
	virtual	bool		IsCPUHasSSE3() const = 0;
	virtual	bool		IsCPUHasSSSE3() const = 0;
	virtual	bool		IsCPUHasSSE41() const = 0;
	virtual	bool		IsCPUHasSSE42() const = 0;
};

INTERFACE_SINGLETON( IEqCPUCaps, CEqCPUCaps, CPUSERVICES_INTERFACE_VERSION, g_cpuCaps )