group "Dependencies"

include "zlib"

-- android dependencies are separate
if IS_ANDROID then
	include "android"
else
	include(DependencyPath.openal)
	include(DependencyPath.libsdl)
	include "wxWidgets"
end

if ENABLE_TESTS then
	include "gtest"
end

include "stb"
include "lz4"
include "cv_sdk"
include "LivePP"

include "imgui"
include "imgui_lua"

include "wgpu-dawn"
include "nvrhi"
include "slang"
include "minivorbis"
include "ffmpeg"
include "lua54"

include "meshoptimizer"
include "recast"
include "bullet2"
include "OpenFBX"