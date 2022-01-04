project "imgui"
	kind "StaticLib"
	language "C"

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

