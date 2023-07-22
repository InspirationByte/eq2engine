usage "ffmpeg"
	links {
		"avcodec", "avformat", "avutil",
		"swresample", "swscale",
	}
	
	filter "system:Linux"
		-- TODO dev package
		
	filter "system:Windows"
        includedirs {
			"./include",
		}

		filter "platforms:x64"
			libdirs {
				"./lib"
			}
			
		filter "platforms:x86"
			libdirs {
				"./lib"
			}
