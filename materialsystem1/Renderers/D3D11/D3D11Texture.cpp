//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Direct3D 10 texture class
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>

#include "core/core_common.h"
#include "D3D11Texture.h"
#include "ShaderAPID3D11.h"

extern ShaderAPID3DX10 s_shaderApi;

CD3D10Texture::~CD3D10Texture()
{
	Release();
}

void CD3D10Texture::Ref_DeleteObject()
{
	s_shaderApi.FreeTexture(this);
	RefCountedObject::Ref_DeleteObject();
}

void CD3D10Texture::Release()
{
	for(int i = 0; i < m_srv.numElem(); i++)
		m_srv[i]->Release();
	m_srv.clear();

	for(int i = 0; i < m_rtv.numElem(); i++)
		m_rtv[i]->Release();
	m_rtv.clear();

	for(int i = 0; i < m_dsv.numElem(); i++)
		m_dsv[i]->Release();
	m_dsv.clear();

	for (int i = 0; i < m_textures.numElem(); i++)
		m_textures[i]->Release();
	m_textures.clear();

	m_arraySize = 0;
	m_depth = 0;
}

// initializes texture from image array of images
bool CD3D10Texture::Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags)
{
	ASSERT_FAIL("Unimplemented");

	return false;
}

bool CD3D10Texture::Lock(LockInOutData& data)
{
	ASSERT_FAIL("Unimplemented");

	return false;
}
	
// unlocks texture for modifications, etc
void CD3D10Texture::Unlock()
{

}