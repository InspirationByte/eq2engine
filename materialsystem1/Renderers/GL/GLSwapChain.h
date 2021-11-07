//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLSWAPCHAIN_H
#define GLSWAPCHAIN_H

#include "renderers/IEqSwapChain.h"

class CGLSwapChain : public IEqSwapChain
{
	friend class CGLRenderLib;
public:
	CGLSwapChain();
	~CGLSwapChain();

	bool			Initialize(	HWND window,
								bool vSync, 
								bool windowed);

	void*			GetWindow() {return m_window;}
	int				GetMSAASamples() {return 1;}

	ITexture*		GetBackbuffer() {return NULL;}

	// retrieves backbuffer size for this swap chain
	void			GetBackbufferSize(int& wide, int& tall);

	// sets backbuffer size for this swap chain
	bool			SetBackbufferSize(int wide, int tall);

	// individual swapbuffers call
	bool			SwapBuffers();


protected:
	HWND			m_window;
	HDC				m_windowDC;

	bool			m_vSyncEnabled;
	int				m_width;
	int				m_height;
};

#endif // GLSWAPCHAIN_H