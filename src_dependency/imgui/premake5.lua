project "imgui"
	kind "StaticLib"
	properties	{ "thirdpartylib" }

	includedirs {
		"./",
	}
	
	files
	{
		"**.h",
		"**.cpp"
	}

usage "imgui"
	links { "imgui" }
	includedirs {
		"./",
	}

