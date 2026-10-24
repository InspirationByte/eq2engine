//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - native file dialogs
//////////////////////////////////////////////////////////////////////////////////

#pragma once

bool	FileDialogInit();
void	FileDialogShutdown();

// filterSpec is a comma-separated list of extensions without dots, e.g. "esc,asc"
bool	FileDialogOpen(EqString& outPath, const char* title, const char* filterName, const char* filterSpec, const char* defaultPath);
bool	FileDialogOpenMultiple(Array<EqString>& outPaths, const char* title, const char* filterName, const char* filterSpec, const char* defaultPath);
