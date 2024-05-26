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
	GetCDkCore()->Shutdown();
	SetAssertHandler(m_oldAssertHandler);
}

TestAppWrapper::TestAppWrapper(const char* name, int argc, char** argv)
{
	GetCDkCore()->Init(name, argc, argv);
	m_oldAssertHandler = SetAssertHandler(TestsAssertHandler);
}
