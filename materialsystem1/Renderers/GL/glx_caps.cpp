#include <algorithm>
#include <vector>
#include <string.h>
#include <stddef.h>
#include "glx_caps.hpp"

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

#if defined(_WIN32)

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
	
#define IntGetProcAddress(name) WinGetProcAddress(name)
#else
	#if defined(__APPLE__)
		#define IntGetProcAddress(name) AppleGLGetProcAddress(name)
	#else
		#if defined(__sgi) || defined(__sun)
			#define IntGetProcAddress(name) SunGetProcAddress(name)
		#else /* GLX */
		    #include <GL/glx.h>

			#define IntGetProcAddress(name) (*glXGetProcAddressARB)((const GLubyte*)name)
		#endif
	#endif
#endif

namespace glX
{
	namespace exts
	{
		LoadTest var_ARB_create_context;
		LoadTest var_ARB_create_context_profile;
		LoadTest var_ARB_create_context_robustness;
		LoadTest var_ARB_fbconfig_float;
		LoadTest var_ARB_framebuffer_sRGB;
		LoadTest var_ARB_multisample;
		LoadTest var_EXT_create_context_es2_profile;
		LoadTest var_EXT_fbconfig_packed_float;
		LoadTest var_EXT_framebuffer_sRGB;
		LoadTest var_EXT_import_context;
		LoadTest var_EXT_swap_control;
		LoadTest var_EXT_swap_control_tear;
		
	} //namespace exts
	
	namespace _detail
	{
		typedef GLXContext (CODEGEN_FUNCPTR *PFNCREATECONTEXTATTRIBSARB)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
		PFNCREATECONTEXTATTRIBSARB CreateContextAttribsARB = 0;
		
		static int Load_ARB_create_context()
		{
			int numFailed = 0;
			CreateContextAttribsARB = reinterpret_cast<PFNCREATECONTEXTATTRIBSARB>(IntGetProcAddress("glXCreateContextAttribsARB"));
			if(!CreateContextAttribsARB) ++numFailed;
			return numFailed;
		}
		
		typedef void (CODEGEN_FUNCPTR *PFNFREECONTEXTEXT)(Display *, GLXContext);
		PFNFREECONTEXTEXT FreeContextEXT = 0;
		typedef GLXContextID (CODEGEN_FUNCPTR *PFNGETCONTEXTIDEXT)(const GLXContext);
		PFNGETCONTEXTIDEXT GetContextIDEXT = 0;
		typedef Display * (CODEGEN_FUNCPTR *PFNGETCURRENTDISPLAYEXT)(void);
		PFNGETCURRENTDISPLAYEXT GetCurrentDisplayEXT = 0;
		typedef GLXContext (CODEGEN_FUNCPTR *PFNIMPORTCONTEXTEXT)(Display *, GLXContextID);
		PFNIMPORTCONTEXTEXT ImportContextEXT = 0;
		typedef int (CODEGEN_FUNCPTR *PFNQUERYCONTEXTINFOEXT)(Display *, GLXContext, int, int *);
		PFNQUERYCONTEXTINFOEXT QueryContextInfoEXT = 0;
		
		static int Load_EXT_import_context()
		{
			int numFailed = 0;
			FreeContextEXT = reinterpret_cast<PFNFREECONTEXTEXT>(IntGetProcAddress("glXFreeContextEXT"));
			if(!FreeContextEXT) ++numFailed;
			GetContextIDEXT = reinterpret_cast<PFNGETCONTEXTIDEXT>(IntGetProcAddress("glXGetContextIDEXT"));
			if(!GetContextIDEXT) ++numFailed;
			GetCurrentDisplayEXT = reinterpret_cast<PFNGETCURRENTDISPLAYEXT>(IntGetProcAddress("glXGetCurrentDisplayEXT"));
			if(!GetCurrentDisplayEXT) ++numFailed;
			ImportContextEXT = reinterpret_cast<PFNIMPORTCONTEXTEXT>(IntGetProcAddress("glXImportContextEXT"));
			if(!ImportContextEXT) ++numFailed;
			QueryContextInfoEXT = reinterpret_cast<PFNQUERYCONTEXTINFOEXT>(IntGetProcAddress("glXQueryContextInfoEXT"));
			if(!QueryContextInfoEXT) ++numFailed;
			return numFailed;
		}
		
		typedef void (CODEGEN_FUNCPTR *PFNSWAPINTERVALEXT)(Display *, GLXDrawable, int);
		PFNSWAPINTERVALEXT SwapIntervalEXT = 0;
		
		static int Load_EXT_swap_control()
		{
			int numFailed = 0;
			SwapIntervalEXT = reinterpret_cast<PFNSWAPINTERVALEXT>(IntGetProcAddress("glXSwapIntervalEXT"));
			if(!SwapIntervalEXT) ++numFailed;
			return numFailed;
		}
		
	} //namespace _detail
	
	namespace sys
	{
		namespace 
		{
			typedef int (*PFN_LOADEXTENSION)();
			struct MapEntry
			{
				MapEntry(const char *_extName, exts::LoadTest *_extVariable)
					: extName(_extName)
					, extVariable(_extVariable)
					, loaderFunc(0)
					{}
					
