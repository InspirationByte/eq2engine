set_xmakever("2.1.0")
set_project("Equilibrium 2")

-- default configurations
add_rules("mode.debug", "mode.release")

set_languages("c++14", "c98")
set_arch("x64") -- TODO: x86 builds too
set_targetdir("bin/$(arch)")

-- some packages
add_requires("zlib", "libjpeg", "bullet3")
add_requireconfs("bullet3", {
    configs = {
        shared = false
    }})
-----------------------------------------------------

-- default configuration capabilities
Groups = {
    core = "Framework",
    component = "Components",
    engine1 = "Equilibrium 1 port",
    engine2 = "Equilibrium 2",
    tools = "Tools"
}

-- folders for framework, libraries and tools
Folders = {
    public =  "$(projectdir)/public/",
    matsystem1 = "$(projectdir)/materialsystem1/",
    shared_engine = "$(projectdir)/shared_engine/",
    shared_game = "$(projectdir)/shared_game/",
    dependency = "$(projectdir)/src_dependency/",
}

local function add_eq_deps() -- for internal use only
    add_deps("coreLib", "frameworkLib")
end

function add_eqcore_deps()
    add_eq_deps()
    add_deps("e2Core")
end

-----------------------------------------------------

if is_plat("windows") or is_plat("linux") or is_plat("android") then
    add_defines("PLAT_SDL=1")
end

function add_SDL2()
    if is_plat("windows") or is_plat("android") then
        add_includedirs(Folders.dependency.."SDL2/include")
        add_linkdirs(Folders.dependency.."SDL2/lib/$(arch)")
        add_links("SDL2")
    end
end

