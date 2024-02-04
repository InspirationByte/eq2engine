//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entry points for various platforms
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ILocalize.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "utils/KeyValues.h"

#include "sys_in_console.h"
#include "sys_window.h"
#include "sys_version.h"

#include <SDL_messagebox.h>
#include <SDL_system.h>

#define DEFAULT_CONFIG_PATH "cfg/config_default.cfg"

#ifdef _RETAIL
#define CHEATS_DEFAULT_VALUE 0
#else
#define CHEATS_DEFAULT_VALUE 1
#endif

DECLARE_CVAR_G(__cheats, QUOTE(CHEATS_DEFAULT_VALUE), "Wireframe", CV_PROTECTED | CV_INVISIBLE);

// To not use GTK or java messages, we just using SDL for it. Neat. Noice.
static int EQSDLMessageBoxCallback(const char* messageStr, const char* titleStr, EMessageBoxType type)
{
	switch(type)
	{
		case MSGBOX_INFO:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, titleStr, messageStr, nullptr);
			break;
		case MSGBOX_WARNING:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, titleStr, messageStr, nullptr);
			break;
		case MSGBOX_ERROR:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, titleStr, messageStr, nullptr);
			break;
		case MSGBOX_CRASH:
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, titleStr, messageStr, nullptr);
			break;
		case MSGBOX_YESNO:
		{
			const SDL_MessageBoxButtonData buttons[] = {
				{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, MSGBOX_BUTTON_YES, "Yes" },
				{ 0, MSGBOX_BUTTON_NO, "No"},
			};

			SDL_MessageBoxData msgBox{};
			msgBox.window = nullptr;
			msgBox.title = titleStr;
			msgBox.buttons = buttons;
			msgBox.numbuttons = elementsOf(buttons);
			msgBox.flags = SDL_MESSAGEBOX_INFORMATION;
			msgBox.message = messageStr;

			int buttonId = -1;
			SDL_ShowMessageBox(&msgBox, &buttonId);

			return buttonId;
		}
		case MSGBOX_ABORTRETRYINGORE:
		{
			const SDL_MessageBoxButtonData buttons[] = {
				{ 0, MSGBOX_BUTTON_ABORT, "Abort" },
				{ SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, MSGBOX_BUTTON_RETRY, "Retry"},
				{ SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, MSGBOX_BUTTON_IGNORE, "Ignore" },
			};

			SDL_MessageBoxData msgBox{};
			msgBox.window = nullptr;
			msgBox.title = titleStr;
			msgBox.buttons = buttons;
			msgBox.numbuttons = elementsOf(buttons);
			msgBox.flags = SDL_MESSAGEBOX_ERROR;
			msgBox.message = messageStr;

			int buttonId = -1;
			SDL_ShowMessageBox(&msgBox, &buttonId);

			return buttonId;
		}
	}

	return 0;
}

static void Sys_InitConfiguration()
{
	const int userCfgIdx = g_cmdLine->FindArgument("-user_cfg");
	if (userCfgIdx != -1)
	{
		extern ConVar user_cfg;
		EqString cfgFileName(g_cmdLine->GetArgumentsOf(userCfgIdx));
		user_cfg.SetValue(cfgFileName.TrimChar('\"').ToCString());
	}

	// execute configuration files and command line after all libraries are loaded.
	g_consoleCommands->ClearCommandBuffer();
	g_consoleCommands->ParseFileToCommandBuffer(DEFAULT_CONFIG_PATH);
	g_consoleCommands->ExecuteCommandBuffer();
}

// engine entry point after Core init
int Sys_Main()
{
	SetMessageBoxCallback(EQSDLMessageBoxCallback);

	// init file system
	if (!g_fileSystem->Init(false))
	{
		g_eqCore->Shutdown();
		return -2;
	}

	Sys_InitConfiguration();

	g_localizer->Init();

	// in case of game FS is packed
	// create configuration directory
	g_fileSystem->MakeDir("cfg", SP_MOD);
	g_localizer->AddToken("GAME_VERSION", EqWString::Format(L"Build %d %ls %ls", BUILD_NUMBER_ENGINE, L"" COMPILE_DATE, L"" COMPILE_TIME).ToCString());
	g_localizer->AddTokensFile("game");

	if (!Host_Init())
	{
		// shutdown
		g_fileSystem->Shutdown();
		g_eqCore->Shutdown();
		return -3;
	}

	Host_GameLoop();

	CEqConsoleInput::SpewClear();

	Host_Terminate();

	// shutdown
	g_localizer->Shutdown();
	g_fileSystem->Shutdown();
	g_eqCore->Shutdown();

	return 0;
}

#if defined(PLAT_ANDROID)
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <vector>

struct JNI_t
{
	JNIEnv* env;
	AAssetManager* assetManager;
	EqString packageName;
	EqString obbPath;
} g_jni;

// init Java Native Interface glue parts required to do some operations
void Sys_Android_InitJNI()
{
	SetMessageBoxCallback(EQSDLMessageBoxCallback);

	JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
	g_jni.env = env;
	jclass class_activity = env->FindClass("android/app/Activity");
	jclass class_resources = env->FindClass("android/content/res/Resources");
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
bool Sys_Android_InitCore(int argc, char** argv)
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
	char filenameBuffer[2048] = { 0 };

	while ((filename = AAssetDir_getNextFileName(assetDir)) != nullptr)
	{
		//search for desired file
		if (!stricmp(filename, "EQ.CONFIG") ||
			!stricmp(filename, "eqBase.epk"))
		{
			AAsset* asset = AAssetManager_open(g_jni.assetManager, filename, AASSET_MODE_STREAMING);

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
			if (fp)
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
	bool result = g_eqCore->Init("Game", argc, argv);

	Msg("bestStoragePath: %s\n", bestStoragePath);
	Msg("dataPath: %s\n", dataPath.ToCString());
	Msg("obbPath: %s\n", obbPath.ToCString());

	Msg("storagePath: %s\n", storagePath.ToCString());
	Msg("storageObbPath: %s\n", storageObbPath.ToCString());

	return result;
}

void Sys_Android_MountFileSystem()
{
	// mount OBB file if available in config
	KVSection* filesystemKvs = g_eqCore->GetConfig()->FindSection("Filesystem");
	KVSection* obbPackageName = filesystemKvs->FindSection("OBBPackage");

	if (obbPackageName)
	{
		EqString packageFullPath;
		CombinePath(packageFullPath, g_jni.obbPath.ToCString(), KV_GetValueString(obbPackageName));
		g_fileSystem->AddPackage(packageFullPath.ToCString(), SP_MOD);
	}
}

#endif // PLAT_ANDROID

#if defined(PLAT_WIN)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef CRT_DEBUG_ENABLED
#include <crtdbg.h>
#endif

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

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
	if(!g_eqCore->Init("Game", lpszCmdLine))
		return -1;

	return Sys_Main();
}

#else

// posix main (Android, Linux, Apple)
int main(int argc, char** argv)
{
	CEqConsoleInput::SpewInit();

#if defined(PLAT_ANDROID)
	Sys_Android_InitJNI(); // initialize JNI

	if (!Sys_Android_InitCore(argc, argv))
		return -1;

	// mount OBB filesystem
	Sys_Android_MountFileSystem();
#else
	// init core
	if (!g_eqCore->Init("Game", argc, argv))
		return -1;
#endif // PLAT_ANDROID

	return Sys_Main();
}

#endif