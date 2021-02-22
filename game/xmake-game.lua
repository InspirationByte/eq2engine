set_project("Games")

local FolderDriversGame = Folders.game.."DriverSyndicate/"

-----------------------------------------------
-- The Driver Syndicate
-----------------------------------------------
target("DriversEditor")
    set_group(Groups.gametools)
    set_kind("binary")
    setup_runtime_config()
    add_defines("EDITOR", "UNICODE", "SHINY_IS_COMPILED=FALSE")
    add_sources_headers(
        FolderDriversGame.."DriversEditor/**",
        FolderDriversGame.."audio/*",
        FolderDriversGame.."eqPhysics/*",
        FolderDriversGame.."objects/GameObject*",
        FolderDriversGame.."objects/ControllableObject*",
        FolderDriversGame.."objects/car*",
        FolderDriversGame.."render/**",
        FolderDriversGame.."system/CameraAnimator",
        FolderDriversGame.."system/physics",
        FolderDriversGame.."system/GameDefs",
        FolderDriversGame.."world/*")
    add_files(Folders.public.."materialsystem1/*.cpp")
    add_includedirs(FolderDriversGame)
    add_eqcore_deps()
    add_deps("fontLib", "renderUtilLib", "soundSystemLib", "egfLib", "animatingLib", "networkLib", "physicsLib")
    add_packages("bullet3", "libvorbis", "libogg")
    add_deps("shinyProfiler")
    add_wxwidgets()
    add_OpenAL()
    if is_plat("windows") then
        add_files(FolderDriversGame.."DriversEditor/**.rc")
    end

target("DriversGame")
    set_group(Groups.game)
    set_kind("binary")
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
    add_packages("bullet3", "libvorbis", "libogg")
    add_deps("shinyProfiler")
    add_OpenAL()
    add_OOLua()
    add_SDL2()
    if is_plat("windows") then
        add_files(FolderDriversGame.."DriversGame/**.rc")
        add_ldflags("/subsystem:windows")
    end
