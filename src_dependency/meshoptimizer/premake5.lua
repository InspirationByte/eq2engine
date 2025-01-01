project "meshoptimizer"
	kind "StaticLib"
	properties	{ "thirdpartylib" }
	language "C"
	includedirs {
		"./src"
	}

	files {
		"./src/**.h",
		"./src/**.cpp"
	}

usage "meshoptimizer"	
	includedirs {
		"./src"
	}
	links "meshoptimizer"