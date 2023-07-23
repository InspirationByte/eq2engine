usage "ffmpeg"
	links {
		"avcodec", "avformat", "avutil",
		"swresample", "swscale",
	}

	filter "system:Linux"
		-- TODO: build script to install
		-- 		RPM: ffmpeg-free
		-- 		DEB: libavcodec-dev libavformat-dev libavutil-dev libswresample-dev libswscale-dev

		includedirs {
			"/usr/include/ffmpeg",
		}
		
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
