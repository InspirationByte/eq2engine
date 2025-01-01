-- DarkTech Package
project "dpkLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", 
		"lz4", "zlib"
	}
    files {
		Folders.public.. "dpk/**.c",
		Folders.public.. "dpk/**.cpp",
		Folders.public.. "dpk/**.h",
	}
	
usage "dpkLib"
	links "dpkLib"
	includedirs {
		Folders.shared_engine
	}

-- fonts
project "fontLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { "public" }
    files {
		Folders.shared_engine.. "font/**.cpp",
		Folders.public.. "font/**.h"
	}
	
usage "fontLib"
	links "fontLib"
	includedirs { Folders.shared_engine }

-- render utility
project "renderUtilLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { "public" }
    files {
		Folders.shared_engine.. "render/**.cpp",
		Folders.shared_engine.. "render/**.h",
		Folders.public.. "render/**.h"
	}
	
usage "renderUtilLib"
	links "renderUtilLib"
	includedirs { Folders.shared_engine }
	
-- GPU Rendering Instance Manager (GRIM)
project "grimLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public",
		"renderUtilLib",
		"imgui"
	}
    files {
		Folders.shared_engine.. "grim/**.cpp",
		Folders.shared_engine.. "grim/**.h",
	}
	
usage "grimLib"
	links "grimLib"
	includedirs { Folders.shared_engine }
	
-- EGF file loadder
project "studioFileLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"public", "shared_engine",
		"bullet2", "zlib"
	}
    files {
		Folders.shared_engine.. "studiofile/**.cpp",
		Folders.shared_engine.. "studiofile/**.h",
		Folders.public.. "egf/**.h"
	}
	
usage "studioFileLib"
	links "studioFileLib"
	includedirs { Folders.shared_engine }
	
-- Studio EGF geometry
project "studioLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared_engine",
		"studioFileLib"
	}
    files {
		Folders.shared_engine.. "studio/**.cpp",
		Folders.shared_engine.. "studio/**.c",
		Folders.shared_engine.. "studio/**.h",
		Folders.public.. "egf/**.h"
	}
	
usage "studioLib"
	links "studioLib"
	includedirs { Folders.shared_engine }

-- Animating Game Library
project "animatingLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"public",
		"studioLib"
	}
    files {
		Folders.shared_game.. "animating/**.cpp",
		Folders.shared_game.. "animating/**.h"
	}
	
usage "animatingLib"
	links "animatingLib"
	includedirs { Folders.shared_game }
	
-- Equilibrium User Interface (EqUI) library
project "equiLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public",
		"fontLib",
	}
    files {
		Folders.shared_engine.. "equi/**.cpp",
		Folders.shared_engine.. "equi/**.h"
	}
	
usage "equiLib"
	links "equiLib"
    includedirs { Folders.shared_engine }
	
-- Engine System Library
project "sysLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared_engine",
		"renderUtilLib", "equiLib",
		"SDL2", "imgui"
	}
    files {
		Folders.shared_engine.. "sys/**.cpp",
		Folders.shared_engine.. "sys/**.h",
		Folders.shared_engine.. "input/**.cpp",
		Folders.shared_engine.. "input/**.h",
		Folders.public.. "input/**.h"
	}
	
-- Eq Script Library (ESL)
project "scriptLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared_engine",
		"lua",
	}
    files {
		Folders.shared_engine.. "scripting/**.cpp",
		Folders.shared_engine.. "scripting/**.h",
		Folders.shared_engine.. "scripting/**.hpp"
	}

usage "scriptLib"
	links "scriptLib"
	includedirs { Folders.shared_engine }

-- Network lib
project "networkLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"public", "shared_engine",
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

usage "networkLib"
	links "networkLib"
	includedirs { Folders.shared_engine }
	filter "system:Windows"
		links { "wsock32" }

-- Sound System
project "soundSystemLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public",
		"minivorbis", "openal-soft", "imgui"
	}
    files {
		Folders.shared_engine.. "audio/**.cpp",
		Folders.shared_engine.. "audio/**.h",
		Folders.public.. "audio/**.h"
	}
	
usage "soundSystemLib"
	links "soundSystemLib"
    includedirs { Folders.shared_engine }

project "physicsLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared_engine",
		"bullet2"
	}
    files {
		Folders.shared_engine.. "physics/**.cpp",
		Folders.shared_engine.. "physics/**.h",
		Folders.public.. "physics/**.h"
	}
	
usage "physicsLib"
	links "physicsLib"
    includedirs { Folders.public, Folders.shared_engine }
	
-- Movie Player library
project "movieLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared_engine",
		"ffmpeg"
	}
    files {
		Folders.shared_engine.. "movie/**.cpp",
		Folders.shared_engine.. "movie/**.h",
	}
	filter "system:android"
		defines {
			"MOVIELIB_DISABLE"
		}
		
usage "movieLib"
	links "movieLib"
    includedirs { Folders.shared_engine }

-- only build tools for big machines
if ENABLE_TOOLS then

	-- EGF generator
	project "egfLib"
		kind "StaticLib"
		properties { "unitybuild" }
		uses {
			"public", "shared_engine",
			"studioFileLib",
			"bullet2", "zlib", "openfbx", "meshoptimizer",
		}
		files {
			Folders.shared_engine.. "egf/**.cpp",
			Folders.shared_engine.. "egf/**.c",
			Folders.shared_engine.. "egf/**.h",
			Folders.public.. "egf/**.h"
		}
		
	usage "egfLib"
		links "egfLib"
		includedirs { Folders.shared_engine }

	-- Equilibrium 1 Darktech Physics (Deprecated but kept for egfMan)
	project "dkPhysicsLib"
		kind "StaticLib"
		properties { "unitybuild" }
		uses {
			"public", "shared_engine",
			"studioLib", "animatingLib", 
			"bullet2"
		}
		files {
			Folders.shared_engine.. "dkphysics/**.cpp",
			Folders.shared_engine.. "dkphysics/**.h",
			Folders.public.. "dkphysics/**.h"
		}

	usage "dkPhysicsLib"
		links "dkPhysicsLib"
		includedirs { Folders.shared_engine }
end


