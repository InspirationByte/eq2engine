project "meshoptimizer"
	kind "StaticLib"
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