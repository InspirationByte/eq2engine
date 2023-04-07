group "Tools"

----------------------------------------------
-- Filesystem compression utility (fcompress)

project "fcompress"
    kind "ConsoleApp"
	unitybuild "on"
    uses {
		"lz4",
		"corelib", "frameworkLib",
		"e2Core" 
	}
    files {
		"fcompress/*.cpp",
		"fcompress/*.h"
	}
    

----------------------------------------------
-- Equilibrium Graphics File Compiler/Assembler tool (egfCA)

project "egfca"
    kind "ConsoleApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib",
		"e2Core",
		"egflib"
	}
    files {
		"egfca/*.cpp",
		"egfca/*.h"
	}

----------------------------------------------
-- Animation Compiler/Assembler tool (AnimCA)

project "animca"
    kind "ConsoleApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib",
		"e2Core", "egflib"
	}
    files {
		"animca/*.cpp",
		"animca/*.h"
	}

----------------------------------------------
-- Texture atlas generator (AtlasGen)

project "atlasgen"
    kind "ConsoleApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib",
		"e2Core"
	}
    files {
		"atlasgen/*.cpp",
		"atlasgen/*.h"
	}


----------------------------------------------
-- Texture cooker (TexCooker)

project "texcooker"
    kind "ConsoleApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib",
		"e2Core" 
	}
    files {
		"texcooker/*.cpp",
		"texcooker/*.h"
	}

if ENABLE_GUI_TOOLS then
	
-- Equilibrium Graphics File manager (EGFMan)
project "egfman"
    kind "WindowedApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib",
		"e2Core", 
		"fontLib", "physicsLib", "dkPhysicsLib", "wxWidgets"
	}
    files {
		"egfman/*.cpp",
		"egfman/*.h"
	}

    filter "system:Windows"
        files {
			"egfman/**.rc"
		}

-- Equilibrium sound system test
project "soundsystem_test"
    kind "WindowedApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib",
		"e2Core", 
		"fontLib", "renderUtilLib", "soundSystemLib", "wxWidgets"
	}
    files {
		"soundsystem_test/*.cpp",
		"soundsystem_test/*.h"
	}
    filter "system:Windows"
        files {
			"soundsystem_test/**.rc"
		}
		linkoptions {
			"/SAFESEH:NO", -- Image Has Safe Exception Handers: No. Because of openal-soft
		}
		
end