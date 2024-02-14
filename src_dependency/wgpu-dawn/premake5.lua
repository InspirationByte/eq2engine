-- WebGPU as a usage
usage "wgpu-dawn"
	includedirs {
		"./include"
	}
	libdirs { 
		"./lib/%{cfg.platform}",
	}

	links {
		"webgpu",
	}

