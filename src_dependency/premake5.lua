include(DependencyPath.zlib.."/premake5.lua")
include(DependencyPath.libjpeg.."/premake5.lua")

-- android dependencies are separate
if not IS_ANDROID then
include(DependencyPath.openal.."/premake5.lua")
include(DependencyPath.libsdl.."/premake5.lua")
end

include("minivorbis/premake5.lua")
include("cv_sdk/premake5.lua")
include("ffmpeg/premake5.lua")
include("wxWidgets/premake5.lua")
include("imgui/premake5.lua")
include("imgui_lua/premake5.lua")
include("lz4/premake5.lua")
include("recast/premake5.lua")

include("bullet2/premake5.lua")
include("OpenFBX/premake5.lua")

----[[
include("lua54/premake5.lua")
include("oolua_premake5.lua")
include("sol2/premake5.lua")
--]]
