#include <gtest/gtest.h>
#include "core/core_common.h"
#include "core/IDkCore.h"

#include "tests_common.h"

static int TestsAssertHandler(PPSourceLine sl, const char* expression, const char* message, bool skipOnly)
{
	GTEST_MESSAGE_AT_(sl.GetFileName(), sl.GetLine(), message, ::testing::TestPartResult::kFatalFailure) << expression;
	return _EQASSERT_SKIP;
}

TestAppWrapper::~TestAppWrapper()
{
	if (m_enableCore)
		GetIDkCoreImpl()->Shutdown();

	SetAssertHandler(m_oldAssertHandler);
}

TestAppWrapper::TestAppWrapper(bool enableCore, const char* name, int argc, char** argv)
	: m_enableCore(enableCore)
{	
	if(m_enableCore)
	{
		CoreAppInitParameters coreParams;
		coreParams.appName = name;
		coreParams.commandLine = ArrayCRef(argv, argc);
		GetIDkCoreImpl()->Init(coreParams);
	}

	Install_SpewFunction();
	m_oldAssertHandler = SetAssertHandler(TestsAssertHandler);
}
