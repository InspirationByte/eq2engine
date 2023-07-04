
#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ICommandLine.h"

#include "RenderManager.h"
#include "renderers/IShaderAPI.h"

#define RHI_NULL      0
#define RHI_OPENGL    1
#define RHI_D3D9      2
#define RHI_D3D11     3

#if RENDERER_TYPE == RHI_NULL
#include "Empty/emptyLibrary.h"
static CEmptyRenderLib  s_EmptyRenderLib;

#elif RENDERER_TYPE == RHI_OPENGL
#if defined(PLAT_LINUX) && !defined(USE_GLES2)
#include "GL/GLRenderLibrary_GLX.h"
static CGLRenderLib_GLX s_GLXRenderLib;
#endif // PLAT_LINUX

#if defined(PLAT_WIN) && !defined(USE_GLES2)
#include "GL/GLRenderLibrary_WGL.h"
static CGLRenderLib_WGL s_WGLRenderLib;
#endif // PLAT_WIN

#include "GL/GLRenderLibrary_EGL.h"
static CGLRenderLib_EGL s_EGLRenderLib;

#elif RENDERER_TYPE == RHI_D3D9
#include "D3D9/D3D9RenderLibrary.h"
static CD3D9RenderLib   s_D3D9RenderLib;
#elif RENDERER_TYPE == RHI_D3D11
#include "D3D11/D3D11Library.h"
static CD3D11RenderLib  s_D3D11RenderLib;
#endif

static CEqRenderManager g_renderManager;
static IRenderLibrary* s_currentRenderLib = nullptr;

CEqRenderManager::CEqRenderManager()
{
    g_eqCore->RegisterInterface(this);
}

CEqRenderManager::~CEqRenderManager()
{
	g_eqCore->UnregisterInterface<CEqRenderManager>();
}

IRenderLibrary* CEqRenderManager::CreateRenderer(const shaderAPIParams_t &params) const
{
#if RENDERER_TYPE == RHI_NULL
    s_currentRenderLib = &s_EmptyRenderLib;
    return s_currentRenderLib;
#endif

    const bool forceEGL = g_cmdLine->FindArgument("-egl") != -1;

    switch(params.windowInfo.windowType)
    {
#if defined(PLAT_WIN)
#   if defined(USE_GLES2)
        case RHI_WINDOW_HANDLE_NATIVE_WINDOWS:
            s_currentRenderLib = &s_EGLRenderLib;
            break;
#   else
        case RHI_WINDOW_HANDLE_NATIVE_WINDOWS:
            // Depends on compiled renderer
#       if RENDERER_TYPE == RHI_OPENGL
            s_currentRenderLib = &s_WGLRenderLib;
#       elif RENDERER_TYPE == RHI_D3D9
            s_currentRenderLib = &s_D3D9RenderLib;
#       elif RENDERER_TYPE == RHI_D3D11
            s_currentRenderLib = &s_D3D11RenderLib;
#       endif // RENDERER_TYPE
            break;
#   endif // USE_GLES2
#elif RENDERER_TYPE == RHI_OPENGL
        case RHI_WINDOW_HANDLE_NATIVE_X11:
#   ifndef USE_GLES2
            if(!forceEGL)
            {
                s_currentRenderLib = &s_GLXRenderLib;
                break;
            }
#   endif // USE_GLES2
        case RHI_WINDOW_HANDLE_NATIVE_WAYLAND:
        case RHI_WINDOW_HANDLE_NATIVE_ANDROID:
            s_currentRenderLib = &s_EGLRenderLib;
            break;
#endif // !PLAT_WIN
        default:
            ASSERT_FAIL("Cannot create renderer library interface");
    }

    return s_currentRenderLib;
}

IRenderLibrary* CEqRenderManager::GetRenderer() const
{
    return s_currentRenderLib;
}