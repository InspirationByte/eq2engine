//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 9 texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CTexture.h"

class CImage;
struct IDirect3DBaseTexture9;
struct IDirect3DSurface9;

class CD3D9Texture : public CTexture
{
public:
	friend class			ShaderAPID3D9;

	CD3D9Texture();
	~CD3D9Texture();

	// initializes texture from image array of images
	bool					Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	void					Release();
	void					ReleaseTextures();
	void					ReleaseSurfaces();

	void					ReleaseForRestoration();
	void					Restore();

	IDirect3DBaseTexture9*	CreateD3DTexture(EImageType type, ETextureFormat format, int mipCount, int widthMip0, int heightMip0, int depthMip0 = 1) const;
	IDirect3DBaseTexture9*	GetCurrentTexture();

	EProgressiveStatus		StepProgressiveLod();

	// locks texture for modifications, etc
	bool					Lock(LockInOutData& data);
	
	// unlocks texture for modifications, etc
	void					Unlock();

	void					Ref_DeleteObject();

protected:
	Array<IDirect3DBaseTexture9*>	m_textures{ PP_SL };
	Array<IDirect3DSurface9*>		m_surfaces{ PP_SL };
	IDirect3DSurface9*				m_dummyDepth{ nullptr };

	uint							m_pool{ 0 };
	uint							m_usage{ 0 };
	int								m_texSize{ 0 };

	IDirect3DSurface9*				m_lockSurface{ nullptr };
};