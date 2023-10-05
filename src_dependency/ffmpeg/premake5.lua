usage "ffmpeg"
	includedirs {
		"./include",
	}

	libdirs {
		"./lib"
	}
	
	filter "system:windows or system:linux"
		links {
			"avcodec", "avformat", "avutil",
			"swresample", "swscale",
		}
	-- TODO: android?