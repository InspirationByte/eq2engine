project "openfbx"
	language    "C"
	kind        "StaticLib"

	includedirs {
		"./src/"
	}

	files
	{
		"./src/**.h",
		"./src/**.cpp",
		"./src/**.c"
	}
		
usage "openfbx"
	includedirs { 
		"./src/"
	}
	links "openfbx"