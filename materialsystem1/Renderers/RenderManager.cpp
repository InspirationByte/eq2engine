
#include "core/core_common.h"
#include "core/IDkCore.h"

#include "RenderManager.h"
#include "renderers/IShaderAPI.h"

#if RENDERER_TYPE == 0
#include "Empty/emptyLibrary.h"
static CEmptyRenderLib  s_EmptyRenderLib;

#elif RENDERER_TYPE == 1
#ifdef USE_SDL2
#include "GL/GLRenderLibrary_SDL.h"
static CGLRenderLib_SDL s_SDLRenderLib;
#endif // USE_SDL2

#include "GL/GLRenderLibrary_GLX.h"
static CGLRenderLib_GLX s_GLXRenderLib;

#ifdef PLAT_WIN
#include "GL/GLRenderLibrary_WGL.h"
static CGLRenderLib_WGL s_WGLRenderLib;
#endif // PLAT_WIN

#include "GL/GLRenderLibrary_EGL.h"
static CGLRenderLib_EGL s_EGLRenderLib;

#elif RENDERER_TYPE == 2
#include "D3D9/D3D9RenderLibrary.h"
static CD3D9RenderLib   s_D3D9RenderLib;
#elif RENDERER_TYPE == 3
#include "D3D11/D3D11Library.h"
static CD3D11RenderLib  s_D3D11RenderLib;
#endif

CEqRenderManager g_renderManager;
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
#if RENDERER_TYPE == 0
    s_currentRenderLib = &s_EmptyRenderLib;
    return s_currentRenderLib;
#endif

    switch(params.windowHandleType)
    {
#ifdef PLAT_WIN
        case RHI_WINDOW_HANDLE_NATIVE_WINDOWS:
            // Depends on compiled renderer
#if RENDERER_TYPE == 1
            s_currentRenderLib = &s_WGLRenderLib;
#elif RENDERER_TYPE == 2
            s_currentRenderLib = &s_D3D9RenderLib;
#elif RENDERER_TYPE == 3
            s_currentRenderLib = &s_D3D11RenderLib;
#endif // RENDERER_TYPE
            break;
#endif // #ifdef PLAT_WIN
#if RENDERER_TYPE == 1
        case RHI_WINDOW_HANDLE_NATIVE_X11:
            s_currentRenderLib = &s_GLXRenderLib;
            break;
        case RHI_WINDOW_HANDLE_NATIVE_WAYLAND:
        case RHI_WINDOW_HANDLE_VTABLE: // Android case
            s_currentRenderLib = &s_EGLRenderLib;
            break;
#ifdef USE_SDL2
        case RHI_WINDOW_HANDLE_SDL:
            s_currentRenderLib = &s_SDLRenderLib;
            break;
#endif // USE_SDL2
#endif // RENDERER_TYPE
        default:
            ASSERT_FAIL("Cannot create renderer library interface");
    }

    return s_currentRenderLib;
}

IRenderLibrary* CEqRenderManager::GetRenderer() const
{
    return s_currentRenderLib;
}