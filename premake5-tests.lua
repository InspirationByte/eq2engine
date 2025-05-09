-- premake5.lua

WORKSPACE_NAME = "auto_tests"
ENABLE_TESTS = true
ENABLE_TOOLS = false
ENABLE_MATSYSTEM = false
BUILD_SINGLE_FILE = true

dofile "premake5.lua"
include "tests"