				MapEntry(const char *_extName, exts::LoadTest *_extVariable, PFN_LOADEXTENSION _loaderFunc)
					: extName(_extName)
					, extVariable(_extVariable)
					, loaderFunc(_loaderFunc)
					{}
				
				const char *extName;
				exts::LoadTest *extVariable;
				PFN_LOADEXTENSION loaderFunc;
			};
			
			struct MapCompare
			{
				MapCompare(const char *test_) : test(test_) {}
				bool operator()(const MapEntry &other) { return strcmp(test, other.extName) == 0; }
				const char *test;
			};
			
			void InitializeMappingTable(std::vector<MapEntry> &table)
			{
				table.reserve(12);
				table.push_back(MapEntry("GLX_ARB_create_context", &exts::var_ARB_create_context, _detail::Load_ARB_create_context));
				table.push_back(MapEntry("GLX_ARB_create_context_profile", &exts::var_ARB_create_context_profile));
				table.push_back(MapEntry("GLX_ARB_create_context_robustness", &exts::var_ARB_create_context_robustness));
				table.push_back(MapEntry("GLX_ARB_fbconfig_float", &exts::var_ARB_fbconfig_float));
				table.push_back(MapEntry("GLX_ARB_framebuffer_sRGB", &exts::var_ARB_framebuffer_sRGB));
				table.push_back(MapEntry("GLX_ARB_multisample", &exts::var_ARB_multisample));
				table.push_back(MapEntry("GLX_EXT_create_context_es2_profile", &exts::var_EXT_create_context_es2_profile));
				table.push_back(MapEntry("GLX_EXT_fbconfig_packed_float", &exts::var_EXT_fbconfig_packed_float));
				table.push_back(MapEntry("GLX_EXT_framebuffer_sRGB", &exts::var_EXT_framebuffer_sRGB));
				table.push_back(MapEntry("GLX_EXT_import_context", &exts::var_EXT_import_context, _detail::Load_EXT_import_context));
				table.push_back(MapEntry("GLX_EXT_swap_control", &exts::var_EXT_swap_control, _detail::Load_EXT_swap_control));
				table.push_back(MapEntry("GLX_EXT_swap_control_tear", &exts::var_EXT_swap_control_tear));
			}
			
			void ClearExtensionVars()
			{
				exts::var_ARB_create_context = exts::LoadTest();
				exts::var_ARB_create_context_profile = exts::LoadTest();
				exts::var_ARB_create_context_robustness = exts::LoadTest();
				exts::var_ARB_fbconfig_float = exts::LoadTest();
				exts::var_ARB_framebuffer_sRGB = exts::LoadTest();
				exts::var_ARB_multisample = exts::LoadTest();
				exts::var_EXT_create_context_es2_profile = exts::LoadTest();
				exts::var_EXT_fbconfig_packed_float = exts::LoadTest();
				exts::var_EXT_framebuffer_sRGB = exts::LoadTest();
				exts::var_EXT_import_context = exts::LoadTest();
				exts::var_EXT_swap_control = exts::LoadTest();
				exts::var_EXT_swap_control_tear = exts::LoadTest();
			}
			
			void LoadExtByName(std::vector<MapEntry> &table, const char *extensionName)
			{
				std::vector<MapEntry>::iterator entry = std::find_if(table.begin(), table.end(), MapCompare(extensionName));
				
				if(entry != table.end())
				{
					if(entry->loaderFunc)
						(*entry->extVariable) = exts::LoadTest(true, entry->loaderFunc());
					else
						(*entry->extVariable) = exts::LoadTest(true, 0);
				}
			}
		} //namespace 
		
		
		namespace 
		{
			static void ProcExtsFromExtString(const char *strExtList, std::vector<MapEntry> &table)
			{
				size_t iExtListLen = strlen(strExtList);
				const char *strExtListEnd = strExtList + iExtListLen;
				const char *strCurrPos = strExtList;
				char strWorkBuff[256];
			
				while(*strCurrPos)
				{
					/*Get the extension at our position.*/
					int iStrLen = 0;
					const char *strEndStr = strchr(strCurrPos, ' ');
					int iStop = 0;
					if(strEndStr == NULL)
					{
						strEndStr = strExtListEnd;
						iStop = 1;
					}
			
					iStrLen = (int)((ptrdiff_t)strEndStr - (ptrdiff_t)strCurrPos);
			
					if(iStrLen > 255)
						return;
			
					strncpy(strWorkBuff, strCurrPos, iStrLen);
					strWorkBuff[iStrLen] = '\0';
			
					LoadExtByName(table, strWorkBuff);
			
					strCurrPos = strEndStr + 1;
					if(iStop) break;
				}
			}
			
		} //namespace 
		
		exts::LoadTest LoadFunctions(Display *display, int screen)
		{
			ClearExtensionVars();
			std::vector<MapEntry> table;
			InitializeMappingTable(table);
			
			
			ProcExtsFromExtString((const char *)glXQueryExtensionsString(display, screen), table);
			return exts::LoadTest(true, 0);
		}
		
	} //namespace sys
} //namespace glX
