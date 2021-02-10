//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D9SWAPCHAIN_H
#define D3D9SWAPCHAIN_H

#include "renderers/IEqSwapChain.h"
#include <d3d9.h>

class CD3D9SwapChain : public IEqSwapChain
{
	friend class		CD3DRenderLib;
public:


	CD3D9SwapChain();
	~CD3D9SwapChain();

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

	//bool			CreateOrUpdateBackbuffer();

protected:
	HWND			m_window;

	//CD3D9Texture*	m_backbuffer;
	bool			m_vSyncEnabled;
	int				m_width;
	int				m_height;
};

#endif // D3D9SWAPCHAIN_H