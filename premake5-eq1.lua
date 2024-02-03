group "Components"

-- fonts
project "fontLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core"
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
		"corelib", "frameworkLib", "e2Core"
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
		"corelib", "frameworkLib", "e2Core",
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
		"corelib", "frameworkLib", "e2Core",
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
		"corelib", "frameworkLib", "e2Core",
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
	
if ENABLE_TOOLS then

	-- Equilibrium 1 Darktech Physics
	project "dkPhysicsLib"
		kind "StaticLib"
		unitybuild "on"
		uses {
			"corelib", "frameworkLib", "e2Core",
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
end -- ENABLE_TOOLS

----------------------------------------------
-- Equilirium Material System 1

group "MatSystem"

project "BaseShader"
    kind "StaticLib"
	uses {
		"corelib", "frameworkLib"
	}
    files {
		Folders.public.."materialsystem1/*.cpp",
		Folders.public.."materialsystem1/*.h"
	}

project "eqMatSystem"
    kind "SharedLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
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
		"corelib", "frameworkLib", "e2Core",
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
		"corelib", "frameworkLib", "e2Core",
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
		"corelib", "frameworkLib", "e2Core",
		"eqRHIBaseLib", "wgpu-dawn"
	}
	defines{
		"EQRHI_WGPU",
		"RENDERER_TYPE=4"
	}
	files {
		Folders.matsystem1.. "renderers/WGPU/**.cpp",
		Folders.matsystem1.."renderers/WGPU/**.h"
	}

