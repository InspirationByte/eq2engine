//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "NVRHIBackend.h"
#include "NVRHISwapChainVK.h"
#include "NVRHILibraryVK.h"
#include "NVRHIRenderDefs.h"

CNVRHISwapChainVK::CNVRHISwapChainVK(const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture)
	: m_winInfo(windowInfo)
{
	m_textureRef = CRefPtr<CNVRHITexture>(static_cast<CNVRHITexture*>(swapChainTexture.Ptr()));
	auto rhiDefaultTexViewDesc = nvrhi::TextureSubresourceSet()
		.setNumMipLevels(nvrhi::TextureSubresourceSet::AllMipLevels)
		.setNumArraySlices(nvrhi::TextureSubresourceSet::AllArraySlices);
	m_textureRef->m_rhiViews.append(rhiDefaultTexViewDesc);
	m_textureRef->m_format = FORMAT_RGBA8;

	/*
	m_dxgiSwapChainDesc = {};
	m_dxgiSwapChainDesc.Width = 800;
	m_dxgiSwapChainDesc.Height = 600;
	m_dxgiSwapChainDesc.SampleDesc.Count = 1;	// TODO
	m_dxgiSwapChainDesc.SampleDesc.Quality = 0; // TODO
	m_dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_dxgiSwapChainDesc.BufferCount = SWAP_CHAIN_BUFFERS;
	m_dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	m_dxgiSwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	if (CNVRHIRenderLibDXGIBase::Instance->m_dxgiTearingSupported)
		m_dxgiSwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	switch (m_swapChainFormat)
	{
	case nvrhi::Format::SRGBA8_UNORM:
		m_dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		m_textureRef->m_format = MakeTexFormat(FORMAT_RGBA8, TEXFORMAT_FLAG_SRGB);
		break;
	case nvrhi::Format::SBGRA8_UNORM:
		m_dxgiSwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		m_textureRef->m_format = MakeTexFormat(FORMAT_RGBA8, TEXFORMAT_FLAG_SWAP_RB | TEXFORMAT_FLAG_SRGB);
		break;
	default:
		m_dxgiSwapChainDesc.Format = nvrhi::d3d12::convertFormat(m_swapChainFormat);
		break;
	}
	//m_dxgiSwapChainDesc.Flags = (dxgi_maxFrameLatency.GetInt() > 0 ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0);
	*/
}

void* CNVRHISwapChainVK::GetWindow() const
{
	return m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
}

ITexturePtr CNVRHISwapChainVK::GetBackbuffer() const
{
	return ITexturePtr(m_textureRef);
}

void CNVRHISwapChainVK::UpdateBackbufferView() const
{
	//m_textureRef->m_rhiTexture = m_rhiSwapChainTextures[m_dxgiSwapChain->GetCurrentBackBufferIndex()];
}

void CNVRHISwapChainVK::GetBackbufferSize(int& wide, int& tall) const
{
	//wide = m_dxgiSwapChainDesc.Width;
	//tall = m_dxgiSwapChainDesc.Height;
}

void CNVRHISwapChainVK::SetVSync(bool enable)
{
	if ((m_vSync > 0) != enable)
		m_vSync = (int)enable;
}

bool CNVRHISwapChainVK::UpdateResize()
{
	/*
	if (m_textureRef->GetWidth() == m_dxgiSwapChainDesc.Width && m_textureRef->GetHeight() == m_dxgiSwapChainDesc.Height)
		return true;

	// Make sure all frame is finished
	nvrhi::DeviceHandle nvrhiDevice = CNVRHIRenderLibDXGIBase::Instance->m_nvrhiDevice;
	nvrhiDevice->waitForIdle();
	nvrhiDevice->runGarbageCollection();

	// release render targets
	m_d3d12SwapChainBuffers.clear();
	m_rhiSwapChainTextures.clear();
	m_textureRef->m_rhiTexture = nullptr;

	m_textureRef->SetDimensions(m_dxgiSwapChainDesc.Width, m_dxgiSwapChainDesc.Height);

	const HRESULT hr = m_dxgiSwapChain->ResizeBuffers(
		m_dxgiSwapChainDesc.BufferCount,
		m_dxgiSwapChainDesc.Width,
		m_dxgiSwapChainDesc.Height,
		m_dxgiSwapChainDesc.Format,
		m_dxgiSwapChainDesc.Flags);

	if (FAILED(hr))
	{
		MsgError("CNVRHISwapChainDXGI UpdateResize failed\n");
		return false;
	}

	CNVRHIRenderLibVK::Instance->CreateSwapchainTargets(this);
	m_textureRef->m_rhiTexture = nullptr;
	*/
	return true;
}

bool CNVRHISwapChainVK::SetBackbufferSize(int wide, int tall)
{
	//m_dxgiSwapChainDesc.Width = wide;
	//m_dxgiSwapChainDesc.Height = tall;
	return true;
}

bool CNVRHISwapChainVK::SwapBuffers()
{
	//UINT presentFlags = 0;
	//if (m_vSync == 0 && (m_dxgiSwapChainDesc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING))
	//	presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
	//
	//m_dxgiSwapChain->Present(m_vSync, presentFlags);

	return true;
}