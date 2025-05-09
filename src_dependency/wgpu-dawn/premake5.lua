-- WebGPU as a usage
usage "wgpu-dawn"
	includedirs {
		"./include"
	}
	libdirs { 
		"./lib/%{cfg.platform}",
	}
	links {
		"webgpu_dawn",
	}
	filter "system:Windows"
		postbuildcommands { 
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/wgpu-dawn/lib/%{!cfg.platform}/webgpu_dawn.dll] %[%{!cfg.targetdir}/webgpu_dawn.dll]"
		}
	filter "system:Linux"
		postbuildcommands { 
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/wgpu-dawn/lib/%{!cfg.platform}/libwebgpu_dawn.so] %[%{!cfg.targetdir}/libwebgpu_dawn.so]"
		}
