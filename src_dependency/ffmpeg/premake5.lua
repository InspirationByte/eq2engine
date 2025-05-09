usage "ffmpeg"
	includedirs {
		"./include",
	}

	libdirs {
		"./lib"
	}
	
	filter "system:Windows or system:linux"
		links {
			"avcodec", "avformat", "avutil",
			"swresample", "swscale",
		}

	filter "system:Windows"
		postbuildcommands { 
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/ffmpeg/bin/avcodec*.dll] %[%{!cfg.targetdir}] ",
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/ffmpeg/bin/avformat*.dll] %[%{!cfg.targetdir}]",
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/ffmpeg/bin/avutil*.dll] %[%{!cfg.targetdir}]",
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/ffmpeg/bin/swresample*.dll] %[%{!cfg.targetdir}]",
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/ffmpeg/bin/swscale*.dll] %[%{!cfg.targetdir}]",
		}
		

	-- TODO: android?