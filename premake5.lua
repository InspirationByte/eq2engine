-- premake5.lua

require "premake_modules/usage"

-- you can redefine dependencies
DependencyPath = {
	["zlib"] = os.getenv("ZLIB_DIR") or "src_dependency/zlib", 
	["libjpeg"] = os.getenv("JPEG_DIR") or "src_dependency/libjpeg", 
	["libogg"] = os.getenv("OGG_DIR") or "src_dependency/libogg", 
	["libvorbis"] = os.getenv("VORBIS_DIR") or "src_dependency/libvorbis", 
	["libsdl"] = os.getenv("SDL2_DIR") or "src_dependency/SDL2",
	["openal"] = os.getenv("OPENAL_DIR") or "src_dependency/openal-soft"
}

-- default configuration capabilities
Groups = {
    core = "Framework",
    
    engine1 = "Equilibrium 1 port",
    component1 = "Equilibrium 1 port/Components",

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

-- Main workspace
workspace "Equilibrium2"
    language "C++"
    configurations { "Debug", "Release" }
	linkgroups 'On'
	platforms { "x64" }
	--characterset "ASCII"
	objdir "build/obj"
	targetdir "bin/%{cfg.buildcfg}"
	location "project_%{_ACTION}"

    filter "system:linux"
        buildoptions {
            "-Wno-narrowing",
            "-fpermissive",
			"-fexceptions",
			"-fpic"
        }
		links { "pthread" }

	filter "system:Windows"
		disablewarnings { "4996", "4554", "4244", "4101", "4838", "4309" }
		defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_SECURE_NO_DEPRECATE" }

    filter "configurations:Debug"
        defines { 
            "DEBUG"
        }
        symbols "On"

    filter "configurations:Release"
        defines {
            "NDEBUG",
        }
		optimize "Speed"

	filter "system:Windows or system:Linux or system:Android"
		defines { "PLAT_SDL=1" }
		
group "Dependencies"
		
-- dependencies are in separate configuration
include "src_dependency/premake5.lua"

--[[ Once i'll transition engine to libnstd.
-- NoSTD
project "libnstd"
    kind "StaticLib"
	targetdir "bin/%{cfg.buildcfg}"
	
	filter "system:Windows"
		defines { "_CRT_SECURE_NO_WARNINGS", "__PLACEMENT_NEW_INLINE" }
    
	includedirs {
		"dependencies/libnstd/include"
	}
	
    files {
        "dependencies/libnstd/src/**.cpp",
        "dependencies/libnstd/src/**.h",
    }
	
usage "libnstd"
	includedirs {
		"dependencies/libnstd/include"
	}
	links "libnstd"
]]

group "Core"

-- eqCore essentials
project "corelib"
    kind "StaticLib"
	
    files {
		Folders.public.. "/core/*.cpp",
		Folders.public.. "/core/**.h"
	}
	includedirs {
		Folders.public
	}
	
usage "corelib"
    includedirs {
		Folders.public
	}
	
-- little framework
project "frameworkLib"
    kind "StaticLib"

	uses { "corelib", "libjpeg" }

    files {
        Folders.public.. "/utils/*.cpp",
        Folders.public.. "/math/*.cpp",
        Folders.public.. "/imaging/*.cpp",
		Folders.public.. "/utils/*.h",
        Folders.public.. "/math/*.h", 
		Folders.public.. "/math/*.inl",
        Folders.public.. "/imaging/*.h"
	}

usage "frameworkLib"
    includedirs { Folders.public }
    links { "User32" }
	
----------------------------------------------
-- e2Core

project "e2Core"
    kind "SharedLib"
	
    files {
        "core/**.cpp",
        "core/minizip/**.c",
        "core/**.h",
        Folders.public.. "/core/**.h"
	}
	
	defines { "CORE_INTERFACE_EXPORT", "COREDLL_EXPORT" }
	
    uses { "zlib", "corelib", "frameworkLib" }
	
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

----------------------------------------------
-- Sub usages

group "Components"

project "sysLib"
	kind "StaticLib"
	uses { 
		"corelib", "frameworkLib",
		"SDL2"	
	}
    files {
		Folders.shared_engine.. "sys/**.cpp",
		Folders.shared_engine.. "sys/**.h",
		Folders.shared_engine.. "input/**.cpp",
		Folders.shared_engine.. "input/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
    

project "equiLib"
	kind "StaticLib"
	uses { 
		"corelib", "frameworkLib" 
	}
    files {
		Folders.shared_engine.. "equi/**.cpp",
		Folders.shared_engine.. "equi/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	

project "networkLib"
	kind "StaticLib"
	uses {
		"corelib", "frameworkLib",
		"zlib" 
	}
    files {
		Folders.shared_engine.. "network/**.cpp",
		Folders.shared_engine.. "network/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
	filter "system:Windows"
		links { "wsock32" }

project "soundSystemLib"
	kind "StaticLib"
	uses { 
		"corelib", "frameworkLib",
		"libvorbis", "libogg", "openal-soft"
	}
    files {
		Folders.shared_engine.. "audio/**.cpp",
		Folders.shared_engine.. "audio/**.h",
		Folders.public.. "audio/**.h"
	}
    includedirs {
		Folders.public
	}

project "physicsLib"
	kind "StaticLib"
	uses { 
		"corelib", "frameworkLib",
		"bullet2"
	}
    files {
		Folders.shared_engine.. "physics/**.cpp"
	}
    includedirs {
		Folders.shared_engine
	}
	
group ""

include "premake5-eq1.lua"
include "game/DriverSyndicate/premake5.lua"

-- only build tools for big machines
if os.host() == "windows" or os.host() == "linux" then
    include "utils/premake5.lua"
end
