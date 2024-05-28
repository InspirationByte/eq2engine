#include <gtest/gtest.h>

#include "core/core_common.h"
#include "tests_common.h"

int main(int argc, char** argv) 
{
	TestAppWrapper test(false, "esl_tests", argc, argv);

	testing::InitGoogleTest(&argc, argv);

	// you can specify flags before running
	//::testing::FLAGS_gtest_filter = "";

	return RUN_ALL_TESTS();
}