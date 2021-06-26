project "bullet2"
	kind "StaticLib"
	language "C"

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
