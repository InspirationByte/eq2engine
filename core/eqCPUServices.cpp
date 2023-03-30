//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: CPU detection
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "eqCPUServices.h"

EXPORTED_INTERFACE(IEqCPUCaps, CEqCPUCaps);

//----------------------------------------------------------------------------------------------------------

#ifdef _WIN32
#define CPUID __asm _emit 0x0F __asm _emit 0xA2
#define RDTSC __asm _emit 0x0F __asm _emit 0x31

#ifdef _WIN64
#include <intrin.h>

#define cpuid(func, a, b, c, d) { int res[4]; __cpuid(res, func); a = res[0]; b = res[1]; c = res[2]; d = res[3]; }

#define getCycleNumber() __rdtsc()

#else
void cpuidAsm(uint32 func, uint32 *a, uint32 *b, uint32 *c, uint32 *d);
#define cpuid(func, a, b, c, d) cpuidAsm(func, &a, &b, &c, &d)

#pragma warning(disable: 4035)
inline uint64 getCycleNumber()
{
	__asm {
		RDTSC
	}
}
#endif

#else

#if defined(__APPLE__)

void cpuidAsm(uint32 op, uint32 reg[4]);

#define cpuid(func, a, b, c, d) { uint32 res[4]; cpuidAsm(func, res); a = res[0]; b = res[1]; c = res[2]; d = res[3]; }

#elif defined(PLAT_LINUX) || defined(PLAT_ANDROID)

#if defined(__arm__)

#pragma todo("ARM cpuid, rdtsc")

#elif defined(__aarch64__)

#pragma todo("ARM64 cpuid, rdtsc")

#elif defined(__PIC__) && !defined(__x86_64__)

#define cpuid(in, a, b, c, d)   \
    asm volatile(               \
        "mov %%ebx, %%edi;"     \
        "cpuid;"                \
        "xchgl %%ebx, %%edi;" :  \
        "=a" (a),  \
        "=D" (b),  \
        "=c" (c),  \
        "=d" (d)   \
         : "a" (in) \
    );
#else

#define cpuid(in, a, b, c, d)   \
    asm volatile( "cpuid":      \
        "=a" (a),  \
        "=b" (b),  \
        "=c" (c),  \
        "=d" (d)   \
         : "a" (in) \
    );

#endif // __PIC__ && __x86_64__

#endif

#if defined(__arm__)

#define rdtsc(a, d) a=d=0;

#elif defined(__aarch64__)

#define rdtsc(a, d) a=d=0;

#else

#define rdtsc(a, d)\
  asm volatile ("rdtsc" : "=a" (a), "=d" (d));

#endif // !__arm__

inline uint64 getCycleNumber()
{
	union {
		struct {
			unsigned int a, d;
		};
		long long ad;
	} ads;

	rdtsc(ads.a, ads.d);
	return ads.ad;
}

#endif

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

uint64 getHz()
{
	LARGE_INTEGER t1, t2, tf;
	uint64 c1, c2;

	QueryPerformanceFrequency(&tf);
	QueryPerformanceCounter(&t1);
	c1 = getCycleNumber();

	// Some spin-wait
	for (volatile int i = 0; i < 1000000; i++);

	QueryPerformanceCounter(&t2);
	c2 = getCycleNumber();

	return ((c2 - c1) * tf.QuadPart / (t2.QuadPart - t1.QuadPart));
}

#ifndef _WIN64
void cpuidAsm(uint32 func, uint32 *a, uint32 *b, uint32 *c, uint32 *d)
{
	__asm {
		PUSH	EAX
		PUSH	EBX
		PUSH	ECX
		PUSH	EDX

		MOV		EAX, func
		CPUID
		MOV		EDI, a
		MOV		[EDI], EAX
		MOV		EDI, b
		MOV		[EDI], EBX
		MOV		EDI, c
		MOV		[EDI], ECX
		MOV		EDI, d
		MOV		[EDI], EDX

		POP		EDX
		POP		ECX
		POP		EBX
		POP		EAX
	}
}
#endif

#else

#include <unistd.h>
#include <sys/time.h>

uint64 getHz()
{
	static struct timeval t1, t2;
	static struct timezone tz;
	unsigned long long c1,c2;

	gettimeofday(&t1, &tz);
	c1 = getCycleNumber();

	// Some spin-wait
	for (volatile int i = 0; i < 1000000; i++);

	gettimeofday(&t2, &tz);
	c2 = getCycleNumber();

	return (1000000 * (c2 - c1)) / ((t2.tv_usec - t1.tv_usec) + 1000000 * (t2.tv_sec - t1.tv_sec));
}

#if defined(__APPLE__)
void cpuidAsm(uint32 op, uint32 reg[4]){
	asm volatile(
		"pushl %%ebx      \n\t"
		"cpuid            \n\t"
		"movl %%ebx, %1   \n\t"
		"popl %%ebx       \n\t"
		: "=a"(reg[0]), "=r"(reg[1]), "=c"(reg[2]), "=d"(reg[3])
		: "a"(op)
		: "cc"
	);
}
#endif

