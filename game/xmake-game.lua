set_project("Games")

local FolderDriversGame = Folders.game.."DriverSyndicate/"

-----------------------------------------------
-- The Driver Syndicate
-----------------------------------------------
target("DriversEditor")
    set_group(Groups.gametools)
    set_kind("binary")
    setup_runtime_config()
    add_files(FolderDriversGame.."DriversEditor/*.cpp")
    add_headerfiles(FolderDriversGame.."DriversEditor/*.h")
    add_files(FolderDriversGame.."world/*.cpp")
    add_headerfiles(FolderDriversGame.."world/*.h")
    add_eqcore_deps()
    add_deps("fontLib", "renderUtilLib", "soundSystemLib")
    add_wxwidgets()
    if is_plat("windows") then
        add_files(FolderDriversGame.."DriversEditor/**.rc")
    end

target("DriversGame")
    set_group(Groups.game)
    set_kind("binary")
    setup_runtime_config()
    add_sources_headers(FolderDriversGame.."DriversGame")
    add_sources_headers(FolderDriversGame.."audio")
    add_sources_headers(FolderDriversGame.."eqPhysics")
    add_sources_headers(FolderDriversGame.."equi")
    add_sources_headers(FolderDriversGame.."luabinding")
    add_sources_headers(FolderDriversGame.."objects", true)
    add_sources_headers(FolderDriversGame.."render", true)
    add_sources_headers(FolderDriversGame.."session")
    add_sources_headers(FolderDriversGame.."system")
    add_sources_headers(FolderDriversGame.."world")
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
