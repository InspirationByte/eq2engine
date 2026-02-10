-- SDL2 as a usage
usage "SDL2"
	links {
		"SDL2",
	}
	
	filter "system:Linux"
		includedirs {
            "/usr/include/SDL2"
        }

	filter "system:Windows"
		includedirs {
			"./include"
		}
		libdirs { 
			"./lib/%{cfg.platform}",
		}
	filter "system:Windows"
		postbuildcommands {
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/SDL2/lib/%{!cfg.platform}/SDL2.dll] %[%{!cfg.targetdir}/SDL2.dll]"
		}

usage "SDL2_main"
	links {
		"SDL2main"
	}
