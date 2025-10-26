group "Core"

----------------------------------------------
-- e2Core DLL

project "e2Core"
    kind "SharedLib"
	properties { 
		"unitybuild", "live_pp", "concurrency_vis"
	}
    uses {
		"coreLib", "frameworkLib", "dpkLib"
	}
    files {
        "e2core/**",
		PUBLIC_DIR.."/core/**"
	}
	vpaths {
		["*"] = "e2core/*",
		["Public Headers"] = PUBLIC_DIR.."/core/*.h",
	}
	
	defines { "CORE_INTERFACE_EXPORT", "COREDLL_EXPORT" }

	filter "system:Windows"
		linkoptions { "-IGNORE:4217,4286" }	-- disable few linker warnings

    filter "system:android"
        files {
			"core/android_libc/**",
		}

    filter "system:Windows"
        links { "User32", "DbgHelp", "Advapi32" }
		

----------------------------------------------
-- Material System and rendering

if ENABLE_MATSYSTEM then

group "MatSystem"

-- Base Shader
project "BaseShader"
    kind "StaticLib"
	uses { "public" }
    files {
		PUBLIC_DIR.."/materialsystem1/BaseShader*",
	}

-- Material System DLL
project "eqMatSystem"
    kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"BaseShader"
	}
    files {
        "materialsystem1/*",
		"materialsystem1/Renderers/*.h",
		PUBLIC_DIR.."/materialsystem1/*.h",
	}
	vpaths {
		["*"] = "materialsystem1/*",
		["Public Headers"] = PUBLIC_DIR.."/materialsystem1/*.h",
	}

-- Default Shaders DLL
project "eqBaseShaders"
    kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"BaseShader"
	}
    files {
        "materialsystem1/Shaders/*",
        "materialsystem1/Shaders/Base/**",
	}
    includedirs {
		PUBLIC_DIR.."/materialsystem1"
	}

----------------------------------------------
-- Render hardware interface libraries of Eq1
group "MatSystem/RHI"

-- Base Renderer Library
usage "eqRHIBaseLib"
    files {
		"materialsystem1/Renderers/Shared/**",
		"materialsystem1/Renderers/*",
		PUBLIC_DIR.."/materialsystem1/renderers/**.h"
	}
    includedirs {
		"materialsystem1/Renderers/Shared",
		PUBLIC_DIR.."/materialsystem1/"
	}
	vpaths {
		["*"] = "materialsystem1/Renderers/*",
		["Public Headers"] = PUBLIC_DIR.."/materialsystem1/renderers/**.h"
	}

-- NULL renderer
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
		"materialsystem1/Renderers/Empty/**",
	}
	
-- D3D11/D3D12/Vulkan renderer
project "eqNVRHI"
	kind "SharedLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core",
		"eqRHIBaseLib", "nvrhi", "nvrhi-validation"
	}
	defines{
		"EQRHI_NVRHI",
		"RENDERER_TYPE=1",
		"VK_USE_PLATFORM_WIN32_KHR"
	}
	files {
		"materialsystem1/Renderers/NVRHI/**.cpp",
		"materialsystem1/Renderers/NVRHI/**.h"
	}
	removefiles {
		"materialsystem1/Renderers/NVRHI/**D3D11.cpp",
		"materialsystem1/Renderers/NVRHI/**D3D11.h"
	}
	
-- WebGPU renderer
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
		"RENDERER_TYPE=2"
	}
	files {
		"materialsystem1/Renderers/WGPU/**",
	}


end -- ENABLE_MATSYSTEM