group "Equilibrium 1/Components"

-- fonts
project "fontLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core"
	}
    files {
		Folders.shared_engine.. "font/**.cpp"
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
		Folders.shared_engine.. "render/**.cpp"
	}
    includedirs {
		Folders.shared_engine
	}
	
usage "renderUtilLib"
	links "renderUtilLib"
	includedirs {
		Folders.shared_engine
	}

-- EGF
project "egfLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
		"renderUtilLib", "bullet2", "zlib",
		"openfbx"
	}
    files {
		Folders.shared_engine.. "egf/**.cpp",
		Folders.shared_engine.. "egf/**.c",
		Folders.shared_engine.. "egf/**.h"
	}
    includedirs {
		Folders.shared_engine
	}
	
usage "egfLib"
	links "egfLib"
	includedirs {
		Folders.shared_engine
	}

-- Animating game library
project "animatingLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
		"egfLib", "bullet2"
	}
    files {
		Folders.shared_game.. "animating/**.cpp"
	}
    includedirs {
		Folders.shared_game
	}
	
usage "animatingLib"
	links "animatingLib"
	includedirs {
		Folders.shared_game
	}

-- Equilibrium 1 Darktech Physics
project "dkPhysicsLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
		"egfLib", "animatingLib", "bullet2"
	}
    files {
		Folders.shared_engine.. "dkphysics/**.cpp",
	}
	includedirs {
		Folders.shared_engine
	}

usage "dkPhysicsLib"
	links "dkPhysicsLib"
	includedirs {
		Folders.shared_engine
	}

----------------------------------------------
-- Equilirium Material System 1

group "Equilibrium 1/MatSystem"

project "eqMatSystem"
    kind "SharedLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core"
	}
    files {
        Folders.matsystem1.. "*.cpp",
        Folders.public.."materialsystem1/**.cpp",
		Folders.matsystem1.."**.h",
		Folders.public.."materialsystem1/**.h"
	}

usage "BaseShader"
	files {
		Folders.public.."materialsystem1/*.cpp"
	}

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
        Folders.matsystem1.."Shaders/Base/**.cpp",
        Folders.public.."materialsystem1/**.cpp",
		Folders.matsystem1.."/Shaders/**.h"
	}
    includedirs {
		Folders.public.."materialsystem1"
	}

----------------------------------------------
-- Render hardware interface libraries of Eq1
group "Equilibrium 1/MatSystem/RHI"

-- base library
usage "eqRHIBaseLib"
    files {
		Folders.matsystem1.. "renderers/Shared/**.cpp",
		Folders.matsystem1.."renderers/Shared/**.h"
	}
    includedirs {
		Folders.public.."materialsystem1/",
		Folders.matsystem1.."renderers/Shared"
	}

-- empty renderer
project "eqNullRHI"
    kind "SharedLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
		"eqRHIBaseLib"
	}
    files {
		Folders.matsystem1.. "renderers/Empty/**.cpp",
		Folders.matsystem1.."renderers/Empty/**.h"
	}

-- OpenGL ES renderer
project "eqGLESRHI"
    kind "SharedLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
		"eqRHIBaseLib"
	}

	defines	{
		"USE_GLES2"
	}
	
    files {
		Folders.matsystem1.. "renderers/GL/*.cpp",
		Folders.matsystem1.. "renderers/GL/loaders/gl_loader.cpp",
		Folders.matsystem1.. "renderers/GL/loaders/glad_es3.c",
		Folders.matsystem1.."renderers/GL/**.h"
	}

	removefiles {
		Folders.matsystem1.. "renderers/GL/CGLRenderLib*",
	}
	
    includedirs {
		Folders.matsystem1.. "renderers/GL/loaders"
	}

	filter "system:Android"
		uses {
			"SDL2"
		}
		defines {
			"USE_SDL2"
		}
		links {
			"GLESv2", "EGL", "android" 
		}
		files {
			Folders.matsystem1.. "renderers/GL/CGLRenderLib_SDL.cpp"
		}

	filter "system:Windows"
		files {
			Folders.matsystem1.. "renderers/GL/CGLRenderLib_ES.cpp",
			Folders.matsystem1.. "renderers/GL/loaders/glad_egl.c",
		}

if not IS_ANDROID then
	-- Windows/Linux version of OpenGL renderer
	project "eqGLRHI"
		kind "SharedLib"
		unitybuild "on"
		uses {
			"corelib", "frameworkLib", "e2Core",
			"eqRHIBaseLib"
		}
		
		files {
			Folders.matsystem1.. "renderers/GL/*.cpp",
			Folders.matsystem1.. "renderers/GL/loaders/gl_loader.cpp",
			Folders.matsystem1.."renderers/GL/**.h"
		}

		removefiles {
			Folders.matsystem1.. "renderers/GL/CGLRenderLib_*",
		}
		
		includedirs {
			Folders.matsystem1.. "renderers/GL/loaders"
		}
		
		filter "system:Windows"
			links {
				"OpenGL32", "Gdi32"
			}
			
			files {
				Folders.matsystem1.. "renderers/GL/loaders/wgl_caps.cpp",
				Folders.matsystem1.. "renderers/GL/loaders/glad.c"
			}

		filter "system:Linux"
			files {
				Folders.matsystem1.. "renderers/GL/loaders/glx_caps.cpp",
				Folders.matsystem1.. "renderers/GL/loaders/glad.c"
			}
			links {
				"gl",	-- is that needed?
			}


    -- Direct3D9 renderer
    project "eqD3D9RHI"
        kind "SharedLib"
		unitybuild "on"
		uses {
			"corelib", "frameworkLib", "e2Core",
			"eqRHIBaseLib"
		}
        files {
			Folders.matsystem1.. "renderers/D3D9/**.cpp",
			Folders.matsystem1.."renderers/D3D9/**.h"
		}
        includedirs {
			Folders.dependency.."minidx9/include"
		}
        libdirs {
			Folders.dependency.."minidx9/lib/%{cfg.platform}"
		}
        links {
			"d3d9", "d3dx9"
		}
		
	-- Direct3D11 renderer
end

