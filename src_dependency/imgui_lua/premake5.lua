project "imgui_lua"
	kind "StaticLib"
	properties	{ "thirdpartylib" }

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
