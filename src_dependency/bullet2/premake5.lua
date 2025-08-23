project "bullet2"
	kind "StaticLib"
	properties	{ "thirdpartylib" }

	includedirs
	{
		"./"
	}

	files
	{
		"**.h",
		"**.c",
		"**.cpp"
	}
	
	defines
	{
		"BT_THREADSAFE=1"
	}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		
usage "bullet2"
	includedirs { 
		"./"
	}
	links "bullet2"
