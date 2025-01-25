group "Components"

project "testsCommonLib"
    kind "StaticLib"
	properties { "unitybuild" }
	uses {
		"coreLib", "frameworkLib", "e2Core", "gtest"
	}
    files {
		"tests_common.cpp",
		"tests_common.h"
	}
	
usage "testsCommonLib"
	links "testsCommonLib"
	includedirs { "./" }
	
----------------------------------------------------------

group ""

project "ds_tests"
    kind "ConsoleApp"
	properties { "unitybuild" }
    uses {
		"corelib", "frameworkLib", 
		"e2Core", 
		"testsCommonLib",
		"gtest"
	}
    files {
		"ds/*.cpp",
		"ds/*.h"
	}

project "scripting_tests"
    kind "ConsoleApp"
	properties { "unitybuild" }
    uses {
		"corelib", "frameworkLib", 
		"e2Core", 
		"testsCommonLib",
		"scriptLib",
		"shared_engine",
		"gtest",
		"lua"
	}
    files {		
		"scripting/*.cpp",
		"scripting/*.h"
	}
