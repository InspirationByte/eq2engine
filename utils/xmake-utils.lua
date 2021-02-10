set_xmakever("2.1.0")
set_project("Equilibrium 2 tools")

----------------------------------------------
-- Equilibrium Graphics File Compiler/Assembler tool (egfCA)

target("egfca")
    set_group(Groups.tools)
    set_kind("binary")
    add_files("egfca/*.cpp")
    add_headerfiles("egfca/*.h")
    add_files(Folders.public.."egf/**.cpp", Folders.public.."egf/**.c")
    add_headerfiles(Folders.public.."egf/**.h")
    add_eqcore_deps()

----------------------------------------------
-- Animation Compiler/Assembler tool (AnimCA)

target("animca")
    set_group(Groups.tools)
    set_kind("binary")
    add_files("animca/*.cpp")
    add_headerfiles("animca/*.h")
    add_files(Folders.public.."egf/dsm_*.cpp")
    add_headerfiles(Folders.public.."egf/dsm_*.h")
    add_packages("zlib")
    add_eqcore_deps()

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