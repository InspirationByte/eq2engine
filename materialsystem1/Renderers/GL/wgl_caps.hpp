#ifndef FUNCTION_CPP_GENERATED_HEADERWINDOWSGL_HPP
#define FUNCTION_CPP_GENERATED_HEADERWINDOWSGL_HPP

#ifdef __wglext_h_
#error Attempt to include auto-generated WGL header after wglext.h
#endif

#define __wglext_h_

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>

#ifdef CODEGEN_FUNCPTR
#undef CODEGEN_FUNCPTR
#endif /*CODEGEN_FUNCPTR*/
#define CODEGEN_FUNCPTR WINAPI

#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
#define GLvoid void

#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/


#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS


#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/

struct _GPU_DEVICE {
    DWORD  cb;
    CHAR   DeviceName[32];
    CHAR   DeviceString[128];
    DWORD  Flags;
    RECT   rcVirtualScreen;
};
DECLARE_HANDLE(HPBUFFERARB);
DECLARE_HANDLE(HPBUFFEREXT);
DECLARE_HANDLE(HVIDEOOUTPUTDEVICENV);
DECLARE_HANDLE(HPVIDEODEV);
DECLARE_HANDLE(HGPUNV);
DECLARE_HANDLE(HVIDEOINPUTDEVICENV);
typedef struct _GPU_DEVICE *PGPU_DEVICE;

namespace wgl
{
	namespace exts
	{
		class LoadTest
		{
		private:
			//Safe bool idiom. Joy!
			typedef void (LoadTest::*bool_type)() const;
			void big_long_name_that_really_doesnt_matter() const {}
			
		public:
			operator bool_type() const
			{
				return m_isLoaded ? &LoadTest::big_long_name_that_really_doesnt_matter : 0;
			}
			
			int GetNumMissing() const {return m_numMissing;}
			
			LoadTest() : m_isLoaded(false), m_numMissing(0) {}
			LoadTest(bool isLoaded, int numMissing) : m_isLoaded(isLoaded), m_numMissing(numMissing) {}
		private:
			bool m_isLoaded;
			int m_numMissing;
		};
		
		extern LoadTest var_ARB_multisample;
		extern LoadTest var_ARB_extensions_string;
		extern LoadTest var_ARB_pixel_format;
		extern LoadTest var_ARB_pixel_format_float;
		extern LoadTest var_ARB_framebuffer_sRGB;
		extern LoadTest var_ARB_create_context;
		extern LoadTest var_ARB_create_context_profile;
		extern LoadTest var_ARB_create_context_robustness;
		extern LoadTest var_EXT_swap_control;
		extern LoadTest var_EXT_pixel_format_packed_float;
		extern LoadTest var_EXT_create_context_es2_profile;
		extern LoadTest var_EXT_swap_control_tear;
		extern LoadTest var_NV_swap_group;
		
	} //namespace exts
	enum
	{
		SAMPLES_ARB                      = 0x2042,
		SAMPLE_BUFFERS_ARB               = 0x2041,
		
