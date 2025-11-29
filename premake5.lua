print("_MAIN_SCRIPT_DIR", _MAIN_SCRIPT_DIR)

-- premake5.lua
require ".premake_modules/usage"
require ".premake_modules/properties"
require ".premake_modules/androidndk"
require ".premake_modules/unitybuild"
require ".premake_modules/wxwidgets"
require ".premake_modules/vscode"

IS_ANDROID = (_ACTION == "androidndk")

local CAN_BUILD_TOOLS = (_TARGET_OS == "linux" or _TARGET_OS == "windows")
local CAN_BUILD_GUI_TOOLS = (--[[os.target() == "linux" or]] _TARGET_OS == "windows")

WORKSPACE_NAME = (WORKSPACE_NAME or "Equilibrium2")
ENGINE_DIR = ENGINE_DIR or _WORKING_DIR

-- project can define own Windows SDK version
WINSDK_VER = WINSDK_VER or os.getenv("WINSDK_VER") or "latest"

BUILD_SINGLE_FILE = iif(BUILD_SINGLE_FILE == nil, false, BUILD_SINGLE_FILE)

ENABLE_TOOLS = iif(ENABLE_TOOLS == nil, CAN_BUILD_TOOLS, ENABLE_TOOLS)
ENABLE_GUI_TOOLS = iif(ENABLE_GUI_TOOLS == nil, CAN_BUILD_GUI_TOOLS, ENABLE_GUI_TOOLS)
ENABLE_MATSYSTEM = iif(ENABLE_MATSYSTEM == nil, true, ENABLE_MATSYSTEM)
ENABLE_TESTS = iif(ENABLE_TESTS == nil, false, ENABLE_TESTS)
ENABLE_LIVEPP = iif(ENABLE_LIVEPP == nil, false, ENABLE_LIVEPP)

print("Workspace", WORKSPACE_NAME)
print("Target OS", _TARGET_OS)
print("Target Arch", _TARGET_ARCH or "Not defined")
if _TARGET_OS == "windows" then
	print("Windows SDK =", WINSDK_VER)
end
print("\n")
print("Build details:")
print("Single File Compilation =", BUILD_SINGLE_FILE)
print("ENABLE_TOOLS =", ENABLE_TOOLS)
print("ENABLE_MATSYSTEM =", ENABLE_MATSYSTEM)
print("ENABLE_TESTS =", ENABLE_TESTS)
print("ENABLE_LIVEPP =", ENABLE_LIVEPP)
print("\n")

-- you can redefine dependencies
DependencyPath = {
	["libsdl"] = os.getenv("SDL2_DIR") or "SDL2",
	["openal"] = os.getenv("OPENAL_DIR") or "openal-soft",
	
	["Android_libsdl"] = os.getenv("SDL2_DIR") or "SDL2",
	["Android_openal"] = os.getenv("OPENAL_DIR") or "openal-soft",
}

include "premake5-properties.lua"
	
-- Main workspace
workspace(WORKSPACE_NAME)
	properties {
		"e2_ws_settings",
		"e2_ws_configurations",
		"gcc_clang",
		"windows_msvc"
	}
	configurations {
		"Debug", 		-- full debug, no optimization
		"Release", 		-- optimized build with asserts and debug drawing and UI on
		"ReleaseAsan", 	-- same as Release except ASAN is enabled
		"Profile", 		-- optimized build without asserts and debug drawing, debug UI present
		"Retail" 		-- optimized build without all of the debug stuff
	}
	
	defines {
		"PROJECT_COMPILE_CONFIGURATION=%{cfg.buildcfg}",
		"PROJECT_COMPILE_PLATFORM=%{cfg.platform}"
	}
		
	filter {}
	
	-- setup VSCode generator settings
	vscode_makefile "build/%{wks.name}.solution"
	vscode_launch_cwd ("${workspaceRoot}/../build/bin64linux")
	vscode_launch_environment {
		LD_LIBRARY_PATH = "${LD_LIBRARY_PATH}:${workspaceRoot}/%{cfg.targetdir}:${workspaceRoot}/../build/bin64linux"
	}
	vscode_launch_visualizerFile "${workspaceRoot}/public/types.natvis"

PUBLIC_DIR = ENGINE_DIR.."/public"
FRAMEWORK_DIR = ENGINE_DIR.."/framework"
SHARED_DIR = ENGINE_DIR.."/shared"

-- properties
usage "public"
	includedirs {
		PUBLIC_DIR,
		FRAMEWORK_DIR
	}
	
usage "shared"
	includedirs {
		SHARED_DIR
	}

include(ENGINE_DIR.."/src_dependency")
include(ENGINE_DIR.."/framework")
include(ENGINE_DIR.."/core")
include(ENGINE_DIR.."/shared")

group ""

-- only build tools for big machines
if ENABLE_TOOLS then
	include(ENGINE_DIR.."/tools")
end
