#pragma once
#include "core/platform/messagebox.h"

class TestAppWrapper
{
public:
	~TestAppWrapper();
	TestAppWrapper(bool enableCore, const char* name, int argc, char** argv);

private:
	AssertHandlerFn	m_oldAssertHandler{ nullptr };
	bool			m_enableCore{ false };
};