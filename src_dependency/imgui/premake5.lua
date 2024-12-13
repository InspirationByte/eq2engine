project "imgui"
	kind "StaticLib"

	includedirs {
		"./",
	}
	
	files
	{
		"**.h",
		"**.cpp"
	}

usage "imgui"
	includedirs {
		"./",
	}

