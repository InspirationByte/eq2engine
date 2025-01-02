group "Tools"

----------------------------------------------
-- Filesystem compression utility (fcompress)

project "fcompress"
    kind "ConsoleApp"
	properties { "unitybuild", "tools" }
    uses {
		"corelib", "frameworkLib", 
		"e2Core", 
		"dpkLib",
		"lz4",
		"zlib"
	}
    files {
		"fcompress/*.cpp",
		"fcompress/*.h"
	}
    

----------------------------------------------
-- Equilibrium Graphics File Compiler/Assembler tool (egfCA)

project "egfca"
    kind "ConsoleApp"
	properties { "unitybuild", "tools" }
    uses {
		"corelib", "frameworkLib",
		"e2Core",
		"egfLib"
	}
    files {
		"egfca/*.cpp",
		"egfca/*.h"
	}

----------------------------------------------
-- Animation Compiler/Assembler tool (AnimCA)

project "animca"
    kind "ConsoleApp"
	properties { "unitybuild", "tools" }
    uses {
		"corelib", "frameworkLib",
		"e2Core", "egfLib", "studioLib"
	}
    files {
		"animca/*.cpp",
		"animca/*.h"
	}

----------------------------------------------
-- Texture cooker (TexCooker)

project "texcooker"
    kind "ConsoleApp"
	properties { "unitybuild", "tools" }
    uses {
		"corelib", "frameworkLib",
		"e2Core"
	}
    files {
		"texcooker/*.cpp",
		"texcooker/*.h"
	}
	
----------------------------------------------
-- Shader cooker

local VULKAN_SDK_LOCATION = os.getenv("VULKAN_SDK")

if VULKAN_SDK_LOCATION ~= nil and VULKAN_SDK_LOCATION ~= "" then
	project "shadercooker"
		kind "ConsoleApp"
		properties { "unitybuild", "tools" }
		uses {
			"corelib", "frameworkLib",
			"e2Core",
			"dpkLib",
			"shaderc"
		}
		files {
			"shadercooker/*.cpp",
			"shadercooker/*.h"
		}
else
	print("WARNING: Vulkan SDK is missing (env VULKAN_SDK not found), ShaderCooker will not be built")
end

if ENABLE_GUI_TOOLS then
	
-- Equilibrium Graphics File manager (EGFMan)
project "egfman"
    kind "WindowedApp"
	properties { "unitybuild", "tools" }
    uses {
		"corelib", "frameworkLib", "e2Core",
		"fontLib", "physicsLib", "dkPhysicsLib", "renderUtilLib", "animatingLib",
		"wxWidgets"
	}
    files {
		"egfman/*.cpp",
		"egfman/*.h"
	}

    filter "system:Windows"
        files {
			"egfman/**.rc"
		}

end
