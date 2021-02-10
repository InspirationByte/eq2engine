set_xmakever("2.1.0")
set_project("Equilibrium 2 tools")

----------------------------------------------
-- Animation Compiler/Assembler tool (AnimCA)

target("animca")
    set_group(Groups.tools)
    set_kind("binary")
    add_files("animca/*.cpp")
    add_files(Folders.public.."egf/dsm_*.cpp")
    add_headerfiles(Folders.public.."egf/dsm_*.h")
    add_headerfiles("animca/*.h")
    add_packages("zlib")
    add_eqcore_deps()
