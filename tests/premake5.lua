group "Components"

project "testsCommonLib"
    kind "StaticLib"
	unitybuild "on"
	uses {
		"coreLib", "frameworkLib", "e2Core", "gtest",
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
	unitybuild "on"
    uses {
		"corelib", "frameworkLib", 
		"e2Core", 
		"testsCommonLib",
	}
    files {
		"ds/*.cpp",
		"ds/*.h"
	}

project "scripting_tests"
    kind "ConsoleApp"
	unitybuild "on"
    uses {
		"corelib", "frameworkLib", 
		"e2Core", 
		"testsCommonLib",
		"scriptLib",
		"shared_engine"
	}
    files {		
		"scripting/*.cpp",
		"scripting/*.h"
	}
