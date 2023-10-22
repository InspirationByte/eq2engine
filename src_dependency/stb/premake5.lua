
project "stb"
	kind "StaticLib"

	files 
	{ 
		"*.h", 
		"*.c" 
	}
	includedirs 
	{
		"./",
	} 
					
usage "stb"
    includedirs {
		"./",
	}
	links "stb"

