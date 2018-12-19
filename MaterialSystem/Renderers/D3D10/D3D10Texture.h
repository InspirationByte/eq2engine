//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 10 texture class
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D10TEXTURE_H
#define D3D10TEXTURE_H

#include "../Shared/CTexture.h"
#include "utils/DkList.h"

#include <d3d10.h>
#include <d3dx10.h>

#define SAFE_RELEASE(p) { if (p){ p->Release(); p = NULL; } }

class IRenderState;

class CD3D10Texture : public CTexture
{
public:
	friend class						ShaderAPID3DX10;

	CD3D10Texture();
	~CD3D10Texture();

	// locks texture for modifications, etc
	void								Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);
	
	// unlocks texture for modifications, etc
	void								Unlock();

	void								Release();

// protected:

	DkList<ID3D10ShaderResourceView*>	srv;
	DkList<ID3D10RenderTargetView*>		rtv;
	DkList<ID3D10DepthStencilView*>		dsv;

	DkList<ID3D10Resource*>				textures;

	IRenderState*						m_pD3D10SamplerState;

	/*
	ID3D10ShaderResourceView**			srvArray;
	ID3D10RenderTargetView**			rtvArray;
	ID3D10DepthStencilView**			dsvArray;
	*/
	DXGI_FORMAT							texFormat;
	DXGI_FORMAT							srvFormat;
	DXGI_FORMAT							rtvFormat;
	DXGI_FORMAT							dsvFormat;

	int									arraySize;
	int									depth;
};

#endif //D3D10TEXTURE_H