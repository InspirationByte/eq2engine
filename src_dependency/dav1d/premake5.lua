usage "dav1d"
	includedirs {
		"./include",
	}

	filter "system:Windows"
		libdirs {
			"./lib"
		}

	filter "system:Linux"
		libdirs {
			"./lib/%{cfg.platform}"
		}

	filter "system:Windows or system:linux"
		links {
			"dav1d",
		}

	filter "system:Windows"
		postbuildcommands {
			"{COPYDIR} %[%{!_WORKING_DIR}/src_dependency/dav1d/bin/dav1d*.dll] %[%{!cfg.targetdir}]",
		}

	filter "system:Linux"
		postbuildcommands {
			"{COPYFILE} %[%{!_WORKING_DIR}/src_dependency/dav1d/lib/%{!cfg.platform}/libdav1d.so.7] %[%{!cfg.targetdir}/libdav1d.so.7]",
		}

	-- TODO: android?
