//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entry points for various platforms
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#include <io.h>
#include <fcntl.h>

#ifdef CRT_DEBUG_ENABLED
#include <crtdbg.h>
#endif

#endif // _WIN32

#include "core/platform/MessageBox.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ILocalize.h"
#include "core/IFileSystem.h"

#include "ds/DkLinkedList.h"
#include "utils/KeyValues.h"

#include "sys_in_console.h"
#include "sys_window.h"
#include "sys_version.h"


#ifdef _WIN32
#include <windows.h>
extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif // _WIN32

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",CV_INVISIBLE);

#if defined(PLAT_ANDROID)
#include "SDL_messagebox.h"
#include "SDL_system.h"

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

#endif // PLAT_ANDROID

#ifdef PLAT_WIN
int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hLastInst, LPSTR lpszCmdLine, int nCmdShow)
{
#if defined(CRT_DEBUG_ENABLED)
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

#ifdef PLAT_ANDROID

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

struct JNI_t
{
	JNIEnv* env;
	AAssetManager* assetManager;
	EqString packageName;
	EqString obbPath;
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


	{
		jmethodID jMethod_id_pn = env->GetMethodID(class_activity, "getPackageName", "()Ljava/lang/String;");
		jstring packageNameJstr = (jstring)env->CallObjectMethod(raw_activity, jMethod_id_pn);

		const char* packageNameUtfStr = env->GetStringUTFChars(packageNameJstr, 0);
		g_jni.packageName = packageNameUtfStr;
		env->ReleaseStringUTFChars(packageNameJstr, packageNameUtfStr);
	}
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
		if (!stricmp(filename, "EQ.CONFIG") ||
			!stricmp(filename, "eqBase.epk"))
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

	EqString dataPath("/data/" + g_jni.packageName + "/files");
	EqString obbPath("/obb/" + g_jni.packageName);

	EqString storageBase(_Es(bestStoragePath).Left(strlen(bestStoragePath) - dataPath.Length()));

	EqString storagePath(storageBase + dataPath);
	EqString storageObbPath(storageBase + obbPath);

	// first we let eqconfig to be found
	g_fileSystem->SetBasePath(storagePath.ToCString());

	g_jni.obbPath = storageObbPath;

	// init core
	bool result = GetCore()->Init("Game", argc, argv);

	Msg("bestStoragePath: %s\n", bestStoragePath);
	Msg("dataPath: %s\n", dataPath.ToCString());
	Msg("obbPath: %s\n", obbPath.ToCString());

	Msg("storagePath: %s\n", storagePath.ToCString());
	Msg("storageObbPath: %s\n", storageObbPath.ToCString());

	return result;
}

void Android_MountFileSystem()
{
	// current game directory needs to be created
	g_fileSystem->MakeDir(g_fileSystem->GetCurrentGameDirectory(), SP_ROOT);

	// as well as configuration file dir
	g_fileSystem->MakeDir("cfg", SP_MOD);

	// mount OBB file if available in config
	kvkeybase_t* filesystemKvs = GetCore()->GetConfig()->FindKeyBase("Filesystem");
	kvkeybase_t* obbPackageName = filesystemKvs->FindKeyBase("OBBPackage");

	if (obbPackageName)
	{
		EqString packageFullPath;
		CombinePath(packageFullPath, 2, g_jni.obbPath.ToCString(), KV_GetValueString(obbPackageName));
		g_fileSystem->AddPackage(packageFullPath.ToCString(), SP_MOD);
	}
}

#endif

// posix apps
int main(int argc, char** argv)
{
#ifdef PLAT_ANDROID
    Android_InitJNI(); // initialize JNI

	if (!Android_InitCore(argc, argv))
		return -1;

	// mount OBB filesystem
	Android_MountFileSystem();
#else
	// init core
	if (!GetCore()->Init("Game", argc, argv))
		return -1;
#endif // PLAT_ANDROID

#endif

	// init file system
	if (!g_fileSystem->Init(false))
	{
		GetCore()->Shutdown();
		return -2;
	}

	// add the copyright
	g_localizer->AddToken("GAME_VERSION", EqWString::Format(L"Build %d %s %s", BUILD_NUMBER_ENGINE, L"" COMPILE_DATE, L"" COMPILE_TIME).ToCString());
	g_localizer->AddTokensFile("game");

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
