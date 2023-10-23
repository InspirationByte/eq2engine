-- WebGPU as a usage
usage "wgpu-dawn"
	includedirs {
		"./include"
	}
	libdirs { 
		"./lib/%{cfg.platform}/%{cfg.buildcfg}",
	}

	filter "system:Windows"
		links {
			"webgpu.lib",
		}
		
	filter "system:Linux"
	