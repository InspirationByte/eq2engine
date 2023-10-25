//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ISwapChain.h"

class CD3D9SwapChain : public ISwapChain
{
public:
	bool		Initialize(const RenderWindowInfo& windowInfo);

	void*		GetWindow() const { return m_window; }
	ITexturePtr	GetBackbuffer() const { return nullptr; }

	void		GetBackbufferSize(int& wide, int& tall)  const;
	bool		SetBackbufferSize(int wide, int tall);
protected:
	HWND		m_window{ nullptr };
	int			m_width{ 0 };
	int			m_height{ 0 };
};