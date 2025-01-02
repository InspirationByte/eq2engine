project "openfbx"
	kind        "StaticLib"
	properties	{ "thirdpartylib" }

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