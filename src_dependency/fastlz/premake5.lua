project "fastlz"
	kind "StaticLib"
	language "C"
	includedirs {
		"./"
	}

	files {
		"**.h",
		"**.c"
	}
		
usage "fastlz"
	includedirs { 
		"./"
	}
	links "fastlz"
