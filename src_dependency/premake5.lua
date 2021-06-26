include(DependencyPath.zlib.."/premake5.lua")
include(DependencyPath.libjpeg.."/premake5.lua")
include(DependencyPath.libogg.."/premake5.lua")
include(DependencyPath.libvorbis.."/premake5.lua")
include(DependencyPath.openal.."/premake5.lua")
include(DependencyPath.libsdl.."/premake5.lua")
include("wxWidgets/premake5.lua")
include("shiny/premake5.lua")
include("bullet2/premake5.lua")
include("lua51/premake5.lua")

usage "oolua"
    includedirs {
		"./oolua/include"
	}
	links "oolua"
	
    filter "platforms:x64"
        libdirs {
			"./oolua/libs/x64"
		}
		
	filter "platforms:x86"
        libdirs {
			"./oolua/libs/x86"
		}

    