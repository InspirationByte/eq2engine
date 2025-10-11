group "Tools"

----------------------------------------------
-- Filesystem compression utility (fcompress)

project "fcompress"
    kind "ConsoleApp"
	properties { "unitybuild", "tools", "app" }
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
	properties { "unitybuild", "tools", "app" }
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
	properties { "unitybuild", "tools", "app" }
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
	properties { "unitybuild", "tools", "app" }
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
	
-- Equilibrium Graphics File manager (EGFMan)
project "egfman"
    kind "WindowedApp"
	properties { "unitybuild", "tools", "app" }
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