#endif

//----------------------------------------------------------------------------------------------------------

CEqCPUCaps::CEqCPUCaps()
{
	m_cpuCount		= 0;
	m_cpuCMOV		= false;
	m_cpuMMX		= false;
	m_cpuSSE		= false;
	m_cpuSSE2		= false;
	m_cpuSSE3		= false;
	m_cpuSSSE3		= false;
	m_cpuSSE41		= false;
	m_cpuSSE42		= false;
	m_cpu3DNow		= false;
	m_cpu3DNowExt	= false;
	m_cpuMMXExt		= false;
	m_cpuFamily		= 0;

	g_eqCore->RegisterInterface(CPUSERVICES_INTERFACE_VERSION, this);
}

void CEqCPUCaps::Init()
{
	Msg("\n-------- CPU Init --------\n\n");

#if defined(_WIN32)
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_cpuCount = sysInfo.dwNumberOfProcessors;

#elif defined(PLAT_LINUX) || defined(PLAT_ANDROID)
	//m_cpuCount = sysconf(_SC_NPROCESSORS_CONF);
	m_cpuCount = sysconf(_SC_NPROCESSORS_ONLN);

#elif defined(__APPLE__)

	// TODO: Fix ...
	m_cpuCount = 1;	// sysconf(_SC_NPROCESSORS_ONLN);

#endif

    MsgInfo("[CPU] Number of cores: %d\n", m_cpuCount);

	// Just in case ...
	if (m_cpuCount < 1)
		m_cpuCount = 1;

	uint32 maxi, maxei, a, b, c, d;

	m_cpuVendor[12]   = '\0';
	m_cpuBrandName[0] = '\0';

#ifndef PLAT_ANDROID
	cpuid(0, maxi, ((uint32 *) m_cpuVendor)[0], ((uint32 *) m_cpuVendor)[2], ((uint32 *) m_cpuVendor)[1]);

	if (maxi >= 1)
	{
		cpuid(1, a, b, c, d);

		//EDX
		m_cpuCMOV		= (d & 0x00008000) != 0;
		m_cpuMMX		= (d & 0x00800000) != 0;
		m_cpuSSE		= (d & 0x02000000) != 0;
		m_cpuSSE2		= (d & 0x04000000) != 0;
		m_cpuSSE3		= (d & 0x00000001) != 0;

		//ECX
		m_cpuSSSE3	= (c & 0x00000200) != 0;
		m_cpuSSE41	= (c & 0x00080000) != 0;
		m_cpuSSE42	= (c & 0x00100000) != 0;

		m_cpuFamily	= (a >> 8) & 0x0F;

		cpuid(0x80000000, maxei, b, c, d);
		if (maxei >= 0x80000001)
		{
			cpuid(0x80000001, a, b, c, d);
			m_cpu3DNow    = (d & 0x80000000) != 0;
			m_cpu3DNowExt = (d & 0x40000000) != 0;
			m_cpuMMXExt   = (d & 0x00400000) != 0;

			if (maxei >= 0x80000002)
			{
				cpuid(0x80000002, ((uint32 *) m_cpuBrandName)[0], ((uint32 *) m_cpuBrandName)[1], ((uint32 *) m_cpuBrandName)[2], ((uint32 *) m_cpuBrandName)[3]);
				m_cpuBrandName[16] = '\0';

				if (maxei >= 0x80000003)
				{
					cpuid(0x80000003, ((uint32 *) m_cpuBrandName)[4], ((uint32 *) m_cpuBrandName)[5], ((uint32 *) m_cpuBrandName)[6], ((uint32 *) m_cpuBrandName)[7]);
					m_cpuBrandName[32] = '\0';

					if (maxei >= 0x80000004)
					{
						cpuid(0x80000004, ((uint32 *) m_cpuBrandName)[8], ((uint32 *) m_cpuBrandName)[9], ((uint32 *) m_cpuBrandName)[10], ((uint32 *) m_cpuBrandName)[11]);
						m_cpuBrandName[48] = '\0';
					}
				}
			}
		}
		MsgInfo("[CPU] %s %s\n", m_cpuVendor, m_cpuBrandName);

		MsgInfo("[CPU] Features: %s%s%s%s%s%s%s%s%s%s%s\n",
			m_cpuCMOV ? "CMOV " : "",
			m_cpuMMX ? "MMX " : "",
			m_cpuMMXExt ? "MMX+ " : "",
			m_cpu3DNow ? "3DNow " : "",
			m_cpu3DNowExt ? "3DNow+ " : "",
			m_cpuSSE ? "SSE " : "",
			m_cpuSSE2 ? "SSE2 " : "",
			m_cpuSSE3 ? "SSE3 " : "",
			m_cpuSSSE3 ? "SSSE3 " : "",
			m_cpuSSE41 ? "SSE4.1 " : "",
			m_cpuSSE42 ? "SSE 4.2 " : "");
	}
#endif // PLAT_ANDROID
}

// CPU frequency in Hz
uint64 CEqCPUCaps::GetFrequency() const
{
	return getHz();
}
