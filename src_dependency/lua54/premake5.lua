project "lua"
	kind "StaticLib"
	language "C"
	properties	{ "thirdpartylib" }
	files {
		"./src/*.c"
	}
	
	removefiles {
		"./src/lua.c",
	}
	
	defines { "LUA_COMPAT_MODULE" } 

usage "lua"
	includedirs {
		"./src",
	}

	links { "lua" }

