project "lz4"
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
		
usage "lz4"
	includedirs { 
		"./"
	}
	links "lz4"
