usage "ffmpeg"
	links {
		"avcodec", "avformat", "avutil",
		"swresample", "swscale",
	}

	includedirs {
		"./include",
	}

	libdirs {
		"./lib"
	}

	--filter { "not system:windows" }
   	--	postbuildcommands { 
	--		"cp %{cfg.libdirs[1]}/* %{cfg.targetdir}"
	--	}