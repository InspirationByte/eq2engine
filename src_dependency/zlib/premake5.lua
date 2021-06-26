project "zlib"
	language    "C"
	kind        "StaticLib"
	defines     { "N_FSEEKO" }
	warnings    "off"

	includedirs {
		"./"
	}

	files
	{
		"**.h",
		"**.c"
	}
	
	removefiles {
		"examples/**",
		"contrib/**",
		"test/**"
	}

	filter "system:windows"
		defines { "_WINDOWS" }

	filter "system:not windows"
		defines { 'HAVE_UNISTD_H' }
		
usage "zlib"
	includedirs { 
		"./"
	}
	links "zlib"