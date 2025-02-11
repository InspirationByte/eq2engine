project "minivorbis"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	language "C"
	includedirs {
		"./"
	}

	files {
		"**.h",
		"**.c"
	}

usage "minivorbis"	
	includedirs {
		"./"
	}
	links "minivorbis"