//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 impl of textures for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CTexture.h"

class IRenderState;

class CD3D10Texture : public CTexture
{
	friend class CD3D11RenderLib;
	friend class CD3D10SwapChain;
	friend class ShaderAPID3DX10;
public:
	~CD3D10Texture();

	// initializes texture from image array of images
	bool					Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	// locks texture for modifications, etc
	bool					Lock(LockInOutData& data);

	// unlocks texture for modifications, etc
	void					Unlock();

protected:
	void					Ref_DeleteObject();
	void					Release();

	Array<ID3D10ShaderResourceView*>	m_srv{ PP_SL };
	Array<ID3D10RenderTargetView*>		m_rtv{ PP_SL };
	Array<ID3D10DepthStencilView*>		m_dsv{ PP_SL };

	Array<ID3D10Resource*>	m_textures{ PP_SL };

	IRenderState*	m_samplerState; // FIXME: do we want it separate or per-texture?

	DXGI_FORMAT		m_texFormat{DXGI_FORMAT_UNKNOWN};
	DXGI_FORMAT		m_srvFormat{DXGI_FORMAT_UNKNOWN};
	DXGI_FORMAT		m_rtvFormat{DXGI_FORMAT_UNKNOWN};
	DXGI_FORMAT		m_dsvFormat{DXGI_FORMAT_UNKNOWN};

	int				m_arraySize{ 1 };
	int				m_depth{ 1 };
	int				m_texSize{ 0 };
};

