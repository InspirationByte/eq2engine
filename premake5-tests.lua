-- premake5.lua

WORKSPACE_NAME = "auto_tests"
ENABLE_TESTS = true
ENABLE_TOOLS = false
ENABLE_MATSYSTEM = false
BUILD_SINGLE_FILE = true

dofile "premake5.lua"

-- PPMem is enabled only in Debug so ppmem_tests can check it;
-- other configurations test containers on plain CRT allocations
workspace(WORKSPACE_NAME)
	filter "configurations:not Debug"
		defines { "PPMEM_DISABLED" }
	filter {}

include "tests"
