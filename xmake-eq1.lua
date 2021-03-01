set_project("Equilibrium 1 components")

-- fonts
target("fontLib")
    set_group(Groups.component1)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "font/**.cpp")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()

-- render utility
target("renderUtilLib")
    set_group(Groups.component1)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "render/**.cpp")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()

-- EGF
target("egfLib")
    set_group(Groups.component1)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_engine.. "egf/**.cpp", Folders.shared_engine.. "egf/**.c")
    add_headerfiles(Folders.shared_engine.. "egf/**.h")
    add_includedirs(Folders.shared_engine, { public = true })
    add_eq_deps()
    add_deps("renderUtilLib")
    add_packages("zlib", "bullet3")

-- Animating game library
target("animatingLib")
    set_group(Groups.component1)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.shared_game.. "animating/**.cpp")
    add_includedirs(Folders.shared_game, { public = true })
    add_deps("egfLib")
    add_eq_deps()

-- Equilibrium 1 Darktech Physics
target("dkPhysicsLib")
    set_group(Groups.component1)
    set_kind("static")
    setup_runtime_config()
    add_files(  Folders.shared_engine.. "dkphysics/**.cpp",
                Folders.shared_engine.. "physics/**.cpp")
    add_includedirs(Folders.shared_engine, { public = true })
    add_deps("egfLib", "animatingLib")
    add_packages("bullet3")
    add_eq_deps()

----------------------------------------------
-- Equilirium Material System 1

target("eqMatSystem")
    set_group(Groups.engine1)
    set_kind("shared")
    setup_runtime_config()
    add_files(
        Folders.matsystem1.. "*.cpp",
        Folders.public.."materialsystem1/*.cpp")
    add_headerfiles(Folders.matsystem1.."*.h")
    add_headerfiles(Folders.public.."materialsystem1/*.h")
    add_eqcore_deps()

-- base shader library
target("eqBaseShaders")
    set_group(Groups.engine1)
    set_kind("shared")
    setup_runtime_config()
    add_files(
        Folders.matsystem1.."Shaders/*.cpp",
        Folders.matsystem1.."Shaders/Base/*.cpp",
        Folders.public.."materialsystem1/*.cpp")
    add_headerfiles(Folders.matsystem1.."/Shaders/*.h")
    add_includedirs(Folders.public.."materialsystem1")
    add_eqcore_deps()

----------------------------------------------
-- Render hardware interface libraries of Eq1

-- base library
target("eqRHIBaseLib")
    set_group(Groups.engine1)
    set_kind("static")
    setup_runtime_config()
    add_files(Folders.matsystem1.. "renderers/Shared/*.cpp")
    add_headerfiles(Folders.matsystem1.."renderers/Shared/*.h")
    add_includedirs(Folders.public.."materialsystem1/", { public = true })
    add_includedirs(Folders.matsystem1.."renderers/Shared", { public = true })
    add_eqcore_deps()

-- empty renderer
target("eqNullRHI")
    set_group(Groups.engine1)
    set_kind("shared")
    setup_runtime_config()
    add_files(Folders.matsystem1.. "renderers/Empty/*.cpp")
    add_headerfiles(Folders.matsystem1.."renderers/Empty/*.h")
    add_deps("eqRHIBaseLib")

-- OpenGL renderer
target("eqGLRHI")
    set_group(Groups.engine1)
    set_kind("shared")
    setup_runtime_config()
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
        setup_runtime_config()
        add_files(Folders.matsystem1.. "renderers/D3D9/*.cpp")
        add_includedirs(Folders.dependency.."minidx9/include")
        add_linkdirs(Folders.dependency.."minidx9/lib/$(arch)")
        add_syslinks("d3d9", "d3dx9")
        add_headerfiles(Folders.matsystem1.."renderers/D3D9/**.h")
        add_deps("eqRHIBaseLib")
end

