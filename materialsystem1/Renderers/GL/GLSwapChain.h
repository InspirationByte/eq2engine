//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ISwapChain.h"
#include "renderers/ITexture.h"

class CGLSwapChain : public ISwapChain
{
	friend class CGLRenderLib_WGL;
	friend class CGLRenderLib_GLX;
	friend class CGLRenderLib_EGL;
public:
	~CGLSwapChain();

	bool			Initialize(const RenderWindowInfo& windowInfo);

	void			SetVSync(bool enable) { m_vSync = enable; }

	void*			GetWindow() const;
	ITexturePtr		GetBackbuffer() const { return nullptr; }

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);
protected:
#ifdef PLAT_WIN
	HWND			m_window;
	HDC				m_windowDC;
#endif
	int				m_width{ 0 };
	int				m_height{ 0 };

	bool			m_vSync{ false };
};