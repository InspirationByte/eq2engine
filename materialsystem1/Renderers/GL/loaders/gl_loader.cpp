//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL loader function
//////////////////////////////////////////////////////////////////////////////////

#include "gl_loader.h"

#if defined(__APPLE__)
#include <dlfcn.h>

static void* AppleGLGetProcAddress (const char *name)
{
	static void* image = NULL;

	if (NULL == image)
		image = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);

	return (image ? dlsym(image, name) : NULL);
}
#endif /* __APPLE__ */

#if defined(__sgi) || defined (__sun)
#include <dlfcn.h>
#include <stdio.h>

static void* SunGetProcAddress (const GLubyte* name)
{
  static void* h = NULL;
  static void* gpa;

  if (h == NULL)
  {
    if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL) return NULL;
    gpa = dlsym(h, "glXGetProcAddress");
  }

  if (gpa != NULL)
    return ((void*(*)(const GLubyte*))gpa)(name);
  else
    return dlsym(h, (const char*)name);
}
#endif /* __sgi || __sun */

#if defined(_WIN32) && !defined(USE_GLES2)

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(disable: 4055)
#pragma warning(disable: 4054)
#pragma warning(disable: 4996)
#endif

static int TestPointer(const PROC pTest)
{
	ptrdiff_t iTest;
	if(!pTest) return 0;
	iTest = (ptrdiff_t)pTest;

	if(iTest == 1 || iTest == 2 || iTest == 3 || iTest == -1) return 0;

	return 1;
}

static PROC WinGetProcAddress(const char *name)
{
	HMODULE glMod = NULL;
	PROC pFunc = wglGetProcAddress((LPCSTR)name);
	if(TestPointer(pFunc))
	{
		return pFunc;
	}
	glMod = GetModuleHandleA("OpenGL32.dll");
	return (PROC)GetProcAddress(glMod, (LPCSTR)name);
}

#define glProcAddressFunc(name) WinGetProcAddress(name)
#else
	#if defined(__APPLE__)
		#define glProcAddressFunc(name) AppleGLGetProcAddress(name)
	#else
		#if defined(__sgi) || defined(__sun)
			#define glProcAddressFunc(name) SunGetProcAddress(name)
        #elif defined(__ANDROID__) || defined(USE_GLES2)

		#ifdef _WIN32
			#include "glad_egl.h"
		#else
			#include <EGL/egl.h>
		#endif
			#define glProcAddressFunc(name) (void*)eglGetProcAddress(name)
		#else /* GLX */
		    #include <GL/glx.h>
			#define glProcAddressFunc(name) (void*)(*glXGetProcAddressARB)((const GLubyte*)name)
		#endif
	#endif
#endif


void* gladHelperLoaderFunction(const char* name)
{
	return glProcAddressFunc(name);
}
