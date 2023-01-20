//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IEqSwapChain.h"

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

	ITexturePtr		GetBackbuffer() {return nullptr;}

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