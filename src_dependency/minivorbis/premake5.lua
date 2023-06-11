project "minivorbis"
	kind "StaticLib"
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