//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/CTexture.h"

class CImage;

class CGLTexture : public CTexture
{
public:
	friend class			ShaderAPIGL;

	CGLTexture();
	~CGLTexture();

		// initializes texture from image array of images
	bool					Init(const SamplerStateParam_t& sampler, const ArrayCRef<CImage*> images, int flags = 0);

	// initializes render target texture
	bool					InitRenderTarget( const SamplerStateParam_t& sampler,
											ETextureFormat format,
											int width, int height,
											int flags = 0
											);

	void					Release();
	void					ReleaseTextures();

	GLTextureRef_t&			GetCurrentTexture();

	// locks texture for modifications, etc
	void					Lock(LockData* pLockData, Rectangle_t* pRect = nullptr, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);

	// unlocks texture for modifications, etc
	void					Unlock();

	Array<GLTextureRef_t>	textures{ PP_SL };

	float					m_flLod;
	GLuint					glTarget;
	GLuint					glDepthID;

	int						m_nLockLevel;
	int						m_lockCubeFace;

	bool					m_bIsLocked;
	int						m_texSize;

protected:
	ubyte*					m_lockPtr;
	int						m_lockOffs;
	int						m_lockSize;
	bool					m_lockDiscard;
	bool					m_lockReadOnly;
};

bool UpdateGLTextureFromImage(GLTextureRef_t texture, const CImage* image, int startMipLevel);
