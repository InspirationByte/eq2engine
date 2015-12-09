//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQSWAPCHAIN_H
#define IEQSWAPCHAIN_H

#include "Platform.h"
#include "ITexture.h"

class IEqSwapChain
{
public:
	virtual ~IEqSwapChain() {}

	virtual void*			GetWindow() = 0;
	virtual int				GetMSAASamples() = 0;

	virtual ITexture*		GetBackbuffer() = 0;

	// retrieves backbuffer size for this swap chain
	virtual void			GetBackbufferSize(int& wide, int& tall) = 0;

	// sets backbuffer size for this swap chain
	virtual bool			SetBackbufferSize(int wide, int tall) = 0;

	// individual swapbuffers call
	virtual bool			SwapBuffers() = 0;
};

#endif // IEQSWAPCHAIN_H