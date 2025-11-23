//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"

#include "NVRHIBackend.h"
#include "NVRHISwapChainVK.h"
#include "NVRHILibraryVK.h"
#include "NVRHIRenderDefs.h"

DECLARE_CVAR_F(vulkan_fastsync);

CNVRHISwapChainVK::CNVRHISwapChainVK(const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture)
	: m_winInfo(windowInfo)
{
	m_textureRef = CRefPtr<CNVRHITexture>(static_cast<CNVRHITexture*>(swapChainTexture.Ptr()));
	auto rhiDefaultTexViewDesc = nvrhi::TextureSubresourceSet()
		.setNumMipLevels(nvrhi::TextureSubresourceSet::AllMipLevels)
		.setNumArraySlices(nvrhi::TextureSubresourceSet::AllArraySlices);
	m_textureRef->m_rhiViews.append(rhiDefaultTexViewDesc);
	m_textureRef->m_format = FORMAT_RGBA8;

	m_swapChainDimensions.x = 800;
	m_swapChainDimensions.y = 600;

	if (m_swapChainFormat == nvrhi::Format::SRGBA8_UNORM)
	{
		m_swapChainFormat = nvrhi::Format::SBGRA8_UNORM;
		m_textureRef->m_format = MakeTexFormat(FORMAT_RGBA8, TEXFORMAT_FLAG_SWAP_RB | TEXFORMAT_FLAG_SRGB);
	}
	else if (m_swapChainFormat == nvrhi::Format::RGBA8_UNORM)
	{
		m_swapChainFormat = nvrhi::Format::BGRA8_UNORM;
		m_textureRef->m_format = MakeTexFormat(FORMAT_RGBA8, TEXFORMAT_FLAG_SWAP_RB);
	}
}

void* CNVRHISwapChainVK::GetWindow() const
{
	return m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
}

ITexturePtr CNVRHISwapChainVK::GetBackbuffer() const
{
	return ITexturePtr(m_textureRef);
}

void CNVRHISwapChainVK::UpdateBackbufferView()
{
	nvrhi::vulkan::DeviceHandle nvrhiDevice = CNVRHIRenderLibVK::Instance->m_nvrhiDevice;
	vk::Device vkDevice = CNVRHIRenderLibVK::Instance->m_vkDevice;

	const vk::Result res = vkDevice.acquireNextImageKHR(m_vkSwapChain,
		std::numeric_limits<uint64_t>::max(),
		m_vkPresentSemaphore,
		vk::Fence(),
		&m_swapChainBufferIndex);

	assert(res == vk::Result::eSuccess || res == vk::Result::eSuboptimalKHR);

	nvrhiDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, m_vkPresentSemaphore, 0);

	if (!m_rhiSwapChainTextures.inRange(m_swapChainBufferIndex))
	{
		m_textureRef->m_rhiTexture = nullptr;
		return;
	}
	m_textureRef->m_rhiTexture = m_rhiSwapChainTextures[m_swapChainBufferIndex];
}

void CNVRHISwapChainVK::GetBackbufferSize(int& wide, int& tall) const
{
	wide = m_swapChainDimensions.x;
	tall = m_swapChainDimensions.y;
}

void CNVRHISwapChainVK::SetVSync(bool enable)
{
	if ((m_vSync > 0) != enable)
		m_vSync = (int)enable;
}

