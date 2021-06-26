usage "wxWidgets"
	filter "system:Windows"
        includedirs {
			"./include",
			"./include/msvc"
		}

		filter "platforms:x64"
			libdirs {
				"./lib/vc_x64_lib"
			}
			
		filter "platforms:x86"
			libdirs {
				"./lib/vc_lib"
			}
