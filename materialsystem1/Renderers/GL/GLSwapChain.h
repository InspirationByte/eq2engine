//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IEqSwapChain.h"

class CGLSwapChain : public IEqSwapChain
{
	friend class CGLRenderLib;
public:
	~CGLSwapChain();

	bool			Initialize(void* window, bool vSync, bool windowed);

	void*			GetWindow() const;
	int				GetMSAASamples() const { return 1; }

	ITexturePtr		GetBackbuffer() const { return nullptr; }

	// retrieves backbuffer size for this swap chain
	void			GetBackbufferSize(int& wide, int& tall) const;

	// sets backbuffer size for this swap chain
	bool			SetBackbufferSize(int wide, int tall);

	// individual swapbuffers call
	bool			SwapBuffers();


protected:
#ifdef PLAT_WIN
	HWND			m_window;
	HDC				m_windowDC;
#endif

	bool			m_vSyncEnabled;
	int				m_width;
	int				m_height;
};