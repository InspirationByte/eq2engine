group "Components"

-- Package File Lib
project "dpkLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"lz4", "zlib"
	}
    files {
		"dpk/**",
	}

-- Font Loader and Renderer
project "fontLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { "public", "shared" }
    files {
		"font/**",
		PUBLIC_DIR.."/font/"
	}
	vpaths {
		["*"] = "font/**",
		["Public Headers"] = PUBLIC_DIR.."/font/**.h"
	}

-- Render Utility and Debug Drawing
project "renderUtilLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { "public", "shared" }
    files {
		"render/**",
		PUBLIC_DIR.."/render/**"
	}
	vpaths {
		["*"] = "render/**",
		["Public Headers"] = PUBLIC_DIR.."/render/**.h"
	}
	
-- GPU Rendering Instance Manager (GRIM)
project "grimLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"renderUtilLib",
		"imgui"
	}
    files {
		"grim/**",
	}
	
-- EGF file loadder
project "studioFileLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"public", "shared",
		"bullet2", "zlib"
	}
    files {
		"studiofile/**",
	}
	
-- Studio EGF geometry
project "studioLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"studioFileLib"
	}
    files {
		"studio/**",
	}
	
-- Equilibrium User Interface (EqUI) library
project "equiLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"fontLib",
		"imgui"
	}
    files {
		"equi/**",
	}
	
-- ImGui backend library
project "imguiBackendLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"imgui"
	}
    files {
		"imgui_backend/**.cpp",
		"imgui_backend/**.h",
	}

-- Network lib
project "networkLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"public", "shared",
		"zlib" 
	}
    files {
		"network/**",
	}
	-- this one is temporary. Once rewrite done it will be removed
	excludes {
		"network/NETThread.cpp",
	}
	
	filter "system:Windows"
		links { "wsock32" }

-- Sound System
project "soundSystemLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"minivorbis", "openal-soft", "imgui"
	}
    files {
		"audio/**",
		PUBLIC_DIR.."/audio/**"
	}
	vpaths {
		["*"] = "audio/**",
		["Public Headers"] = PUBLIC_DIR.."/audio/**.h"
	}

-- Physics Engine Library
project "physicsLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"bullet2"
	}
    files {
		"physics/**",
		PUBLIC_DIR.."/physics/**"
	}
	vpaths {
		["*"] = "physics/**",
		["Public Headers"] = PUBLIC_DIR.."/physics/**.h"
	}
	
-- Movie Player library
project "movieLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"ffmpeg"
	}
    files {
		"movie/**",
	}	
	filter "system:android"
		defines {
			"MOVIELIB_DISABLE"
		}
		
-- Eq Script Library (ESL)
project "scriptLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared",
		"lua",
	}
    files {
		"scripting/**",
	}
		
-- Engine System Library (Host, Input, States)
project "sysLib"
	kind "StaticLib"
	properties { "unitybuild" }
	uses { 
		"public", "shared", "scriptLib",
		"renderUtilLib", "equiLib", "movieLib",
		"SDL2", "imguiBackendLib", "imgui_lua"
	}
    files {
		"sys/**",
		"input/**",
		"gamesys/**",
		PUBLIC_DIR.."/input/**"
	}
	vpaths {
		["sys/*"] = "sys/**",
		["input/*"] = "input/**",
		["Public Headers"] = PUBLIC_DIR.."/input/**.h"
	}

-- only build tools for big machines
if ENABLE_TOOLS then
	-- EGF generator
	project "egfLib"
		kind "StaticLib"
		properties { "unitybuild" }
		uses {
			"public", "shared",
			"studioFileLib",
			"bullet2", "zlib", "openfbx", "meshoptimizer",
		}
		files {
			"egf/**",
			PUBLIC_DIR.."/egf/**"
		}
		vpaths {
			["*"] = "egf/**",
			["Public Headers"] = PUBLIC_DIR.."/egf/**.h"
		}

	-- Equilibrium 1 Darktech Physics (Deprecated but kept for egfMan)
	project "dkPhysicsLib"
		kind "StaticLib"
		properties { "unitybuild" }
		uses {
			"public", "shared",
			"studioLib", "animatingLib", 
			"jolt"
		}
		files {
			"dkphysics/**",
			PUBLIC_DIR.."/dkphysics/**"
		}
		vpaths {
			["*"] = "dkphysics/**",
			["Public Headers"] = PUBLIC_DIR.."/dkphysics/**.h"
		}
end

-- Animating Game Library
project "animatingLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"public", "shared",
		"studioLib"
	}
    files {
		"animating/**",
	}