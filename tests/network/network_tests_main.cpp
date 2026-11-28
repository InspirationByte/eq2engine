#include <gtest/gtest.h>

#include "core/core_common.h"
#include "tests_common.h"

int main(int argc, char** argv)
{
	TestAppWrapper test(false, "network_tests", argc, argv);
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
