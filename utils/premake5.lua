group "Tools"

----------------------------------------------
-- Filesystem compression utility (fcompress)

project "fcompress"
    kind "ConsoleApp"
    uses {
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
    uses {
		"corelib", "frameworkLib",
		"e2Core" 
	}
    files {
		"texcooker/*.cpp",
		"texcooker/*.h"
	}

-- Equilibrium Graphics File manager (EGFMan)
project "egfman"
    kind "WindowedApp"
    uses {
		"corelib", "frameworkLib",
		"e2Core", 
		"fontLib", "dkPhysicsLib", "wxWidgets"
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
		
-- tree generator for DrvSyn
project "treegen"
    kind "WindowedApp"
    uses {
		"corelib", "frameworkLib",
		"e2Core", 
		"fontLib", "dkPhysicsLib", "wxWidgets"
	}
	defines { "TREEGEN" }
    files {
		"treegen/*.cpp",
		"treegen/*.h"
	}
    filter "system:Windows"
        files {
			"treegen/**.rc"
		}
