project "nfd"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	language "C++"
	includedirs {
		"./include"
	}

	files {
		"include/**.h",
		"include/**.hpp"
	}

	filter "system:Windows"
		files {
			"src/nfd_win.cpp"
		}

	-- xdg-desktop-portal backend, no window parenting (NFD_X11 / NFD_WAYLAND are off)
	filter "system:Linux"
		files {
			"src/nfd_portal.cpp"
		}
		includedirs {
			"/usr/include/dbus-1.0",
			"/usr/lib/x86_64-linux-gnu/dbus-1.0/include",
			"/usr/lib64/dbus-1.0/include"
		}
		defines {
			"NFD_PORTAL"
		}

usage "nfd"
	includedirs {
		"./include"
	}
	links "nfd"

	filter "system:Windows"
		links { "ole32", "uuid", "shell32" }

	filter "system:Linux"
		links { "dbus-1" }
		defines {
			"NFD_PORTAL"
		}
