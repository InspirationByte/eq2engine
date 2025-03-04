include "zlib"

-- android dependencies are separate
if not IS_ANDROID then
include(DependencyPath.openal)
include(DependencyPath.libsdl)
include "shaderc"
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
include "minivorbis"
include "ffmpeg"
include "lua54"

include "meshoptimizer"
include "recast"
include "bullet2"
include "OpenFBX"
