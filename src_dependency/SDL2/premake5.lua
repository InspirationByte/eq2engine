-- SDL2 as a usage
usage "SDL2"
	links {
		"SDL2",
	}
	
	filter "system:linux"
		includedirs {
            "/usr/include/SDL2"
        }

	filter "system:windows"
		includedirs {
			"./include"
		}
		libdirs { 
			"./lib/%{cfg.platform}",
		}