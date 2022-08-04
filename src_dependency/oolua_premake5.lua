--
-- OOLua binding premake script
--
project "oolua"
	kind "StaticLib"

	uses { "lua" }
	-- TODO: check here if boilerplate headers are generated

	files 
	{ 
		"oolua/include/*.h", 
		"oolua/src/*.cpp" 
	}
	includedirs 
	{
		"oolua/include/lua/",
		"oolua/include/",
	} 
					
usage "oolua"
    includedirs {
		"oolua/include/lua/",
		"oolua/include/",
	}
	links "oolua"

