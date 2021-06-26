set_project("Games")

local FolderDriversGame = Folders.game.."DriverSyndicate/"
local GroupDriversGame = Groups.game.."/DriverSyndicate"

add_requires("luajit")

--[[
add_requires("luajit", "sol2")
add_requireconfs("sol2", {
    configs = {
        includes_lua = false,
    }})
]]
function add_OOLua()
    add_includedirs(Folders.dependency.."oolua/include")
    if is_arch("x64") then
        add_linkdirs(Folders.dependency.."oolua/libs/x64")
    else
        add_linkdirs(Folders.dependency.."oolua/libs/x86")
    end
    add_links("oolua")
    add_packages("luajit")
end

-----------------------------------------------
-- The Driver Syndicate
-----------------------------------------------
target("DriversEditor")
    set_group(GroupDriversGame)
    set_kind("binary")
    set_basename("LevelEditor")
    setup_runtime_config()
    add_defines("EDITOR", "UNICODE", "SHINY_IS_COMPILED=FALSE")
    add_sources_headers(
        FolderDriversGame.."DriversEditor/**",
        FolderDriversGame.."audio/*",
        FolderDriversGame.."eqPhysics/*",
        FolderDriversGame.."objects/GameObject*",
        FolderDriversGame.."objects/ControllableObject*",
        FolderDriversGame.."objects/car*",
		FolderDriversGame.."objects/animating*",
        FolderDriversGame.."render/**",
        FolderDriversGame.."system/CameraAnimator",
        FolderDriversGame.."system/GamePhysics",
        FolderDriversGame.."system/GameDefs",
        FolderDriversGame.."world/*")
    add_files(Folders.public.."materialsystem1/*.cpp")
    add_includedirs(FolderDriversGame)
    add_eqcore_deps()
    add_deps("fontLib", "renderUtilLib", "soundSystemLib", "egfLib", "animatingLib", "networkLib", "physicsLib")
    add_packages("libvorbis", "libogg")
    add_deps("shinyProfiler", "bullet2")
    add_wxwidgets()
    add_OpenAL()
    if is_plat("windows") then
        add_files(FolderDriversGame.."DriversEditor/**.rc")
    end

target("DriversGame")
    set_group(GroupDriversGame)
    set_kind("binary")
    set_basename("DrvSyn")
    setup_runtime_config()
    add_sources_headers(
        FolderDriversGame.."DriversGame/*",
        FolderDriversGame.."audio/*",
        FolderDriversGame.."eqPhysics/*",
        FolderDriversGame.."equi/*",
        FolderDriversGame.."luabinding/*",
        FolderDriversGame.."objects/**",
        FolderDriversGame.."render/**",
        FolderDriversGame.."session/*",
        FolderDriversGame.."system/*",
        FolderDriversGame.."world/*")
    add_files(Folders.public.."materialsystem1/*.cpp")
    add_includedirs(FolderDriversGame)
    add_eqcore_deps()
    add_deps("fontLib", "renderUtilLib", "sysLib", "egfLib", "animatingLib", "networkLib", "equiLib", "physicsLib")
    -- add OpenAL, ogg
    add_packages("libvorbis", "libogg", "libsdl")
    add_deps("shinyProfiler", "bullet2")
    add_OpenAL()
    add_OOLua()
    if is_plat("windows") then
        add_files(FolderDriversGame.."DriversGame/**.rc")
        add_ldflags("/subsystem:windows")
    end
