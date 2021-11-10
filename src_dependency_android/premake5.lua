-- android dependencies are separate
if IS_ANDROID then
include(DependencyPath.Android_openal.."/premake5.lua")
include(DependencyPath.Android_libsdl.."/premake5.lua")
end
