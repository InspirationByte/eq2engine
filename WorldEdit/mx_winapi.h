//**************** Copyright (C) Parallel Prevision, L.L.C 2012 ******************
//
// Description: MX Toolkit tools for windows
//
//********************************************************************************

#ifndef MX_WINAPI_H
#define MX_WINAPI_H

#include <Windows.h>
#include "mx/mx.h"

inline void mxSetWindowStyle(mxWindow* pWindow, LONG style, bool enable)
{
	LONG nstyle = GetWindowLong((HWND)pWindow->getHandle(), GWL_STYLE);

	if(enable)
		nstyle |= style;
	else
		nstyle &= ~style;

	SetWindowLong((HWND)pWindow->getHandle(), GWL_STYLE, nstyle);
}

inline void mxSetWindowExStyle(mxWindow* pWindow, LONG style, bool enable)
{
	LONG nstyle = GetWindowLong((HWND)pWindow->getHandle(), GWL_EXSTYLE);

	if(enable)
		nstyle |= style;
	else
		nstyle &= ~style;

	SetWindowLong((HWND)pWindow->getHandle(), GWL_EXSTYLE, nstyle);
}

#endif // MX_WINAPI_H