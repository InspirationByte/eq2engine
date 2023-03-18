usage "wxWidgets"
	filter "system:Linux"
		wx_config { 
			Root=WX_DIR, 
			Debug="no",
			Static="no",
			Unicode="no", 
			Version="3.0", 
			Libs="core,aui"
		}

	filter "system:Windows"
        includedirs {
			"./include",
			"./include/msvc"
		}

		defines { "WXUSINGDLL", "wxMSVC_VERSION_AUTO", "wxMSVC_VERSION_ABI_COMPAT" }

		filter "platforms:x64"
			libdirs {
				"./lib/vc14x_x64_dll"
			}
			
		filter "platforms:x86"
			libdirs {
				"./lib/vc14x_dll"
			}
