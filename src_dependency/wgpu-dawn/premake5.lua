-- WebGPU as a usage
usage "wgpu-dawn"
	includedirs {
		"./include"
	}
	libdirs { 
		"./lib/%{cfg.platform}",
	}

	filter "system:Windows"
		links {
			"webgpu.lib",
		}
		
	filter "system:Linux"
		links {
			"dawn_native",
			"webgpu_dawn",
		}
	