if is_plat("windows") then

    add_defines(
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_SECURE_NO_DEPRECATE")

elseif is_plat("linux") then 

    add_syslinks("pthread")
    add_cxflags(
        "-fexceptions", 
        "-fpic")
end

-- default runtime configuration
if is_mode("debug") or is_mode("releasedbg") then

    add_defines("EQ_DEBUG")
    set_symbols("debug")

elseif is_mode("release") then

    add_ldflags("/LTCG", "/OPT:REF")
    add_cxflags("/Ot", "/GL", "/Ob2", "/Oi", "/GS-")
    add_defines("NDEBUG")

    set_optimize("fastest")
end

-- extra runtime configuration (if needed)
function setup_runtime_config(crtDebug)
    if is_mode("debug") then 
        set_runtimes("MDd")

        -- allow CRT debugging features on windows
        if is_plat("windows") and crtDebug then
            add_defines("CRT_DEBUG_ENABLED")
        end
    end
end

----------------------------------------------
-- Core library

target("coreLib")
    set_group(Groups.core)
    set_kind("static")
    add_files(Folders.public.. "/core/*.cpp")
    add_headerfiles(Folders.public.. "/core/**.h")
    add_includedirs(Folders.public, { public = true })

----------------------------------------------
-- Framework library

target("frameworkLib")
    set_group(Groups.core)
    set_kind("static")
    add_files(
        Folders.public.. "/utils/*.cpp",
        Folders.public.. "/math/*.cpp",
        Folders.public.. "/imaging/*.cpp")
    add_headerfiles(
        Folders.public.. "/utils/*.h",
        Folders.public.. "/math/*.h",
        Folders.public.. "/imaging/*.h")
    add_packages("zlib", "libjpeg")
    add_includedirs(Folders.public, { public = true })

----------------------------------------------
-- e2Core

target("e2Core")
    set_group(Groups.core)
    set_kind("shared")
    --setup_runtime_config(true)
    add_files(
        "core/**.cpp",
        "core/minizip/*.c")
    add_headerfiles(
        "core/*.h",
        "core/platform/*.h")
    add_defines(
        "CORE_INTERFACE_EXPORT",
        "COREDLL_EXPORT")

    if is_plat("android") then 
        add_files("core/android_libc/*.c")
        add_headerfiles("core/android_libc/*.h")
    end

    add_packages("zlib")
    add_eq_deps()

    if is_os("windows") then
        add_syslinks("User32", "DbgHelp", "Advapi32")
    end
    
----------------------------------------------
-- Various components

-- fonts
target("fontLib")
    set_group(Groups.component)
    set_kind("static")
    add_files(Folders.shared_engine.. "font/**.cpp")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()

-- render utility
target("renderUtilLib")
    set_group(Groups.component)
    set_kind("static")
    add_files(Folders.shared_engine.. "render/**.cpp")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()

-- EGF
target("egfLib")
    set_group(Groups.component)
    set_kind("static")
    add_files(Folders.shared_engine.. "egf/**.cpp", Folders.shared_engine.. "egf/**.c")
    add_headerfiles(Folders.shared_engine.. "egf/**.h")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()
    add_deps("renderUtilLib")
    add_packages("zlib", "bullet3")

-- Animating game library
target("animatingLib")
    set_group(Groups.component)
    set_kind("static")
    add_files(Folders.shared_game.. "animating/**.cpp")
    add_includedirs(Folders.shared_game, { public = true })
    add_deps("egfLib")
    add_eq_deps()

----------------------------------------------
-- Equilirium Material System 1

target("eqMatSystem")
    set_group(Groups.engine1)
    set_kind("shared")
    add_files(
        Folders.matsystem1.. "*.cpp",
        Folders.public.."materialsystem1/*.cpp")
    add_headerfiles(Folders.matsystem1.."*.h")
    add_eqcore_deps()

----------------------------------------------
-- Render hardware interface libraries of Eq1

-- base library
target("eqRHIBaseLib")
    set_group(Groups.engine1)
    set_kind("static")
    add_files(Folders.matsystem1.. "renderers/Shared/*.cpp")
    add_headerfiles(Folders.matsystem1.."renderers/Shared/*.h")
    add_includedirs(Folders.public.."materialsystem1/", { public = true })
    add_includedirs(Folders.matsystem1.."renderers/Shared", { public = true })
    add_eqcore_deps()

-- empty renderer
target("eqNullRHI")
    set_group(Groups.engine1)
    set_kind("shared")
    add_files(Folders.matsystem1.. "renderers/Empty/*.cpp")
    add_headerfiles(Folders.matsystem1.."renderers/Empty/*.h")
    add_deps("eqRHIBaseLib")

-- OpenGL renderer
target("eqGLRHI")
    set_group(Groups.engine1)
    set_kind("shared")
    add_files(Folders.matsystem1.. "renderers/GL/*.cpp")
    add_files(Folders.matsystem1.. "renderers/GL/loaders/gl_loader.cpp")
    add_includedirs(Folders.matsystem1.. "renderers/GL/loaders")

    if is_plat("windows") then
        add_syslinks("OpenGL32", "Gdi32")
        add_files(
                Folders.matsystem1.. "renderers/GL/loaders/wgl_caps.cpp",
                Folders.matsystem1.. "renderers/GL/loaders/glad.c")
    elseif is_plat("linux") then
    
        add_files(
            Folders.matsystem1.. "renderers/GL/loaders/glx_caps.cpp",
                Folders.matsystem1.. "renderers/GL/loaders/glad.c")
    elseif is_plat("android") then
        add_files(Folders.matsystem1.. "renderers/GL/loaders/glad_es3.c")
        add_defines("USE_GLES2")
        add_syslinks("GLESv2", "EGL")
    end

    add_headerfiles(Folders.matsystem1.."renderers/GL/**.h")
    add_deps("eqRHIBaseLib")

if is_plat("windows") then 
    -- Direct3D9 renderer
    target("eqD3D9RHI")
        set_group(Groups.engine1)
        set_kind("shared")
        add_files(Folders.matsystem1.. "renderers/D3D9/*.cpp")
        add_includedirs(Folders.dependency.."minidx9/include")
        add_linkdirs(Folders.dependency.."minidx9/lib/$(arch)")
        add_syslinks("d3d9", "d3dx9")
        add_headerfiles(Folders.matsystem1.."renderers/D3D9/**.h")
        add_deps("eqRHIBaseLib")
end

-- only build tools for big machines
if is_plat("windows", "linux") then
    includes("utils/xmake-utils.lua")
end

