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

#include <d3d9.h>

class CImage;

class CD3D9Texture : public CTexture
{
public:
	friend class			ShaderAPID3DX9;

	CD3D9Texture();
	~CD3D9Texture();

	void					Release();
	void					ReleaseTextures();
	void					ReleaseSurfaces();

	LPDIRECT3DBASETEXTURE9	GetCurrentTexture();

	// locks texture for modifications, etc
	void					Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);
	
	// unlocks texture for modifications, etc
	void					Unlock();

	Array<LPDIRECT3DBASETEXTURE9>	textures{ PP_SL };
	Array<LPDIRECT3DSURFACE9>		surfaces{ PP_SL };
	LPDIRECT3DSURFACE9		m_dummyDepth;

	D3DPOOL					m_pool;
	DWORD					usage;
	int						m_texSize;

	bool					m_bIsLocked;
	ushort					m_nLockLevel;
	ushort					m_nLockCube;
	LPDIRECT3DSURFACE9		m_pLockSurface;
};

bool UpdateD3DTextureFromImage(IDirect3DBaseTexture9* texture, CImage* image, int startMipLevel, bool convert);

#endif //D3D9TEXTURE_H