		ACCELERATION_ARB                 = 0x2003,
		ACCUM_ALPHA_BITS_ARB             = 0x2021,
		ACCUM_BITS_ARB                   = 0x201D,
		ACCUM_BLUE_BITS_ARB              = 0x2020,
		ACCUM_GREEN_BITS_ARB             = 0x201F,
		ACCUM_RED_BITS_ARB               = 0x201E,
		ALPHA_BITS_ARB                   = 0x201B,
		ALPHA_SHIFT_ARB                  = 0x201C,
		AUX_BUFFERS_ARB                  = 0x2024,
		BLUE_BITS_ARB                    = 0x2019,
		BLUE_SHIFT_ARB                   = 0x201A,
		COLOR_BITS_ARB                   = 0x2014,
		DEPTH_BITS_ARB                   = 0x2022,
		DOUBLE_BUFFER_ARB                = 0x2011,
		DRAW_TO_BITMAP_ARB               = 0x2002,
		DRAW_TO_WINDOW_ARB               = 0x2001,
		FULL_ACCELERATION_ARB            = 0x2027,
		GENERIC_ACCELERATION_ARB         = 0x2026,
		GREEN_BITS_ARB                   = 0x2017,
		GREEN_SHIFT_ARB                  = 0x2018,
		NEED_PALETTE_ARB                 = 0x2004,
		NEED_SYSTEM_PALETTE_ARB          = 0x2005,
		NO_ACCELERATION_ARB              = 0x2025,
		NUMBER_OVERLAYS_ARB              = 0x2008,
		NUMBER_PIXEL_FORMATS_ARB         = 0x2000,
		NUMBER_UNDERLAYS_ARB             = 0x2009,
		PIXEL_TYPE_ARB                   = 0x2013,
		RED_BITS_ARB                     = 0x2015,
		RED_SHIFT_ARB                    = 0x2016,
		SHARE_ACCUM_ARB                  = 0x200E,
		SHARE_DEPTH_ARB                  = 0x200C,
		SHARE_STENCIL_ARB                = 0x200D,
		STENCIL_BITS_ARB                 = 0x2023,
		STEREO_ARB                       = 0x2012,
		SUPPORT_GDI_ARB                  = 0x200F,
		SUPPORT_OPENGL_ARB               = 0x2010,
		SWAP_COPY_ARB                    = 0x2029,
		SWAP_EXCHANGE_ARB                = 0x2028,
		SWAP_LAYER_BUFFERS_ARB           = 0x2006,
		SWAP_METHOD_ARB                  = 0x2007,
		SWAP_UNDEFINED_ARB               = 0x202A,
		TRANSPARENT_ALPHA_VALUE_ARB      = 0x203A,
		TRANSPARENT_ARB                  = 0x200A,
		TRANSPARENT_BLUE_VALUE_ARB       = 0x2039,
		TRANSPARENT_GREEN_VALUE_ARB      = 0x2038,
		TRANSPARENT_INDEX_VALUE_ARB      = 0x203B,
		TRANSPARENT_RED_VALUE_ARB        = 0x2037,
		TYPE_COLORINDEX_ARB              = 0x202C,
		TYPE_RGBA_ARB                    = 0x202B,
		
		TYPE_RGBA_FLOAT_ARB              = 0x21A0,
		
		FRAMEBUFFER_SRGB_CAPABLE_ARB     = 0x20A9,
		
		CONTEXT_DEBUG_BIT_ARB            = 0x00000001,
		CONTEXT_FLAGS_ARB                = 0x2094,
		CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x00000002,
		CONTEXT_LAYER_PLANE_ARB          = 0x2093,
		CONTEXT_MAJOR_VERSION_ARB        = 0x2091,
		CONTEXT_MINOR_VERSION_ARB        = 0x2092,
		ERROR_INVALID_VERSION_ARB        = 0x2095,
		
		CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB = 0x00000002,
		CONTEXT_CORE_PROFILE_BIT_ARB     = 0x00000001,
		CONTEXT_PROFILE_MASK_ARB         = 0x9126,
		ERROR_INVALID_PROFILE_ARB        = 0x2096,
		
		CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB = 0x8256,
		CONTEXT_ROBUST_ACCESS_BIT_ARB    = 0x00000004,
		LOSE_CONTEXT_ON_RESET_ARB        = 0x8252,
		NO_RESET_NOTIFICATION_ARB        = 0x8261,
		
		TYPE_RGBA_UNSIGNED_FLOAT_EXT     = 0x20A8,
		
		CONTEXT_ES2_PROFILE_BIT_EXT      = 0x00000004,
		
	};
	
	namespace _detail
	{
		
		extern const char * (CODEGEN_FUNCPTR *GetExtensionsStringARB)(HDC hdc);
		
		extern BOOL (CODEGEN_FUNCPTR *ChoosePixelFormatARB)(HDC hdc, const int * piAttribIList, const FLOAT * pfAttribFList, UINT nMaxFormats, int * piFormats, UINT * nNumFormats);
		extern BOOL (CODEGEN_FUNCPTR *GetPixelFormatAttribfvARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int * piAttributes, FLOAT * pfValues);
		extern BOOL (CODEGEN_FUNCPTR *GetPixelFormatAttribivARB)(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int * piAttributes, int * piValues);
		
		
		
