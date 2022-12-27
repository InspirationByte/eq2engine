//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 9 texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/CTexture.h"

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
	bool					Init(const SamplerStateParam_t& sampler, const ArrayCRef<CImage*> images, int flags = 0);

	void					Release();
	void					ReleaseTextures();
	void					ReleaseSurfaces();

	IDirect3DBaseTexture9*	CreateD3DTexture(EImageType type, ETextureFormat format, int mipCount, int widthMip0, int heightMip0, int depthMip0 = 1) const;
	IDirect3DBaseTexture9*	GetCurrentTexture();

	// locks texture for modifications, etc
	void					Lock(LockData* pLockData, Rectangle_t* pRect = nullptr, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);
	
	// unlocks texture for modifications, etc
	void					Unlock();

	Array<IDirect3DBaseTexture9*>	textures{ PP_SL };
	Array<IDirect3DSurface9*>		surfaces{ PP_SL };
	IDirect3DSurface9*				m_dummyDepth{ nullptr };

	uint					m_pool{ 0 };
	uint					m_usage{ 0 };
	int						m_texSize{ 0 };

	bool					m_bIsLocked{ false };
	ushort					m_nLockLevel{ 0 };
	ushort					m_nLockCube{ 0 };
	IDirect3DSurface9*		m_pLockSurface{ nullptr };
};