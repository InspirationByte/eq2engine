//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/CTexture.h"

struct SamplerStateParams;
class CImage;

struct GLTextureRef_t
{
	GLuint		glTexID{ 0 };
	EImageType	type{ IMAGE_TYPE_INVALID };
};

class CGLTexture : public CTexture
{
public:
	friend class			ShaderAPIGL;

	CGLTexture();
	~CGLTexture();

	// initializes texture from image array of images
	bool					Init(const SamplerStateParams& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	void					Release();
	void					ReleaseTextures();

	GLTextureRef_t&			GetCurrentTexture();

	// locks texture for modifications, etc
	bool					Lock(LockInOutData& data);

	// unlocks texture for modifications, etc
	void					Unlock();

	EProgressiveStatus		StepProgressiveLod();

protected:
	Array<GLTextureRef_t>	m_textures{ PP_SL };

	float					m_flLod{ 0.0f };
	GLuint					m_glTarget{ GL_NONE };
	GLuint					m_glDepthID{ GL_NONE };

	int						m_texSize{ 0 };
	void					Ref_DeleteObject();

	GLTextureRef_t			CreateGLTexture(const CImage* img, const SamplerStateParams& sampler, int startMip, int mipCount) const;

};

void SetupGLSamplerState(uint texTarget, const SamplerStateParams& sampler, int mipMapCount = 1);
