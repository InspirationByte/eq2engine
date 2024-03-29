//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////////////

#pragma once

EQWNDHANDLE Sys_CreateWindow();

void Sys_GetWindowConfig(bool& fullscreen, int& screen, int& wide, int& tall);

bool Host_Init();
void Host_GameLoop();
void Host_Terminate();
