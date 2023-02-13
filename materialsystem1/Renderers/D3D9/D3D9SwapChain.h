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
public:


	CD3D9SwapChain();
	~CD3D9SwapChain();

	bool			Initialize(	HWND window, bool vSync, bool windowed);

	void*			GetWindow() const { return m_window; }
	int				GetMSAASamples()  const { return 1; }	// TODO: return real information

	ITexturePtr		GetBackbuffer() const { return nullptr; }

	// retrieves backbuffer size for this swap chain
	void			GetBackbufferSize(int& wide, int& tall)  const;

	// sets backbuffer size for this swap chain
	bool			SetBackbufferSize(int wide, int tall);

	// individual swapbuffers call
	bool			SwapBuffers();

	//bool			CreateOrUpdateBackbuffer();

protected:
	HWND			m_window;

	bool			m_vSyncEnabled;
	int				m_width;
	int				m_height;
};