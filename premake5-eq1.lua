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

-- OpenGL ES renderer
project "eqGLESRHI"
    kind "SharedLib"
	unitybuild "on"
	uses {
		"corelib", "frameworkLib", "e2Core",
		"eqRHIBaseLib"
	}

	defines	{
		"USE_GLES2",
		"EQRHI_GL",
		"RENDERER_TYPE=1"
	}
	
    files {
		Folders.matsystem1.. "Renderers/GL/*.cpp",
		Folders.matsystem1.. "Renderers/GL/loaders/gl_loader.cpp",
		Folders.matsystem1.. "Renderers/GL/loaders/glad_es3.c",
		Folders.matsystem1.."Renderers/GL/**.h"
	}

	removefiles {
		Folders.matsystem1.. "Renderers/GL/GLRenderLibrary*",
	}
	
    includedirs {
		Folders.matsystem1.. "Renderers/GL/loaders"
	}

	filter "system:Android"
		links {
			"GLESv2", "EGL", "android" 
		}
		files {
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_EGL.cpp",
		}

	filter "system:Windows"
		files {
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_EGL.cpp",
			Folders.matsystem1.. "Renderers/GL/loaders/glad_egl.c",
		}

	filter "system:Linux"
		links {
			"X11", "GLESv2", "EGL", "wayland-egl"
		}
		files {
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_EGL.cpp",
			Folders.matsystem1.. "Renderers/GL/loaders/glad_egl.c",
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

	defines{
		"EQRHI_GL",
		"RENDERER_TYPE=1"
	}
	
	files {
		Folders.matsystem1.. "Renderers/GL/*.cpp",
		Folders.matsystem1.. "Renderers/GL/loaders/gl_loader.cpp",
		Folders.matsystem1.."Renderers/GL/**.h"
	}

	removefiles {
		Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_*",
	}
	
	includedirs {
		Folders.matsystem1.. "Renderers/GL/loaders"
	}
	
	filter "system:Windows"
		links {
			"OpenGL32", "Gdi32", "Dwmapi"
		}
		
		files {
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_EGL.cpp",
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_WGL.cpp",
			Folders.matsystem1.. "Renderers/GL/loaders/wgl_caps.cpp",
			Folders.matsystem1.. "Renderers/GL/loaders/glad.c",
			Folders.matsystem1.. "Renderers/GL/loaders/glad_egl.c",
		}

	filter "system:Linux"
		files {
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_GLX.cpp",
			Folders.matsystem1.. "Renderers/GL/GLRenderLibrary_EGL.cpp",
			Folders.matsystem1.. "Renderers/GL/loaders/glx_caps.cpp",
			Folders.matsystem1.. "Renderers/GL/loaders/glad.c",
			Folders.matsystem1.. "Renderers/GL/loaders/glad_egl.c",
		}
		links {
			"EGL", "GL", "GLU", "wayland-egl", "X11", "Xxf86vm", "Xext"
		}

	if os.target() == "windows" then
	
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
	
		-- Direct3D9 renderer
		project "eqD3D9RHI"
			kind "SharedLib"
			unitybuild "on"
			uses {
				"corelib", "frameworkLib", "e2Core",
				"eqRHIBaseLib"
			}
			defines{
				"EQRHI_D3D9",
				"RENDERER_TYPE=2"
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
	end
end

