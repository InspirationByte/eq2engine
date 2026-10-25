project "nestegg"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	language "C"
	includedirs {
		"./include"
	}

	files {
		"include/**.h",
		"src/**.c"
	}

usage "nestegg"
	includedirs {
		"./include"
	}
	links "nestegg"
