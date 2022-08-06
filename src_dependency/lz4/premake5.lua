project "lz4"
	kind "StaticLib"
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
