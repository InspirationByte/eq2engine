//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 9 texture class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"

#include "imaging/ImageLoader.h"
#include "shaderapid3d9_def.h"
#include "D3D9Texture.h"

#include "ShaderAPID3D9.h"

extern ShaderAPID3DX9 s_shaderApi;

CD3D9Texture::CD3D9Texture() : CTexture()
{
	m_nLockLevel = 0;
	m_bIsLocked = false;
	m_pLockSurface = nullptr;
	m_texSize = 0;
	m_dummyDepth = nullptr;
}

CD3D9Texture::~CD3D9Texture()
{
	Release();
}

void CD3D9Texture::Release()
{
	ASSERT_MSG(!m_bIsLocked, "texture was locked");

	ReleaseTextures();
	ReleaseSurfaces();
}

IDirect3DBaseTexture9* CD3D9Texture::CreateD3DTexture(EImageType type, ETextureFormat format, int mipCount, int widthMip0, int heightMip0, int depthMip0) const
{
	IDirect3DDevice9* d3dDevice = s_shaderApi.GetD3DDevice();
	IDirect3DBaseTexture9* d3dTexture = nullptr;

	if (IsCompressedFormat(format))
	{
		//FIXME: is that even valid?
		widthMip0 &= ~3;
		heightMip0 &= ~3;
	}

	if (type == IMAGE_TYPE_CUBE)
	{
		HRESULT status = d3dDevice->CreateCubeTexture(widthMip0, mipCount, 0,
														formats[format], (D3DPOOL)m_pool, (LPDIRECT3DCUBETEXTURE9*)&d3dTexture, nullptr);
		if (status != D3D_OK)
		{
			return nullptr;
		}
	} 
	else if (type == IMAGE_TYPE_3D)
	{
		HRESULT status = d3dDevice->CreateVolumeTexture(widthMip0, heightMip0, depthMip0, mipCount, 0,
														formats[format], (D3DPOOL)m_pool, (LPDIRECT3DVOLUMETEXTURE9*)&d3dTexture, nullptr);
		if (status != D3D_OK)
		{
			return nullptr;
		}
	} 
	else if(type == IMAGE_TYPE_2D || type == IMAGE_TYPE_1D)
	{
		HRESULT status = d3dDevice->CreateTexture(widthMip0, heightMip0, mipCount, 0,
													formats[format], (D3DPOOL)m_pool, (LPDIRECT3DTEXTURE9*)&d3dTexture, nullptr);
		if (status != D3D_OK)
		{
			return nullptr;
		}
	}
	else
	{
		ASSERT_FAIL("Invalid texture type!");
	}

	return d3dTexture;
}

