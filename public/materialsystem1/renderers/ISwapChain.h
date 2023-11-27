//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct RenderWindowInfo;

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class ISwapChain
{
public:
	virtual ~ISwapChain() = default;

	virtual void		SetVSync(bool enable) = 0;

	virtual void*		GetWindow()  const = 0;
	virtual ITexturePtr	GetBackbuffer() const = 0;

	virtual void		GetBackbufferSize(int& wide, int& tall) const = 0;
	virtual bool		SetBackbufferSize(int wide, int tall) = 0;
};

