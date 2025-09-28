usage "slang"
	includedirs {
		"./include"
	}
	libdirs { 
		"./lib",
	}
	links {
		"slang"
	}
	filter "system:Windows"
		postbuildcommands { 
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/slang/bin/slang.dll] %[%{!cfg.targetdir}/slang.dll]",
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/slang/bin/slang-glslang.dll] %[%{!cfg.targetdir}/slang-glslang.dll]",
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/slang/bin/slang-glsl-module.dll] %[%{!cfg.targetdir}/slang-glsl-module.dll]",
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/slang/bin/slang-llvm.dll] %[%{!cfg.targetdir}/slang-llvm.dll]",
			--"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/slang/bin/slang-rt.dll] %[%{!cfg.targetdir}/slang-rt.dll]",
		}
	--filter "system:Linux"
	--	postbuildcommands { 
	--		"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/wgpu-dawn/lib/%{!cfg.platform}/libwebgpu_dawn.so] %[%{!cfg.targetdir}/libwebgpu_dawn.so]"
	--	}