bool CNVRHISwapChainVK::UpdateResize()
{
	if (m_textureRef->GetWidth() == m_swapChainDimensions.x && m_textureRef->GetHeight() == m_swapChainDimensions.y)
		return true;

	m_textureRef->SetDimensions(m_swapChainDimensions.x, m_swapChainDimensions.y);

	vk::Device vkDevice = CNVRHIRenderLibVK::Instance->m_vkDevice;
	vk::PhysicalDevice vkPhysicalDevice = CNVRHIRenderLibVK::Instance->m_vkPhysicalDevice;
	nvrhi::DeviceHandle nvrhiDevice = CNVRHIRenderLibVK::Instance->m_nvrhiDevice;

	const bool enablePModeMailbox = CNVRHIRenderLibVK::Instance->m_enablePModeMailbox;
	const bool enablePModeImmediate = CNVRHIRenderLibVK::Instance->m_enablePModeImmediate;
	const bool enablePModeFifoRelaxed = CNVRHIRenderLibVK::Instance->m_enablePModeFifoRelaxed;

	vk::SurfaceFormatKHR vkSurfaceFormat =
	{
		vk::Format(nvrhi::vulkan::convertFormat(m_swapChainFormat)),
		vk::ColorSpaceKHR::eSrgbNonlinear
	};

	// Clamp swap chain extent within the range supported by the device / window surface
	auto surfaceCaps = vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_vkWindowSurface);
	const int backBufferWidth = clamp(m_swapChainDimensions.x, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
	const int backBufferHeight = clamp(m_swapChainDimensions.y, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);

	// set up Vulkan present mode based on vsync setting and available surface features
	vk::PresentModeKHR presentMode;
	switch (m_vSync)
	{
	case 0:
		presentMode = enablePModeMailbox && vulkan_fastsync.GetBool() ? vk::PresentModeKHR::eMailbox :
					 (enablePModeImmediate ? vk::PresentModeKHR::eImmediate : vk::PresentModeKHR::eFifo);
		break;
	case 1:
		presentMode = enablePModeFifoRelaxed ? vk::PresentModeKHR::eFifoRelaxed : vk::PresentModeKHR::eFifo;
		break;
	case 2:
	default:
		presentMode = vk::PresentModeKHR::eFifo;	// eFifo always supported according to Vulkan spec
	}

	FixedArray<uint32_t, 4> queues;
	queues.addUnique(CNVRHIRenderLibVK::Instance->m_vkGraphicsQueueFamily);
	queues.addUnique(CNVRHIRenderLibVK::Instance->m_vkPresentQueueFamily);

	const bool enableSwapChainSharing = queues.numElem() > 1;

	auto desc = vk::SwapchainCreateInfoKHR()
		.setSurface(m_vkWindowSurface)
		.setMinImageCount(CNVRHIRenderLibVK::Instance->m_swapChainBufferCount)
		.setImageFormat(vkSurfaceFormat.format)
		.setImageColorSpace(vkSurfaceFormat.colorSpace)
		.setImageExtent(vk::Extent2D(backBufferWidth, backBufferHeight))
		.setImageArrayLayers(1)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
		.setImageSharingMode(enableSwapChainSharing ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
		.setQueueFamilyIndexCount(enableSwapChainSharing ? queues.numElem() : 0)
		.setPQueueFamilyIndices(enableSwapChainSharing ? queues.ptr() : nullptr)
		.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
		.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
		.setPresentMode(presentMode)
		.setClipped(true)
		.setOldSwapchain(m_vkSwapChain);

	m_rhiSwapChainTextures.clear();
	m_vkSwapChainBuffers.clear();
	m_vkPresentSemaphoreQueue.clear();
	m_vkPresentSemaphore = nullptr;
	m_vkSwapChain = nullptr;

	const vk::Result res = vkDevice.createSwapchainKHR(&desc, nullptr, &m_vkSwapChain);
	if (res != vk::Result::eSuccess)
	{
		MsgError("Failed to create a Vulkan swap chain, error code = %s", nvrhi::vulkan::resultToString((VkResult)res));
		return false;
	}

	// retrieve swap chain images
	auto images = vkDevice.getSwapchainImagesKHR(m_vkSwapChain);
	for (auto image : images)
	{
		nvrhi::TextureDesc textureDesc;
		textureDesc.width = backBufferWidth;
		textureDesc.height = backBufferHeight;
		textureDesc.format = m_swapChainFormat;
		textureDesc.debugName = m_textureRef->GetName();
		textureDesc.isRenderTarget = true;
		textureDesc.isUAV = false;
		textureDesc.initialState = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;

		nvrhi::TextureHandle rhiHandle = nvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, nvrhi::Object(image), textureDesc);
		m_rhiSwapChainTextures.append(rhiHandle);
		m_vkSwapChainBuffers.append(image);

		// Give each swapchain image its own semaphore in case of overlap (e.g.MoltenVK async queue submit)
		m_vkPresentSemaphoreQueue.append(vkDevice.createSemaphore(vk::SemaphoreCreateInfo()));
	}
	m_vkPresentSemaphore = m_vkPresentSemaphoreQueue.front();

	m_textureRef->m_rhiTexture = nullptr;

	return true;
}

bool CNVRHISwapChainVK::SetBackbufferSize(int wide, int tall)
{
	m_swapChainDimensions.x = wide;
	m_swapChainDimensions.y = tall;
	return true;
}

bool CNVRHISwapChainVK::SwapBuffers()
{
	nvrhi::vulkan::DeviceHandle nvrhiDevice = CNVRHIRenderLibVK::Instance->m_nvrhiDevice;
	nvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, m_vkPresentSemaphore, 0);

	void* pNext = nullptr;
	vk::PresentInfoKHR info = vk::PresentInfoKHR()
		.setWaitSemaphoreCount(1)
		.setPWaitSemaphores(&m_vkPresentSemaphore)
		.setSwapchainCount(1)
		.setPSwapchains(&m_vkSwapChain)
		.setPImageIndices(&m_swapChainBufferIndex)
		.setPNext(pNext);

	const vk::Result res = CNVRHIRenderLibVK::Instance->m_vkPresentQueue.presentKHR(&info);
	ASSERT_MSG(res == vk::Result::eSuccess || res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR, "Present failure");

	// cycle the semaphore queue and setup presentSemaphore for the next swapchain image
	m_vkPresentSemaphoreQueue.popFront();
	m_vkPresentSemaphoreQueue.append(m_vkPresentSemaphore);
	m_vkPresentSemaphore = m_vkPresentSemaphoreQueue.front();

	return true;
}