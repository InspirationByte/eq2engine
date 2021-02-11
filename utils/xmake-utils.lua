set_xmakever("2.1.0")
set_project("Equilibrium 2 tools")

----------------------------------------------
-- Equilibrium Graphics File Compiler/Assembler tool (egfCA)

target("egfca")
    set_group(Groups.tools)
    set_kind("binary")
    add_files("egfca/*.cpp")
    add_headerfiles("egfca/*.h")
    add_eqcore_deps()
    add_deps("egfLib")

----------------------------------------------
-- Animation Compiler/Assembler tool (AnimCA)

target("animca")
    set_group(Groups.tools)
    set_kind("binary")
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
    add_files("atlasgen/*.cpp")
    add_headerfiles("atlasgen/*.h")
    add_packages("jpeg")
    add_eqcore_deps()

----------------------------------------------
-- Texture cooker (TexCooker)

target("texcooker")
    set_group(Groups.tools)
    set_kind("binary")
    add_files("texcooker/*.cpp")
    add_headerfiles("texcooker/*.h")
    add_eqcore_deps()

-- Equilibrium Graphics File manager (EGFMan)
target("egfman")
    set_group(Groups.tools)
    set_kind("binary")
    add_files("egfman/*.cpp")
    add_headerfiles("egfman/*.h")
    add_includedirs(Folders.dependency.."wxWidgets/include")
    add_includedirs(Folders.shared_game)
    add_eqcore_deps()
    add_deps("animatingLib", "fontLib")
    if is_plat("windows") then
        if is_arch("x64") then
            add_linkdirs(Folders.dependency.."wxWidgets/lib/vc_x64_lib")
        else
            add_linkdirs(Folders.dependency.."wxWidgets/lib/vc_lib")
        end
    else
        error("FIXME: setup other platforms!")
    end

    
