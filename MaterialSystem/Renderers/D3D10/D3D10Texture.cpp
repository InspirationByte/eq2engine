//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 10 texture class
//////////////////////////////////////////////////////////////////////////////////

#include "D3D10Texture.h"
#include "DebugInterface.h"

CD3D10Texture::CD3D10Texture()  : CTexture()
{
	//srvArray = NULL;
	//rtvArray = NULL;
	//dsvArray = NULL;

	m_pD3D10SamplerState = NULL;

	arraySize = 1;
	depth = 1;
}

void CD3D10Texture::Release()
{
	for(int i = 0; i < srv.numElem(); i++)
		SAFE_RELEASE(srv[i])

	srv.clear();

	for(int i = 0; i < rtv.numElem(); i++)
		SAFE_RELEASE(rtv[i])

	rtv.clear();

	for(int i = 0; i < dsv.numElem(); i++)
		SAFE_RELEASE(dsv[i])

	dsv.clear();

	for(int i = 0; i < textures.numElem(); i++)
		SAFE_RELEASE(textures[i]);

	textures.clear();

	/*
	int sliceCount = (depth == 1)? arraySize : depth;

	if (srvArray)
	{
		for (int i = 0; i < sliceCount; i++)
			srvArray[i]->Release();

		delete [] srvArray;
		srvArray = NULL;
	}

	if (rtvArray)
	{
		for (int i = 0; i < sliceCount; i++)
			rtvArray[i]->Release();

		delete [] rtvArray;
		rtvArray = NULL;
	}

	if (dsvArray)
	{
		for (int i = 0; i < sliceCount; i++)
			dsvArray[i]->Release();

		delete [] dsvArray;
		dsvArray = NULL;
	}
	*/
	arraySize = 0;
	depth = 0;
}

CD3D10Texture::~CD3D10Texture()
{
	Release();
}

void CD3D10Texture::Lock(texlockdata_t* pLockData, Rectangle_t* pRect, bool bDiscard, bool bReadOnly, int nLevel, int nCubeFaceId)
{/*
	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	m_nLockLevel = nLevel;
	m_bIsLocked = true;

	ASSERT( !m_bIsLocked );

	D3DLOCKED_RECT rect;

	RECT lock_rect;
	if(pRect)
	{
		lock_rect.top = pRect->vleftTop.y;
		lock_rect.left = pRect->vleftTop.x;
		lock_rect.right = pRect->vrightBottom.x;
		lock_rect.bottom = pRect->vrightBottom.y;
	}

	DWORD lock_flags = (bDiscard ? D3DLOCK_DISCARD : 0) | (bReadOnly ? D3DLOCK_READONLY : 0);

	if(((LPDIRECT3DTEXTURE9) textures[0])->LockRect(nLevel, &rect, (pRect ? &lock_rect : NULL), lock_flags) == D3D_OK)
	{
		// set lock data params
		pLockData->pData = (ubyte*)rect.pBits;
		pLockData->nPitch = rect.Pitch;
	}
	else
		ASSERT(!"Couldn't lock texture!");
		*/
}
	
// unlocks texture for modifications, etc
void CD3D10Texture::Unlock()
{
	/*
	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	ASSERT( m_bIsLocked );

	((LPDIRECT3DTEXTURE9) textures[0])->UnlockRect( m_nLockLevel );

	m_bIsLocked = false;
	*/
}