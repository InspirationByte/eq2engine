project "zlib-lib"
	language    "C"
	kind        "StaticLib"
	defines     { "N_FSEEKO" }
	warnings    "off"

	files
	{
		"**.h",
		"**.c"
	}

	excludes
	{
		"contrib/**.*",
		"examples/**.*",
		"old/**.*",
		"nintendods/**.*",
		"test/**.*",
	}

	filter "system:windows"
		defines { "_WINDOWS", "ZLIB_WINAPI" }

	filter "system:not windows"
		defines { 'HAVE_UNISTD_H' }