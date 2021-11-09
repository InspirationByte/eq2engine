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