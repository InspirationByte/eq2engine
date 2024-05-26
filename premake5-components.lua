usage "shared_engine"
	includedirs {
		Folders.shared_engine
	}

-- fonts
project "fontLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core"
	}
    files {
		Folders.shared_engine.. "font/**.cpp",
		Folders.public.. "font/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
usage "fontLib"
	links "fontLib"
	includedirs {
		Folders.shared_engine
	}

-- render utility
project "renderUtilLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core"
	}
    files {
		Folders.shared_engine.. "render/**.cpp",
		Folders.shared_engine.. "render/**.h",
		Folders.public.. "render/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
usage "renderUtilLib"
	links "renderUtilLib"
	includedirs {
		Folders.shared_engine
	}
	
-- Studio model lib
project "studioFileLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"renderUtilLib", "bullet2", "zlib"
	}
    files {
		Folders.shared_engine.. "studiofile/**.cpp",
		Folders.shared_engine.. "studiofile/**.h",
		Folders.public.. "egf/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
usage "studioFileLib"
	links "studioFileLib"
	includedirs {
		Folders.shared_engine
	}
	
-- Studio model lib
project "studioLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"renderUtilLib", "bullet2", "zlib",
		"studioFileLib"
	}
    files {
		Folders.shared_engine.. "studio/**.cpp",
		Folders.shared_engine.. "studio/**.c",
		Folders.shared_engine.. "studio/**.h",
		Folders.public.. "egf/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
usage "studioLib"
	links "studioLib"
	includedirs {
		Folders.shared_engine
	}

-- Animating game library
project "animatingLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"studioLib", "bullet2"
	}
    files {
		Folders.shared_game.. "animating/**.cpp",
		Folders.shared_game.. "animating/**.h"
	}
    includedirs {
		Folders.shared_game
	}
	
usage "animatingLib"
	links "animatingLib"
	includedirs {
		Folders.shared_game
	}

group "Components"

project "dpkLib"
	kind "StaticLib"
	unitybuild "on"
	uses { 
		"lz4", "zlib"
	}
    files {
		Folders.public.. "dpk/**.c",
		Folders.public.. "dpk/**.cpp",
		Folders.public.. "dpk/**.h",
	}
    includedirs {
		Folders.public
	}

project "sysLib"
	kind "StaticLib"
	unitybuild "on"
	uses { 
		"coreLib", "frameworkLib",
		"equiLib",
		"SDL2", "imgui"
	}
    files {
		Folders.shared_engine.. "sys/**.cpp",
		Folders.shared_engine.. "sys/**.h",
		Folders.shared_engine.. "input/**.cpp",
		Folders.shared_engine.. "input/**.h",
		Folders.public.. "input/**.h"
	}
    includedirs {
		Folders.shared_engine
	}

project "equiLib"
	kind "StaticLib"
	unitybuild "on"
	uses { 
		"coreLib", "frameworkLib",
		"fontLib",
	}
    files {
		Folders.shared_engine.. "equi/**.cpp",
		Folders.shared_engine.. "equi/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
project "scriptLib"
	kind "StaticLib"
	unitybuild "on"
	uses { 
		"coreLib", "frameworkLib",
		"lua",
	}
    files {
		Folders.shared_engine.. "scripting/**.cpp",
		Folders.shared_engine.. "scripting/**.h",
		Folders.shared_engine.. "scripting/**.hpp"
	}
    includedirs {
		Folders.shared_engine
	}	
	
project "networkLib"
	kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib",
		"zlib" 
	}
    files {
		Folders.shared_engine.. "network/**.cpp",
		Folders.shared_engine.. "network/**.h"
	}
	-- this one is temporary. Once rewrite done it will be removed
	excludes {
		Folders.shared_engine.. "network/NETThread.cpp",
	}
    includedirs {
		Folders.shared_engine
	}
	
	filter "system:Windows"
		links { "wsock32" }

project "soundSystemLib"
	kind "StaticLib"
	unitybuild "on"
	uses { 
		"coreLib", "frameworkLib",
		"minivorbis", "openal-soft",
		"imgui"
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
	unitybuild "on"
	uses { 
		"coreLib", "frameworkLib",
		"bullet2"
	}
    files {
		Folders.shared_engine.. "physics/**.cpp",
		Folders.shared_engine.. "physics/**.h",
		Folders.public.. "physics/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
project "movieLib"
	kind "StaticLib"
	unitybuild "on"
	uses { 
		"coreLib", "frameworkLib", "ffmpeg"
	}
    files {
		Folders.shared_engine.. "movie/**.cpp",
		Folders.shared_engine.. "movie/**.h",
	}
    includedirs {
		Folders.shared_engine
	}
	filter "system:android"
		defines {
			"MOVIELIB_DISABLE"
		}

-- only build tools for big machines
if ENABLE_TOOLS then

	-- EGF generator
	project "egfLib"
		kind "StaticLib"
		unitybuild "on"
		uses {
			"coreLib", "frameworkLib", "e2Core",
			"bullet2", "zlib", "openfbx", 
			"studioFileLib"
		}
		files {
			Folders.shared_engine.. "egf/**.cpp",
			Folders.shared_engine.. "egf/**.c",
			Folders.shared_engine.. "egf/**.h",
			Folders.public.. "egf/**.h"
		}
		includedirs {
			Folders.shared_engine
		}
		
	usage "egfLib"
		links "egfLib"
		includedirs {
			Folders.shared_engine
		}

	-- Equilibrium 1 Darktech Physics (Deprecated but kept for egfMan)
	project "dkPhysicsLib"
		kind "StaticLib"
		unitybuild "on"
		uses {
			"coreLib", "frameworkLib", "e2Core",
			"studioLib", "animatingLib", "bullet2"
		}
		files {
			Folders.shared_engine.. "dkphysics/**.cpp",
			Folders.shared_engine.. "dkphysics/**.h",
			Folders.public.. "dkphysics/**.h"
		}
		includedirs {
			Folders.shared_engine
		}

	usage "dkPhysicsLib"
		links "dkPhysicsLib"
		includedirs {
			Folders.shared_engine
		}
end


