include(DependencyPath.zlib.."/premake5.lua")
include(DependencyPath.libjpeg.."/premake5.lua")
include(DependencyPath.libogg.."/premake5.lua")
include(DependencyPath.libvorbis.."/premake5.lua")

-- android dependencies are separate
if not IS_ANDROID then
include(DependencyPath.openal.."/premake5.lua")
include(DependencyPath.libsdl.."/premake5.lua")

-- before we fix NDK 21...
include("lua54/premake5.lua")
include("sol2/premake5.lua")
end

include("wxWidgets/premake5.lua")
include("shiny/premake5.lua")

include("bullet2/premake5.lua")
include("OpenFBX/premake5.lua")

----[[
include("lua51/premake5.lua")
include("oolua_premake5.lua")
--]]