//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 10 texture class
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D10TEXTURE_H
#define D3D10TEXTURE_H

#include "../Shared/CTexture.h"
#include "ds/Array.h"

#include <d3d10.h>
#include <d3dx10.h>

#define SAFE_RELEASE(p) { if (p){ p->Release(); p = nullptr; } }

class IRenderState;

class CD3D10Texture : public CTexture
{
public:
	friend class						ShaderAPID3DX10;

	CD3D10Texture();
	~CD3D10Texture();

	// locks texture for modifications, etc
	void								Lock(LockData* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);
	
	// unlocks texture for modifications, etc
	void								Unlock();

	void								Release();

// protected:

	Array<ID3D10ShaderResourceView*>	srv{ PP_SL };
	Array<ID3D10RenderTargetView*>		rtv{ PP_SL };
	Array<ID3D10DepthStencilView*>		dsv{ PP_SL };

	Array<ID3D10Resource*>				textures{ PP_SL };

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