		extern HGLRC (CODEGEN_FUNCPTR *CreateContextAttribsARB)(HDC hDC, HGLRC hShareContext, const int * attribList);
		
		
		
		extern int (CODEGEN_FUNCPTR *GetSwapIntervalEXT)(void);
		extern BOOL (CODEGEN_FUNCPTR *SwapIntervalEXT)(int interval);
		
		
		
		
		extern BOOL (CODEGEN_FUNCPTR *BindSwapBarrierNV)(GLuint group, GLuint barrier);
		extern BOOL (CODEGEN_FUNCPTR *JoinSwapGroupNV)(HDC hDC, GLuint group);
		extern BOOL (CODEGEN_FUNCPTR *QueryFrameCountNV)(HDC hDC, GLuint * count);
		extern BOOL (CODEGEN_FUNCPTR *QueryMaxSwapGroupsNV)(HDC hDC, GLuint * maxGroups, GLuint * maxBarriers);
		extern BOOL (CODEGEN_FUNCPTR *QuerySwapGroupNV)(HDC hDC, GLuint * group, GLuint * barrier);
		extern BOOL (CODEGEN_FUNCPTR *ResetFrameCountNV)(HDC hDC);
		
	} //namespace _detail
	
	
	inline const char * GetExtensionsStringARB(HDC hdc){return _detail::GetExtensionsStringARB(hdc);}
	
	inline BOOL ChoosePixelFormatARB(HDC hdc, const int * piAttribIList, const FLOAT * pfAttribFList, UINT nMaxFormats, int * piFormats, UINT * nNumFormats){return _detail::ChoosePixelFormatARB(hdc, piAttribIList, pfAttribFList, nMaxFormats, piFormats, nNumFormats);}
	inline BOOL GetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int * piAttributes, FLOAT * pfValues){return _detail::GetPixelFormatAttribfvARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, pfValues);}
	inline BOOL GetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int * piAttributes, int * piValues){return _detail::GetPixelFormatAttribivARB(hdc, iPixelFormat, iLayerPlane, nAttributes, piAttributes, piValues);}
	
	
	
	inline HGLRC CreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int * attribList){return _detail::CreateContextAttribsARB(hDC, hShareContext, attribList);}
	
	
	
	inline int GetSwapIntervalEXT(void){return _detail::GetSwapIntervalEXT();}
	inline BOOL SwapIntervalEXT(int interval){return _detail::SwapIntervalEXT(interval);}
	
	
	
	
	inline BOOL BindSwapBarrierNV(GLuint group, GLuint barrier){return _detail::BindSwapBarrierNV(group, barrier);}
	inline BOOL JoinSwapGroupNV(HDC hDC, GLuint group){return _detail::JoinSwapGroupNV(hDC, group);}
	inline BOOL QueryFrameCountNV(HDC hDC, GLuint * count){return _detail::QueryFrameCountNV(hDC, count);}
	inline BOOL QueryMaxSwapGroupsNV(HDC hDC, GLuint * maxGroups, GLuint * maxBarriers){return _detail::QueryMaxSwapGroupsNV(hDC, maxGroups, maxBarriers);}
	inline BOOL QuerySwapGroupNV(HDC hDC, GLuint * group, GLuint * barrier){return _detail::QuerySwapGroupNV(hDC, group, barrier);}
	inline BOOL ResetFrameCountNV(HDC hDC){return _detail::ResetFrameCountNV(hDC);}
	
	
	namespace sys
	{
		
		exts::LoadTest LoadFunctions(HDC hdc);
		
		int GetMinorVersion();
		int GetMajorVersion();
		bool IsVersionGEQ(int majorVersion, int minorVersion);
		
	} //namespace sys
} //namespace wgl
#endif //FUNCTION_CPP_GENERATED_HEADERWINDOWSGL_HPP
