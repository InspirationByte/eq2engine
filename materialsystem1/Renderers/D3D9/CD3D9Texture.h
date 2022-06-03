//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 9 texture class
//////////////////////////////////////////////////////////////////////////////////


#ifndef D3D9TEXTURE_H
#define D3D9TEXTURE_H

#include "../Shared/CTexture.h"
#include "ds/Array.h"

class CImage;
struct IDirect3DBaseTexture9;
struct IDirect3DSurface9;

class CD3D9Texture : public CTexture
{
public:
	friend class			ShaderAPID3DX9;

	CD3D9Texture();
	~CD3D9Texture();

	void					Release();
	void					ReleaseTextures();
	void					ReleaseSurfaces();

	IDirect3DBaseTexture9*	GetCurrentTexture();

	// locks texture for modifications, etc
	void					Lock(LockData* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);
	
	// unlocks texture for modifications, etc
	void					Unlock();

	Array<IDirect3DBaseTexture9*>	textures{ PP_SL };
	Array<IDirect3DSurface9*>		surfaces{ PP_SL };
	IDirect3DSurface9*		m_dummyDepth;

	uint					m_pool;
	uint					usage;
	int						m_texSize;

	bool					m_bIsLocked;
	ushort					m_nLockLevel;
	ushort					m_nLockCube;
	IDirect3DSurface9*		m_pLockSurface;
};

bool UpdateD3DTextureFromImage(IDirect3DBaseTexture9* texture, CImage* image, int startMipLevel, bool convert);

#endif //D3D9TEXTURE_H