static bool UpdateD3DTextureFromImage(IDirect3DBaseTexture9* texture, const CImage* image, int startMipLevel, bool convert)
{
	const EImageType imgType = image->GetImageType();
	const bool isAcceptableImageType =
		(texture->GetType() == D3DRTYPE_VOLUMETEXTURE && imgType == IMAGE_TYPE_3D) ||
		(texture->GetType() == D3DRTYPE_CUBETEXTURE && imgType == IMAGE_TYPE_CUBE) ||
		(texture->GetType() == D3DRTYPE_TEXTURE && (imgType == IMAGE_TYPE_2D || imgType == IMAGE_TYPE_1D));

	ASSERT_MSG(isAcceptableImageType, "UpdateD3DTextureFromImage - image type to texture mismatch");

	if (!isAcceptableImageType)
		return false;

	const ETextureFormat nFormat = image->GetFormat();

	if (convert)
	{
		CImage* conv = const_cast<CImage*>(image);

		if (nFormat == FORMAT_RGB8 || nFormat == FORMAT_RGBA8)
			conv->SwapChannels(0, 2); // convert to BGR

		// Convert if needed and upload datas
		if (nFormat == FORMAT_RGB8) // as the D3DFMT_X8R8G8B8 used
			conv->Convert(FORMAT_RGBA8);
	}

	const DWORD lockFlags = D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK;

	const int numMipMaps = image->GetMipMapCount();
	int mipMapLevel = numMipMaps-1;

	while (mipMapLevel >= startMipLevel)
	{
		ubyte* src = image->GetPixels(mipMapLevel);
		ASSERT(src);
		const int size = image->GetMipMappedSize(mipMapLevel, 1);
		const int lockBoxLevel = mipMapLevel - startMipLevel;

		switch (texture->GetType())
		{
			case D3DRTYPE_VOLUMETEXTURE:
			{
				IDirect3DVolumeTexture9* texture3D = (IDirect3DVolumeTexture9*)texture;
				D3DLOCKED_BOX box;
				if (texture3D->LockBox(lockBoxLevel, &box, nullptr, lockFlags) == D3D_OK)
				{
					memcpy(box.pBits, src, size);
					texture3D->UnlockBox(lockBoxLevel);
				}
				break;
			}
			case D3DRTYPE_CUBETEXTURE:
			{
				IDirect3DCubeTexture9* cubeTexture = (IDirect3DCubeTexture9*)texture;
				const int cubeFaceSize = size / 6;

				D3DLOCKED_RECT rect;
				for (int i = 0; i < 6; i++)
				{
					if (cubeTexture->LockRect((D3DCUBEMAP_FACES)i, lockBoxLevel, &rect, nullptr, lockFlags) == D3D_OK)
					{
						memcpy(rect.pBits, src, cubeFaceSize);
						cubeTexture->UnlockRect((D3DCUBEMAP_FACES)i, lockBoxLevel);
					}
					src += cubeFaceSize;
				}
				break;
			}
			case D3DRTYPE_TEXTURE:
			{
				IDirect3DTexture9* texture2D = (IDirect3DTexture9*)texture;
				D3DLOCKED_RECT rect;

				if (texture2D->LockRect(lockBoxLevel, &rect, nullptr, lockFlags) == D3D_OK)
				{
					memcpy(rect.pBits, src, size);
					texture2D->UnlockRect(lockBoxLevel);
				}
				break;
			}
			default:
			{
				ASSERT_FAIL("Invalid resource type passed to UpdateD3DTextureFromImage");
			}
		}

		texture->SetLOD(lockBoxLevel);
		--mipMapLevel;
	}
	
	return true;
}

// initializes texture from image array of images
bool CD3D9Texture::Init(const SamplerStateParam_t& sampler, const ArrayCRef<CImage*> images, int flags)
{
	// FIXME: only release if pool, flags, format and size is different
	Release();

	m_samplerState = sampler;
	m_iFlags = flags;
	m_pool = D3DPOOL_MANAGED;

	HOOK_TO_CVAR(r_loadmiplevel);

	for (int i = 0; i < images.numElem(); i++)
	{
		if (images[i]->IsCube())
			m_iFlags |= TEXFLAG_CUBEMAP;
	}

	m_iFlags |= TEXFLAG_MANAGED;

	const int quality = (m_iFlags & TEXFLAG_NOQUALITYLOD) ? 0 : r_loadmiplevel->GetInt();

	for (int i = 0; i < images.numElem(); i++)
	{
		const CImage* img = images[i];

		if ((m_iFlags & TEXFLAG_CUBEMAP) && !img->IsCube())
		{
			CrashMsg("TEXFLAG_CUBEMAP set - every texture in set must be cubemap, %s is not a cubemap\n", m_szTexName.ToCString());
		}

		const EImageType imgType = img->GetImageType();
		const ETextureFormat imgFmt = img->GetFormat();
		const int imgMipCount = img->GetMipMapCount();
		const bool imgHasMipMaps = (imgMipCount > 1);

		const int mipStart = imgHasMipMaps ? min(quality, imgMipCount - 1) : 0;
		const int mipCount = max(imgMipCount - quality, 0);

		const int texWidth = img->GetWidth(mipStart);
		const int texHeight = img->GetHeight(mipStart);
		const int texDepth = img->GetDepth(mipStart);

		IDirect3DBaseTexture9* d3dTexture = CreateD3DTexture(imgType, imgFmt, mipCount, texWidth, texHeight, texDepth );

		if (!d3dTexture)
		{
			MsgError("D3D9 ERROR: failed to create texture for image %s\n", img->GetName());
			continue;
		}

		UpdateD3DTextureFromImage(d3dTexture, img, mipStart, true);

		d3dTexture->PreLoad();

		// FIXME: check for differences?
		m_mipCount = max(m_mipCount, mipCount);
		m_iWidth = max(m_iWidth, texWidth);
		m_iHeight = max(m_iHeight, texHeight);
		m_iFormat = imgFmt;

		m_texSize += img->GetMipMappedSize(mipStart);
		textures.append(d3dTexture);
	}

	m_numAnimatedTextureFrames = textures.numElem();

	return true;
}

