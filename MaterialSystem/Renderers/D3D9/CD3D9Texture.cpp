//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 9 texture class
//////////////////////////////////////////////////////////////////////////////////

#include "CD3D9Texture.h"
#include "ShaderAPID3DX9.h"
#include "d3dx9_def.h"
#include "DebugInterface.h"


CD3D9Texture::CD3D9Texture() : CTexture()
{
	m_nLockLevel = 0;
	m_bIsLocked = false;
	m_pLockSurface = NULL;
	m_texSize = 0;
}

CD3D9Texture::~CD3D9Texture()
{
	Release();
}

void CD3D9Texture::Release()
{
	ReleaseTextures();
	ReleaseSurfaces();
}

void CD3D9Texture::ReleaseTextures()
{
	for (int i = 0; i < textures.numElem(); i++)
		textures[i]->Release();

	textures.clear();
}

void CD3D9Texture::ReleaseSurfaces()
{
	for (int i = 0; i < surfaces.numElem(); i++)
		surfaces[i]->Release();

	surfaces.clear();
}

LPDIRECT3DBASETEXTURE9 CD3D9Texture::GetCurrentTexture()
{
	if(m_nAnimatedTextureFrame > textures.numElem()-1)
		return NULL;

	return textures[m_nAnimatedTextureFrame];
}

// locks texture for modifications, etc
void CD3D9Texture::Lock(texlockdata_t* pLockData, Rectangle_t* pRect, bool bDiscard, bool bReadOnly, int nLevel, int nCubeFaceId)
{
	ASSERTMSG(!m_bIsLocked, "CD3D9Texture: already locked");
	
	if(m_bIsLocked)
		return;

	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	ASSERT( !m_bIsLocked );

	m_nLockLevel = nLevel;
	m_nLockCube = nCubeFaceId;
	m_bIsLocked = true;

	D3DLOCKED_RECT rect;

	RECT lock_rect;
	if(pRect)
	{
		lock_rect.top = pRect->vleftTop.y;
		lock_rect.left = pRect->vleftTop.x;
		lock_rect.right = pRect->vrightBottom.x;
		lock_rect.bottom = pRect->vrightBottom.y;
	}

	if(m_pool != D3DPOOL_DEFAULT)
		bDiscard = false;

	DWORD lock_flags = (bDiscard ? D3DLOCK_DISCARD : 0) | (bReadOnly ? D3DLOCK_READONLY : 0);

	// try lock surface if exist
	if( surfaces.numElem() )
	{
		ASSERT(nCubeFaceId < surfaces.numElem());

		if(m_pool != D3DPOOL_DEFAULT)
		{
			HRESULT r = ((LPDIRECT3DSURFACE9) surfaces[nCubeFaceId])->LockRect(&rect, (pRect ? &lock_rect : NULL), lock_flags);

			if(r == D3D_OK)
			{
				// set lock data params
				pLockData->pData = (ubyte*)rect.pBits;
				pLockData->nPitch = rect.Pitch;
			}
			else
			{
				ASSERT(!"Couldn't lock surface for texture!");
			}
		}
		else if(bReadOnly)
		{
			LPDIRECT3DDEVICE9 pDev = ((ShaderAPID3DX9*)g_pShaderAPI)->m_pD3DDevice;

			if (pDev->CreateOffscreenPlainSurface(m_iWidth, m_iHeight, formats[m_iFormat], D3DPOOL_SYSTEMMEM, &m_pLockSurface, NULL) == D3D_OK)
			{
				HRESULT r = ((ShaderAPID3DX9*)g_pShaderAPI)->m_pD3DDevice->GetRenderTargetData(((LPDIRECT3DSURFACE9) surfaces[nCubeFaceId]), m_pLockSurface);

				if(r != D3D_OK)
					ASSERT(!"Couldn't lock surface: failed to copy surface to m_pLockSurface!");

				r = m_pLockSurface->LockRect(&rect, (pRect ? &lock_rect : NULL), lock_flags);

				if(r == D3D_OK)
				{
					// set lock data params
					pLockData->pData = (ubyte*)rect.pBits;
					pLockData->nPitch = rect.Pitch;
				}
				else
				{
					ASSERT(!"Couldn't lock surface for texture!");
					m_pLockSurface->Release();
					m_pLockSurface = NULL;
				}
			}
			else
				ASSERT(!"Couldn't lock surface: CreateOffscreenPlainSurface fails!");
		}
		else
			ASSERT(!"Couldn't lock: Rendertargets are read-only!");
	}
	else // lock texture data
	{
		HRESULT r;

		if(m_iFlags & TEXFLAG_CUBEMAP)
		{
			r = ((LPDIRECT3DCUBETEXTURE9) textures[0])->LockRect((D3DCUBEMAP_FACES)m_nLockCube, 0, &rect, (pRect ? &lock_rect : NULL), lock_flags);
		}
		else
		{
			r = ((LPDIRECT3DTEXTURE9) textures[0])->LockRect(nLevel, &rect, (pRect ? &lock_rect : NULL), lock_flags);
		}

		if(r == D3D_OK)
		{
			// set lock data params
			pLockData->pData = (ubyte*)rect.pBits;
			pLockData->nPitch = rect.Pitch;
		}
		else
		{
			if(r == D3DERR_WASSTILLDRAWING)
				ASSERT(!"Please unbind lockable texture!");

			ASSERT(!"Couldn't lock texture!");
		}
	}
}
	
// unlocks texture for modifications, etc
void CD3D9Texture::Unlock()
{
	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	ASSERT( m_bIsLocked );

	if( surfaces.numElem() )
	{
		if(m_pool != D3DPOOL_DEFAULT)
		{
			((LPDIRECT3DSURFACE9) surfaces[0])->UnlockRect();
		}
		else
		{
			m_pLockSurface->UnlockRect();
			m_pLockSurface->Release();
			m_pLockSurface = NULL;
		}
	}
	else
	{
		if(m_iFlags & TEXFLAG_CUBEMAP)
			((LPDIRECT3DCUBETEXTURE9) textures[0])->UnlockRect( (D3DCUBEMAP_FACES)m_nLockCube, m_nLockLevel );
		else
			((LPDIRECT3DTEXTURE9) textures[0])->UnlockRect( m_nLockLevel );
	}
		

	m_bIsLocked = false;
}