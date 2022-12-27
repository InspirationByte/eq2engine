//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 9 texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/CTexture.h"

static constexpr const int TEXTURE_TRANSFER_RATE_THRESHOLD = 512;
static constexpr const int TEXTURE_TRANSFER_MAX_TEXTURES_PER_FRAME = 15;

enum EProgressiveStatus : int
{
	PROGRESSIVE_STATUS_COMPLETED = 0,
	PROGRESSIVE_STATUS_WAIT_MORE_FRAMES,
	PROGRESSIVE_STATUS_DID_UPLOAD,
};

class CImage;
struct IDirect3DBaseTexture9;
struct IDirect3DSurface9;

class CD3D9Texture : public CTexture
{
public:
	friend class			ShaderAPID3DX9;

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

	struct LodState
	{
		CRefPtr<CImage> image;
		int8 idx{ 0 };
		int8 lockBoxLevel{ 0 };
		int8 mipMapLevel{ 0 };
		uint8 frameDelay{ 1 };
	};

	Future<bool>					m_mipLoading;
	Array<LodState>					m_progressiveState{ PP_SL };

	Array<IDirect3DBaseTexture9*>	textures{ PP_SL };
	Array<IDirect3DSurface9*>		surfaces{ PP_SL };
	IDirect3DSurface9*				m_dummyDepth{ nullptr };

	uint							m_pool{ 0 };
	uint							m_usage{ 0 };
	int								m_texSize{ 0 };

	IDirect3DSurface9*				m_lockSurface{ nullptr };
};