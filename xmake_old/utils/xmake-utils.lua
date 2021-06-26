set_project("Equilibrium tools")

----------------------------------------------
-- Filesystem compression utility (fcompress)

target("fcompress")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("fcompress/*.cpp")
    add_headerfiles("fcompress/*.h")
    add_eqcore_deps()
    add_packages("zlib")

----------------------------------------------
-- Equilibrium Graphics File Compiler/Assembler tool (egfCA)

target("egfca")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("egfca/*.cpp")
    add_headerfiles("egfca/*.h")
    add_eqcore_deps()
    add_deps("egfLib")

----------------------------------------------
-- Animation Compiler/Assembler tool (AnimCA)

target("animca")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("animca/*.cpp")
    add_headerfiles("animca/*.h")
    add_packages("zlib")
    add_eqcore_deps()
    add_deps("egfLib")

----------------------------------------------
-- Texture atlas generator (AtlasGen)

target("atlasgen")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("atlasgen/*.cpp")
    add_headerfiles("atlasgen/*.h")
    add_packages("jpeg")
    add_eqcore_deps()

----------------------------------------------
-- Texture cooker (TexCooker)

target("texcooker")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("texcooker/*.cpp")
    add_headerfiles("texcooker/*.h")
    add_eqcore_deps()

-- Equilibrium Graphics File manager (EGFMan)
target("egfman")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("egfman/*.cpp")
    add_headerfiles("egfman/*.h")
    add_eqcore_deps()
    add_deps("fontLib", "dkPhysicsLib")
    add_packages("bullet3")
    add_wxwidgets()
    if is_plat("windows") then
        add_files("egfman/**.rc")
    end

-- Equilibrium sound system test
target("soundsystem_test")
    set_group(Groups.tools)
    set_kind("binary")
    setup_runtime_config()
    add_files("soundsystem_test/*.cpp")
    add_headerfiles("soundsystem_test/*.h")
    add_eqcore_deps()
    add_deps("fontLib", "renderUtilLib", "soundSystemLib")
    add_wxwidgets()
    if is_plat("windows") then
        add_files("soundsystem_test/**.rc")
    end
