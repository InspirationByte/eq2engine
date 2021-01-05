//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: CPU macros and detector
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQCPUSERVICES_H
#define EQCPUSERVICES_H

#include "IEqCPUServices.h"
#include "platform/Platform.h"

class CEqCPUCaps : public IEqCPUCaps
{
public:
				CEqCPUCaps();

	void		Init();

	// CPU frequency in Hz
	uint64		GetFrequency() const;

	int			GetCPUCount() const {return m_cpuCount;}

	int			GetCPUFamily() const {return m_cpuFamily;}
	const char*	GetCPUVendor() const {return m_cpuVendor;}
	const char*	GetBrandName() const {return m_cpuBrandName;}

	
//----------------------------------------------------------
// instruction set capabilities

	bool		IsCPUHasCMOV() const		{return m_cpuCMOV;}

	// mostly 3dnow and 3dnow+ dropped
	bool		IsCPUHas3DNow() const		{return m_cpu3DNow;}
	bool		IsCPUHas3DNowExt() const	{return m_cpu3DNowExt;}

	//
	// all modern processors has support
	//
	bool		IsCPUHasMMX() const		{return m_cpuMMX;}
	bool		IsCPUHasMMXExt() const	{return m_cpuMMXExt;}

	bool		IsCPUHasSSE() const		{return m_cpuSSE;}
	bool		IsCPUHasSSE2() const	{return m_cpuSSE2;}
	bool		IsCPUHasSSE3() const	{return m_cpuSSE3;}
	bool		IsCPUHasSSSE3() const	{return m_cpuSSSE3;}
	bool		IsCPUHasSSE41() const	{return m_cpuSSE41;}
	bool		IsCPUHasSSE42() const	{return m_cpuSSE42;}

	bool		IsInitialized() const	{return m_cpuCount && m_cpuVendor[0] != '\0';}
	const char*	GetInterfaceName() const {return CPUSERVICES_INTERFACE_VERSION;}

protected:

	int m_cpuCount;
	int m_cpuFamily;

	char m_cpuVendor[13];
	char m_cpuBrandName[49];

	bool m_cpuCMOV;
	bool m_cpu3DNow;
	bool m_cpu3DNowExt;
	bool m_cpuMMX;
	bool m_cpuMMXExt;
	bool m_cpuSSE;
	bool m_cpuSSE2;
	bool m_cpuSSE3;
	bool m_cpuSSSE3;
	bool m_cpuSSE41;
	bool m_cpuSSE42;
};

#endif // EQCPUSERVICES_H
