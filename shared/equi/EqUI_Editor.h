//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EqUI editor
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#if !defined(_RETAIL) && !defined(_PROFILE)
#define EQUI_EDITOR
#endif

// draws the editor window, used as an ImGui debug menu handler
void	equiEditorUIDraw(bool& open);

// opens a layout file in the editor
bool	equiEditorOpenFile(const char* filePath);

// destroys preview controls and targets, must be called before equi::Manager->Shutdown()
void	equiEditorShutdown();
