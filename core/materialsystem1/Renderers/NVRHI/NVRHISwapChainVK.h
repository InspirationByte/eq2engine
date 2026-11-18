//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/vulkan.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "renderers/ISwapChain.h"
#include "renderers/ShaderAPI_defs.h"
#include "NVRHITexture.h"

class CNVRHIRenderLibVK;
using nvrhi::RefCountPtr;

class CNVRHISwapChainVK : public ISwapChain
{
public:
	friend class CNVRHIRenderLibVK;

	CNVRHISwapChainVK(const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture);
	~CNVRHISwapChainVK();

	void			SetVSync(bool enable);

	void*			GetWindow() const;
	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();

	bool			UpdateResize();

protected:

	void			UpdateBackbufferView();

	vk::SurfaceKHR				m_vkWindowSurface;
	vk::SwapchainKHR			m_vkSwapChain;

	Array<nvrhi::TextureHandle>	m_rhiSwapChainTextures{ PP_SL };
	Array<vk::Image>			m_vkSwapChainBuffers{ PP_SL };
	// one signal semaphore per swapchain image, selected by the acquired image index
	Array<vk::Semaphore>		m_vkPresentSemaphores{ PP_SL };
	// acquire semaphores are consumed by the next graphics queue submit, so they rotate
	Array<vk::Semaphore>		m_vkAcquireSemaphores{ PP_SL };
	vk::Semaphore				m_vkCurrentPresentSemaphore;

	uint						m_swapChainBufferIndex{ COM_UINT_MAX };
	uint						m_vkAcquireSemaphoreIndex{ 0 };

	nvrhi::Format				m_swapChainFormat{ nvrhi::Format::RGBA8_UNORM };
	CRefPtr<CNVRHITexture>		m_textureRef;

	RenderWindowInfo			m_winInfo;
	IVector2D					m_swapChainDimensions{ 0 };
	int							m_vSync{ 0 };
};