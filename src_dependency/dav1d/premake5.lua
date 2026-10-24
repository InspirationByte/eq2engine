usage "dav1d"
	includedirs {
		"./include",
	}

	filter "system:Windows"
		libdirs {
			"./lib"
		}

	filter "system:Windows or system:linux"
		links {
			"dav1d",
		}

	filter "system:Windows"
		postbuildcommands {
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/dav1d/bin/dav1d*.dll] %[%{!cfg.targetdir}]",
		}

	-- TODO: linux build, android?
