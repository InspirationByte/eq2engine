group "Tools"

----------------------------------------------
-- Filesystem compression utility (fcompress)

project "fcompress"
    kind "ConsoleApp"
	properties { "tools", "app", "unitybuild",  }
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
	properties { "tools", "app", "unitybuild" }
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
	properties { "tools", "app", "unitybuild" }
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
	properties { "tools", "app", "unitybuild" }
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
project "shadercooker"
	kind "ConsoleApp"
	properties { "tools", "app", "unitybuild" }
	uses {
		"corelib", "frameworkLib",
		"e2Core",
		"dpkLib",
		"slang"
	}
	files {
		"shadercooker/*.cpp",
		"shadercooker/*.h"
	}

if ENABLE_GUI_TOOLS then
	
-- Equilibrium Graphics File manager on sysLib and ImGui (EGFMan2)
project "egfMan2"
	kind "WindowedApp"
	properties { "tools", "app", "unitybuild" }
	uses {
		"corelib", "frameworkLib", "e2Core",
		"sysLib",
		"soundSystemLib", -- required by movieLib through sysLib
		"physicsLib", "bullet2", "renderUtilLib", "animatingLib",
		"BaseShader",
		"imgui",
		"nfd"
	}
	files {
		"egfMan2/*.cpp",
		"egfMan2/*.h",
		"egfMan2/ragdoll/*.cpp",
		"egfMan2/ragdoll/*.h"
	}

	filter "system:Windows"
		files {
			"egfMan2/**.rc"
		}

end
