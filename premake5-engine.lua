-- premake5.lua

require ".premake_modules/usage"
require ".premake_modules/androidndk"
require ".premake_modules/unitybuild"
require ".premake_modules/wxwidgets"
require ".premake_modules/vscode"

local CAN_BUILD_TOOLS = (os.target() == "linux" or os.target() == "windows") and not IS_ANDROID
local CAN_BUILD_GUI_TOOLS = (--[[os.target() == "linux" or]] os.target() == "windows") and not IS_ANDROID

IS_ANDROID = (_ACTION == "androidndk")
ENABLE_TOOLS = iif(ENABLE_TOOLS == nil, CAN_BUILD_TOOLS, ENABLE_TOOLS)
ENABLE_GUI_TOOLS = iif(ENABLE_GUI_TOOLS == nil, CAN_BUILD_GUI_TOOLS, ENABLE_GUI_TOOLS)
ENABLE_MATSYSTEM = iif(ENABLE_MATSYSTEM == nil, true, ENABLE_MATSYSTEM)
ENABLE_TESTS = iif(ENABLE_TESTS == nil, false, ENABLE_TESTS)
WORKSPACE_NAME = (WORKSPACE_NAME or "Equilibrium2")

-- you can redefine dependencies
DependencyPath = {
	["zlib"] = os.getenv("ZLIB_DIR") or "src_dependency/zlib", 
	["libjpeg"] = os.getenv("JPEG_DIR") or "src_dependency/libjpeg",
	["libsdl"] = os.getenv("SDL2_DIR") or "src_dependency/SDL2",
	["openal"] = os.getenv("OPENAL_DIR") or "src_dependency/openal-soft",
	
	["Android_libsdl"] = os.getenv("SDL2_DIR") or "src_dependency_android/SDL2",
	["Android_openal"] = os.getenv("OPENAL_DIR") or "src_dependency_android/openal-soft",
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
    dependency = "./src_dependency/",
    game = "./game/",
}

function prj_name(prj, def)
	if prj ~= nil then
		if prj.group ~= nil and string.len(prj.group) > 0 then
			return prj.group .. '/' .. prj.name
		end
		return prj.name
	end
	return def
end

-- Main workspace
workspace(WORKSPACE_NAME)
    language "C++"
	cppdialect "C++17"	-- required for sol2
    configurations { "Debug", "Release", "ReleaseAsan", "Profile", "Retail" }
	linkgroups 'On'
	
	--characterset "ASCII"
	objdir "build/obj"
	targetdir "build/bin/%{cfg.platform}/%{cfg.buildcfg}"

	if _ACTION ~= "vscode" then
		location "build/%{ prj_name(prj) }"
	end

	defines {
		"PROJECT_COMPILE_CONFIGURATION=%{cfg.buildcfg}",
		"PROJECT_COMPILE_PLATFORM=%{cfg.platform}"
	}

	if IS_ANDROID then
		system "android"
	end

	filter "system:android"
		shortcommands "On"
		
		platforms {
			"android-arm", --TEMPORARILY DISABLED FOR COMPILE TIME SPEED
			"android-arm64"
			--"android-x86_64"
		}
		
		disablewarnings {
			-- disable warnings which are emitted by my stupid (and not so) code
			"c++11-narrowing",
			"writable-strings",
			"logical-op-parentheses",
			"parentheses",
			"register",
			"unused-local-typedef",
			"nonportable-include-path",
			"format-security",
			"unused-parameter",
			"sign-compare",
		}
		
		buildoptions {
			"-fpermissive",
			"-fexceptions",
			"-pthread"
		}
		
		linkoptions {
			"--no-undefined",
			"-fexceptions",
			"-pthread",
			
			"-mfloat-abi=softfp",	-- force NEON to be used
			"-mfpu=neon"
		}

		filter "platforms:*-x86"
			architecture "x86"

		filter "platforms:*-x86_64"
			architecture "x86_64"

		filter "platforms:*-arm"
			architecture "arm"

		filter "platforms:*-arm64"
			architecture "arm64"

    filter "system:linux"
		platforms { 
			"x86", "x64" -- maybe add ARM & ARM64 for RPi?
		}
		vscode_makefile "build/%{wks.name}"
		vscode_launch_cwd ("${workspaceRoot}/../%{wks.name}/build/bin64linux")
		vscode_launch_environment {
			LD_LIBRARY_PATH = "${LD_LIBRARY_PATH}:${workspaceRoot}%{cfg.targetdir}:${workspaceRoot}/../%{wks.name}/build/bin64linux"
		}
		vscode_launch_visualizerFile "${workspaceRoot}/public/types.natvis"
        buildoptions {
            "-fpermissive",
			"-fexceptions",
			"-fpic",
        }
		disablewarnings {
			-- disable warnings which are emitted by my stupid (and not so) code
			"narrowing",
			"c++11-narrowing",
			"writable-strings",
			"logical-op-parentheses",
			"parentheses",
			"register",
			"unused-local-typedef",
			"nonportable-include-path",
			"format-security",
			"unused-parameter",
			"sign-compare",
			"ignored-attributes",	-- annyoing, don't re-enable
			"write-strings",		-- TODO: fix this
			"subobject-linkage"		-- TODO: fix this
		}
		links { "pthread" }

	filter "system:Windows"
		platforms { 
			"x86", "x64" -- maybe add ARM64?
		}
		disablewarnings { "4996", "4554", "4244", "4101", "4838", "4309" }
		enablewarnings { "26433" }
		defines { 
			"NOMINMAX", 
			"_CRT_SECURE_NO_WARNINGS", "_CRT_SECURE_NO_DEPRECATE"
		}

    filter "configurations:Debug"
        defines { 
            "DEBUG"
        }
        symbols "On"

    filter "configurations:Release"
        defines {
            "NDEBUG",
        }
		optimize "On"
		symbols "On"
		
	filter "configurations:ReleaseAsan"
        defines {
            "NDEBUG",
        }
		optimize "On"
		symbols "On"
		sanitize "address"

	filter "configurations:Profile"
        defines {
			"NDEBUG",
			"_PROFILE"
        }
		optimize "On"
		symbols "On"

	filter "configurations:Retail"
        defines {
			"NDEBUG",
			"_RETAIL"
        }
		optimize "On"

	filter "system:Linux"
		defines {
			"__LINUX__"
		}

	filter "system:Windows or system:Linux or system:Android"
		defines { 
			"EQ_USE_SDL"
		}

