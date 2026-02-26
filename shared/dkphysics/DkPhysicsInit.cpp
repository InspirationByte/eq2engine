#include "core/core_common.h"

#include "DkJoltPCH.h"
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "DkPhysicsWorld.h"

DECLARE_CVAR(ph_jobThreads, "2", "Physics job threads", CV_ARCHIVE);
DECLARE_CVAR(ph_tempMemAllocMB, "10", "Temp allocator memory", CV_CHEAT);

#define jphPP_SL PPSourceLine::Make("JoltPhysics", 0)

static JPH::JobSystemThreadPool* s_jphJobThreadPool = nullptr;
static JPH::TempAllocatorImpl* s_jphTempAlloc = nullptr;

static bool jphAssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	static bool physIgnoreAsserts = false;
	const int assertResult = _InternalAssertMsg(PPSourceLine::Make(inFile, inLine), physIgnoreAsserts, inExpression, inMessage);
	if (assertResult == _EQASSERT_IGNORE_ALWAYS)
		physIgnoreAsserts = true;
	return assertResult == _EQASSERT_BREAK;
};

static void jphTraceImpl(const char *inFMT, ...)
{
	va_list list;
	va_start(list, inFMT);
	LogMsgV(SPEW_INFO, inFMT, list);
	va_end(list);
}

static void *jphAlloc(size_t inSize)
{
	return PPDAlloc(inSize, jphPP_SL);
}

static void *jphRealloc(void *inBlock, size_t inOldSize, size_t inNewSize)
{
	return PPDReAlloc(inBlock, inNewSize, jphPP_SL);
}

static void jphFree(void *inBlock)
{
	PPFree(inBlock);
}

static void *jphAlignedAlloc(size_t inSize, size_t inAlignment)
{
	return PPDAlignedAlloc(inSize, inAlignment, jphPP_SL);
}

static void jphAlignedFree(void *inBlock)
{
	PPAlignedFree(inBlock);
}

static DkPhysics s_dkPhysics;

JPH::JobSystem* GetJoltJobSystem() { return s_jphJobThreadPool; }
JPH::TempAllocator* GetJoltTempAlloc() { return s_jphTempAlloc; }

void dkPhysicsLibInit()
{
	GetIDkCoreImpl()->RegisterInterface(&s_dkPhysics);

	using namespace JPH;
#ifdef JPH_ENABLE_ASSERTS
	AssertFailed = jphAssertFailedImpl;
#endif
	Trace = jphTraceImpl;
#ifndef JPH_DISABLE_CUSTOM_ALLOCATOR
	Allocate = jphAlloc;
	Reallocate = jphRealloc;
	Free = jphFree;
	AlignedAllocate = jphAlignedAlloc;
	AlignedFree = jphAlignedFree;
#endif

	Factory::sInstance = new Factory();
	RegisterTypes();

	s_jphJobThreadPool = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, ph_jobThreads.GetInt());
	s_jphTempAlloc = new TempAllocatorImpl(ph_tempMemAllocMB.GetInt() * 1024 * 1024);
}

void dkPhysicsLibShutdown()
{
	using namespace JPH;

	SAFE_DELETE(s_jphTempAlloc);
	SAFE_DELETE(s_jphJobThreadPool);
	SAFE_DELETE(Factory::sInstance);

#ifdef JPH_ENABLE_ASSERTS
	AssertFailed = nullptr;
#endif
	UnregisterTypes();

	GetIDkCoreImpl()->UnregisterInterface<DkPhysics>();
}