//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class IEqSwapChain
{
public:
	virtual ~IEqSwapChain() {}

	virtual void*			GetWindow() = 0;
	virtual int				GetMSAASamples() = 0;

	virtual ITexturePtr		GetBackbuffer() = 0;

	// retrieves backbuffer size for this swap chain
	virtual void			GetBackbufferSize(int& wide, int& tall) = 0;

	// sets backbuffer size for this swap chain
	virtual bool			SetBackbufferSize(int wide, int tall) = 0;

	// individual swapbuffers call
	virtual bool			SwapBuffers() = 0;
};

