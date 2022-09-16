project "imgui_lua"
	kind "StaticLib"
	language "C"

	uses {"imgui", "lua"}

	includedirs {
		"./",
	}
	
	files
	{
		"**.h",
		"**.cpp",
		"**.inl"
	}

usage "imgui_lua"
	includedirs {
		"./",
	}
	links "imgui_lua"