void CD3D9Texture::ReleaseTextures()
{
	for (int i = 0; i < textures.numElem(); i++)
		textures[i]->Release();

	textures.clear();
	m_texSize = 0;
}

void CD3D9Texture::ReleaseSurfaces()
{
	for (int i = 0; i < surfaces.numElem(); i++)
		surfaces[i]->Release();

	surfaces.clear();

	if (m_dummyDepth)
		m_dummyDepth->Release();
	m_dummyDepth = nullptr;
}

LPDIRECT3DBASETEXTURE9 CD3D9Texture::GetCurrentTexture()
{
	if (!textures.inRange(m_nAnimatedTextureFrame))
		return nullptr;

	return textures[m_nAnimatedTextureFrame];
}

// locks texture for modifications, etc
void CD3D9Texture::Lock(LockData* pLockData, Rectangle_t* pRect, bool bDiscard, bool bReadOnly, int nLevel, int nCubeFaceId)
{
	ASSERT_MSG(!m_bIsLocked, "CD3D9Texture: already locked");
	
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

	// FIXME: LOck just like in the CreateD3DTextureFromImage

	// try lock surface if exist
	if( surfaces.numElem() )
	{
		ASSERT(nCubeFaceId < surfaces.numElem());

		if(m_pool != D3DPOOL_DEFAULT)
		{
			HRESULT r = surfaces[nCubeFaceId]->LockRect(&rect, (pRect ? &lock_rect : nullptr), lock_flags);

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
			IDirect3DDevice9* d3dDevice = s_shaderApi.GetD3DDevice();

			if (d3dDevice->CreateOffscreenPlainSurface(m_iWidth, m_iHeight, formats[m_iFormat], D3DPOOL_SYSTEMMEM, &m_pLockSurface, nullptr) == D3D_OK)
			{
				HRESULT r = d3dDevice->GetRenderTargetData(surfaces[nCubeFaceId], m_pLockSurface);

				if(r != D3D_OK)
					ASSERT(!"Couldn't lock surface: failed to copy surface to m_pLockSurface!");

				r = m_pLockSurface->LockRect(&rect, (pRect ? &lock_rect : nullptr), lock_flags);

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
					m_pLockSurface = nullptr;
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
			r = ((IDirect3DCubeTexture9*) textures[0])->LockRect((D3DCUBEMAP_FACES)m_nLockCube, 0, &rect, (pRect ? &lock_rect : nullptr), lock_flags);
		}
		else
		{
			r = ((IDirect3DTexture9*) textures[0])->LockRect(nLevel, &rect, (pRect ? &lock_rect : nullptr), lock_flags);
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
			surfaces[0]->UnlockRect();
		}
		else
		{
			m_pLockSurface->UnlockRect();
			m_pLockSurface->Release();
			m_pLockSurface = nullptr;
		}
	}
	else
	{
		if(m_iFlags & TEXFLAG_CUBEMAP)
			((IDirect3DCubeTexture9*) textures[0])->UnlockRect( (D3DCUBEMAP_FACES)m_nLockCube, m_nLockLevel );
		else
			((IDirect3DTexture9*) textures[0])->UnlockRect( m_nLockLevel );
	}
		

	m_bIsLocked = false;
}