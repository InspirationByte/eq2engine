//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - native file dialogs
//////////////////////////////////////////////////////////////////////////////////

#include <nfd.h>
#include <nfd_sdl2.h>

#include "core/core_common.h"
#include "sys/sys_host.h"

#include "egfman2_filedialog.h"

static nfdwindowhandle_t FileDialogParentWindow()
{
	nfdwindowhandle_t parentWindow{};
	if (g_pHost && g_pHost->GetWindowHandle())
		NFD_GetNativeWindowFromSDLWindow(g_pHost->GetWindowHandle(), &parentWindow);
	return parentWindow;
}

static void FileDialogLogError(const char* dialogTitle)
{
	MsgError("File dialog '%s' failed: %s\n", dialogTitle, NFD_GetError());
	NFD_ClearError();
}

bool FileDialogInit()
{
	if (NFD_Init() != NFD_OKAY)
	{
		MsgError("Failed to initialize native file dialogs: %s\n", NFD_GetError());
		return false;
	}
	return true;
}

void FileDialogShutdown()
{
	NFD_Quit();
}

bool FileDialogOpen(EqString& outPath, const char* title, const char* filterName, const char* filterSpec, const char* defaultPath)
{
	// IFileDialog does not accept forward slashes in default path
	EqString nativeDefaultPath(defaultPath);
	fnmPathFixSeparators(nativeDefaultPath);

	const nfdu8filteritem_t filterItem{ filterName, filterSpec };

	nfdopendialogu8args_t args{};
	args.filterList = &filterItem;
	args.filterCount = 1;
	args.defaultPath = nativeDefaultPath.Length() ? nativeDefaultPath.ToCString() : nullptr;
	args.parentWindow = FileDialogParentWindow();
	args.title = title;

	nfdu8char_t* resultPath = nullptr;
	const nfdresult_t result = NFD_OpenDialogU8_With(&resultPath, &args);
	if (result == NFD_ERROR)
		FileDialogLogError(title);

	if (result != NFD_OKAY)
		return false;

	outPath = resultPath;
	NFD_FreePathU8(resultPath);
	return true;
}

bool FileDialogOpenMultiple(Array<EqString>& outPaths, const char* title, const char* filterName, const char* filterSpec, const char* defaultPath)
{
	EqString nativeDefaultPath(defaultPath);
	fnmPathFixSeparators(nativeDefaultPath);

	const nfdu8filteritem_t filterItem{ filterName, filterSpec };

	nfdopendialogu8args_t args{};
	args.filterList = &filterItem;
	args.filterCount = 1;
	args.defaultPath = nativeDefaultPath.Length() ? nativeDefaultPath.ToCString() : nullptr;
	args.parentWindow = FileDialogParentWindow();
	args.title = title;

	const nfdpathset_t* pathSet = nullptr;
	const nfdresult_t result = NFD_OpenDialogMultipleU8_With(&pathSet, &args);
	if (result == NFD_ERROR)
		FileDialogLogError(title);

	if (result != NFD_OKAY)
		return false;

	defer{
		NFD_PathSet_Free(pathSet);
	};

	nfdpathsetsize_t numPaths = 0;
	if (NFD_PathSet_GetCount(pathSet, &numPaths) != NFD_OKAY)
	{
		FileDialogLogError(title);
		return false;
	}

	for (nfdpathsetsize_t i = 0; i < numPaths; ++i)
	{
		nfdu8char_t* path = nullptr;
		if (NFD_PathSet_GetPathU8(pathSet, i, &path) != NFD_OKAY)
		{
			FileDialogLogError(title);
			continue;
		}

		outPaths.append(path);
		NFD_PathSet_FreePathU8(path);
	}

	return outPaths.numElem() > 0;
}
