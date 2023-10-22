-- WebGPU as a usage
usage "wgpu-dawn"
	links {
		"dawn_native.dll.lib",
		"dawn_proc.dll.lib",
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
			"./lib/%{cfg.platform}/%{cfg.buildcfg}",
		}

