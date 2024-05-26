#pragma once
#include "core/platform/messagebox.h"

class TestAppWrapper
{
public:
	~TestAppWrapper();
	TestAppWrapper(const char* name, int argc, char** argv);

private:
	AssertHandlerFn	m_oldAssertHandler{ nullptr };
};