group "Core"

-- eqCore essentials
project "coreLib"
    kind "StaticLib"
	uses "concurrency_vis"
	
	--unitybuild "on"
    files {
		Folders.public.. "/core/**.cpp",
		Folders.public.. "/core/**.h"
	}
	includedirs {
		Folders.public
	}
	
usage "coreLib"
    includedirs {
		Folders.public
	}
	links { "coreLib" }

	filter "system:Linux"
		links { "pthread" }
	
-- little framework
project "frameworkLib"
    kind "StaticLib"

	unitybuild "on"
	uses { "coreLib", "libjpeg" }

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
	
	filter "system:Windows"
		links { "User32" }
	
----------------------------------------------
-- e2Core

project "e2Core"
    kind "SharedLib"
	
	unitybuild "on"
    files {
        "core/**.cpp",
        "core/minizip/**.c",
        "core/**.h",
        Folders.public.. "/core/**.h"
	}
	
	defines { "CORE_INTERFACE_EXPORT", "COREDLL_EXPORT" }
	
    uses {
		"coreLib", "frameworkLib", "dpkLib"
	}

	filter "system:Windows"
		linkoptions { "-IGNORE:4217,4286" }	-- disable few linker warnings

    filter "system:android"
        files {
			"core/android_libc/**.c", 
			"core/android_libc/**.h"
		}

    filter "system:Windows"
        links {"User32", "DbgHelp", "Advapi32"}
		
usage "e2Core"
	links "e2Core"

group "Dependencies"
		
-- dependencies are in separate configuration
include "src_dependency/premake5.lua"
include "src_dependency_android/premake5.lua"

group "Components"

-- components are in separate configuration
include "premake5-components.lua"	

----------------------------------------------
-- Material System and rendering

if ENABLE_MATSYSTEM then

group "MatSystem"

project "BaseShader"
    kind "StaticLib"
	uses {
		"coreLib", "frameworkLib"
	}
    files {
		Folders.public.."materialsystem1/*.cpp",
		Folders.public.."materialsystem1/*.h"
	}

project "eqMatSystem"
    kind "SharedLib"
	unitybuild "on"
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

usage "BaseShader"
	links "BaseShader"

-- base shader library
project "eqBaseShaders"
    kind "SharedLib"
	unitybuild "on"
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

-- empty renderer
project "eqNullRHI"
    kind "SharedLib"
	unitybuild "on"
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
		Folders.matsystem1.."Renderers/Empty/**.h"
	}

-- WebGPU renderer (atm Windows-only)
project "eqWGPURHI"
	kind "SharedLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"eqRHIBaseLib", "wgpu-dawn"
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
	include "utils/premake5.lua"
end
