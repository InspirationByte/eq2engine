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

-- default configuration capabilities
Groups = {
    core = "Framework",
    engine2 = "Equilibrium 2",
    tools = "Tools",
    game = "Game",
}

-- folders for framework, libraries and tools
Folders = {
    public =  "./public/",
    matsystem1 = "./materialsystem1/",
    shared_engine = "./shared_engine/",
    shared_game = "./shared_game/",
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
	
	filter "platforms:*64"
		debugdir "%{wks.location}../../%{wks.name}/build/Bin64"
		debugenvs "PATH=%{wks.location}../../%{wks.name}/build/Bin64"

	filter "platforms:*86"
		debugdir "%{wks.location}../../%{wks.name}/build/Bin32"
		debugenvs "PATH=%{wks.location}../../%{wks.name}/build/Bin64"
		
	filter {}
	
	-- setup VSCode generator settings
	vscode_makefile "build/%{wks.name}.solution"
	vscode_launch_cwd ("${workspaceRoot}/../%{wks.name}/build/bin64linux")
	vscode_launch_environment {
		LD_LIBRARY_PATH = "${LD_LIBRARY_PATH}:${workspaceRoot}%{cfg.targetdir}:${workspaceRoot}/../%{wks.name}/build/bin64linux"
	}
	vscode_launch_visualizerFile "${workspaceRoot}/public/types.natvis"

group "Dependencies"

-- dependencies are in separate configuration
include(ENGINE_DIR.."/src_dependency")

if IS_ANDROID then
include(ENGINE_DIR.."/src_dependency_android")
end


-- properties
usage "public"
	includedirs {
		Folders.public
	}
	
usage "shared_engine"
	includedirs {
		Folders.shared_engine
	}
	
usage "shared_game"
	includedirs {
		Folders.shared_engine
	}

group "Core"

-- eqCore essentials
project "coreLib"
    kind "StaticLib"
	properties { "unitybuild", "concurrency_vis" }
	uses { "public" }
    files {
		Folders.public.. "/core/**.cpp",
		Folders.public.. "/core/**.h"
	}
	
usage "coreLib"
    includedirs { Folders.public }
	links { "coreLib" }
	filter "system:Linux"
		links { "pthread" }
	
-- little framework
project "frameworkLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { "public", "stb" }

    files {
		Folders.public.. "ds/*.cpp",
        Folders.public.. "utils/*.cpp",
        Folders.public.. "math/*.cpp",
        Folders.public.. "imaging/*.cpp",
		Folders.public.. "ds/*.h",
		Folders.public.. "utils/*.h",
        Folders.public.. "math/*.h", 
		Folders.public.. "math/*.inl",
        Folders.public.. "imaging/*.h",
		Folders.public.. "**.natvis"
	}

usage "frameworkLib"
    includedirs { Folders.public }
	links { "frameworkLib" }

	filter "system:Android"
		links { "log" }
	
----------------------------------------------
-- e2Core

project "e2Core"
    kind "SharedLib"
	properties { "unitybuild", "live_pp", "concurrency_vis" }
    uses {
		"coreLib", "frameworkLib", "dpkLib"
	}
    files {
        "core/**.cpp",
        "core/minizip/**.c",
        "core/**.h",
	}
	
	defines { "CORE_INTERFACE_EXPORT", "COREDLL_EXPORT" }

	filter "system:Windows"
		linkoptions { "-IGNORE:4217,4286" }	-- disable few linker warnings

    filter "system:android"
        files {
			"core/android_libc/**.c", 
			"core/android_libc/**.h"
		}

    filter "system:Windows"
        links { "User32", "DbgHelp", "Advapi32" }
		
usage "e2Core"
	links "e2Core"

group "Components"

-- components are in separate configuration
include "premake5-components.lua"	

----------------------------------------------
-- Material System and rendering

if ENABLE_MATSYSTEM then

group "MatSystem"

project "BaseShader"
    kind "StaticLib"
	uses { "public" }
    files {
		Folders.public.."materialsystem1/*.cpp",
		Folders.public.."materialsystem1/*.h"
	}
	
usage "BaseShader"
	links "BaseShader"

project "eqMatSystem"
    kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"BaseShader"
	}
    files {
        Folders.matsystem1.. "*.cpp",
		Folders.matsystem1.. "*.h",
		Folders.matsystem1.. "Renderers/*.h",
		Folders.public.."materialsystem1/**.h"
	}

-- base shader library
project "eqBaseShaders"
    kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"BaseShader"
	}
    files {
        Folders.matsystem1.."Shaders/*.cpp",
		Folders.matsystem1.."Shaders/**.h",
        Folders.matsystem1.."Shaders/Base/**.cpp",
	}
    includedirs {
		Folders.public.."materialsystem1"
	}

----------------------------------------------
-- Render hardware interface libraries of Eq1
group "MatSystem/RHI"

-- base library
usage "eqRHIBaseLib"
    files {
		Folders.matsystem1.. "Renderers/Shared/**.cpp",
		Folders.matsystem1.."Renderers/Shared/**.h", 
		Folders.matsystem1.."Renderers/*.cpp",
		Folders.matsystem1.."Renderers/*.h",
		Folders.public.."materialsystem1/renderers/**.h",
	}
    includedirs {
		Folders.public.."materialsystem1/",
		Folders.matsystem1.."Renderers/Shared"
	}
	vpaths {
		["*"] = Folders.matsystem1.."Renderers/*",
		["Public Headers"] = Folders.public.."materialsystem1/renderers/**.h",
	}

-- empty renderer
project "eqNullRHI"
    kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"eqRHIBaseLib"
	}
	defines{
		"EQRHI_NULL",
		"RENDERER_TYPE=0"
	}
    files {
		Folders.matsystem1.. "Renderers/Empty/**.cpp",
		Folders.matsystem1.."Renderers/Empty/**.h",
	}

-- WebGPU renderer (atm Windows-only)
project "eqWGPURHI"
	kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"eqRHIBaseLib", 
		"wgpu-dawn"
	}
	defines{
		"EQRHI_WGPU",
		"RENDERER_TYPE=4"
	}
	files {
		Folders.matsystem1.. "Renderers/WGPU/**.cpp",
		Folders.matsystem1.."Renderers/WGPU/**.h"
	}


end -- ENABLE_MATSYSTEM

group ""

-- only build tools for big machines
if ENABLE_TOOLS then
	include(ENGINE_DIR.."/tools")
end
