-- Google ShaderC as a usage
usage "shaderc"
	includedirs {
		"./include"
	}
	libdirs { 
		"./lib",
	}

	filter "system:Windows"
		links {
			"shaderc_shared.lib",
		}
		
	filter "system:Linux"
	