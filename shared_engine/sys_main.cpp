//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entry points for various platforms
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#include <io.h>
#include <fcntl.h>

#include <crtdbg.h>

#endif // _WIN32

#include "platform/MessageBox.h"

#include "IDkCore.h"
#include "sys_window.h"

#include "ConVar.h"
#include "ILocalize.h"
#include "IFileSystem.h"

#include "sys_in_console.h"

#include "utils/DkLinkedList.h"

#ifdef _WIN32
//#include "Resources/resource.h"
#endif // _WIN32

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",CV_INVISIBLE);

#if defined(ANDROID) && defined(EQ_USE_SDL)

void EQSDLMessageBoxCallback(const char* messageStr, EMessageBoxType type )
{
	switch(type)
	{
		case MSGBOX_INFO:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "INFO", messageStr, NULL);
			break;
		case MSGBOX_WARNING:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "WARNING", messageStr, NULL);
			break;
		case MSGBOX_ERROR:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ERROR", messageStr, NULL);
			break;
		case MSGBOX_CRASH:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "FATAL ERROR", messageStr, NULL);
			break;
	}
}

#endif // ANDROID && EQ_USE_SDL

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	//flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	flag |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
#endif

	CEqConsoleInput::SpewInit();

	// init core
	if(!GetCore()->Init("Game", lpszCmdLine))
		return -1;

#else

#include <vector>

#ifdef ANDROID

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

struct JNI_t
{
	JNIEnv* env;
	AAssetManager* assetManager;
} g_jni;

// init Java Native Interface glue parts required to do some operations
void Android_InitJNI()
{
	SetMessageBoxCallback(EQSDLMessageBoxCallback);

    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	g_jni.env = env;
    jclass class_activity       = env->FindClass("android/app/Activity");
    jclass class_resources      = env->FindClass("android/content/res/Resources");
	jmethodID method_get_resources = env->GetMethodID(class_activity, "getResources", "()Landroid/content/res/Resources;");
	jmethodID method_get_assets = env->GetMethodID(class_resources, "getAssets", "()Landroid/content/res/AssetManager;");

	jobject raw_activity = (jobject)SDL_AndroidGetActivity();
	jobject raw_resources = env->CallObjectMethod(raw_activity, method_get_resources);
	jobject raw_asset_manager = env->CallObjectMethod(raw_resources, method_get_assets);

	g_jni.assetManager = AAssetManager_fromJava(env, raw_asset_manager);
}

// init base path to extract
bool Android_InitCore(int argc, char** argv)
{
	// 1. Extract EQ.CONFIG file
	// preconfigure game base path
	const char* bestStoragePath = nullptr;// "/storage/6135-3937/eqengine"

	int storageState = SDL_AndroidGetExternalStorageState();

	// if both read and write supported, use it
	if ((storageState & SDL_ANDROID_EXTERNAL_STORAGE_READ) &&
		(storageState & SDL_ANDROID_EXTERNAL_STORAGE_WRITE))
	{
		bestStoragePath = SDL_AndroidGetExternalStoragePath();
		MsgInfo("[FS] Using external storage: %s\n", bestStoragePath);
	}
	else
	{
		bestStoragePath = SDL_AndroidGetInternalStoragePath();
		MsgInfo("[FS] Using internal storage: %s\n", bestStoragePath);
	}

	AAssetDir* assetDir = AAssetManager_openDir(g_jni.assetManager, "");

	const char* filename;
	std::vector<char> buffer;
	char filenameBuffer[2048] = {0};

	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL)
	{
		//search for desired file
		if (!stricmp(filename, "EQ.CONFIG"))
		{
			AAsset *asset = AAssetManager_open(g_jni.assetManager, filename, AASSET_MODE_STREAMING);

			//holds size of searched file
			off64_t length = AAsset_getLength64(asset);
			//keeps track of remaining bytes to read
			off64_t remaining = AAsset_getRemainingLength64(asset);
			size_t Mb = 1000 * 1024; // 1Mb is maximum chunk size for compressed assets
			size_t currChunk;
			buffer.reserve(length);

			//while we have still some data to read
			while (remaining != 0)
			{
				//set proper size for our next chunk
				if (remaining >= Mb)
					currChunk = Mb;
				else
					currChunk = remaining;

				char chunk[currChunk];

				//read data chunk
				if (AAsset_read(asset, chunk, currChunk) > 0) // returns less than 0 on error
				{
					//and append it to our vector
					buffer.insert(buffer.end(), chunk, chunk + currChunk);
					remaining = AAsset_getRemainingLength64(asset);
				}
			}
			AAsset_close(asset);
		}

		// if we have file to extraxt in buffer
		if (buffer.size() > 0)
		{
            strcpy(filenameBuffer, bestStoragePath);
            strcat(filenameBuffer, "/");
            strcat(filenameBuffer, filename);

		    FILE* fp = fopen(filenameBuffer, "wb");
            if(fp)
            {
                fwrite(buffer.data(), 1, buffer.size(), fp);
                fclose(fp);

                MsgInfo("Extracted: %s\n", filenameBuffer);
            }

            filenameBuffer[0] = '\0';
			buffer.clear();
		}
	}

	// first we let eqconfig
	g_fileSystem->SetBasePath(bestStoragePath);

	// init core
	return GetCore()->Init("Game", argc, argv);
}

void Android_MountFileSystem()
{
	// current game directory needs to be created
	g_fileSystem->MakeDir(g_fileSystem->GetCurrentGameDirectory(), SP_ROOT);

	// as well as configuration file dir
	g_fileSystem->MakeDir("cfg", SP_MOD);

	// mount OBB file if available

}

#endif

// posix apps
int main(int argc, char** argv)
{
#ifdef ANDROID
    Android_InitJNI(); // initialize JNI

	if (!Android_InitCore(argc, argv))
		return -1;

	// mount OBB filesystem
	Android_MountFileSystem();
#else
	// init core
	if (!GetCore()->Init("Game", argc, argv))
		return -1;
#endif // ANDROID

#endif

	// init file system
	if (!g_fileSystem->Init(false))
	{
		GetCore()->Shutdown();
		return -2;
	}

	g_localizer->AddTokensFile("game");

	// initialize timers
	Platform_InitTime();

	if (!Host_Init())
	{
		// shutdown
		g_fileSystem->Shutdown();
		GetCore()->Shutdown();

		return -3;
	}


	Host_GameLoop();

	CEqConsoleInput::SpewClear();

	Host_Terminate();

	// shutdown
	g_fileSystem->Shutdown();
	GetCore()->Shutdown();

	return 0;
}
