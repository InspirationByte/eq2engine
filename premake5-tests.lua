-- premake5.lua

WORKSPACE_NAME = "auto_tests"
ENABLE_TESTS = true
ENABLE_TOOLS = false
ENABLE_MATSYSTEM = false
BUILD_SINGLE_FILE = true

dofile "premake5.lua"

-- PPMem is disabled in tests: an overwritten guard checkmark makes PPFree release a wrong pointer
workspace(WORKSPACE_NAME)
	defines { "PPMEM_DISABLED" }

include "tests"
