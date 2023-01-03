//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/CTexture.h"

typedef struct SamplerStateParam_s SamplerStateParam_t;
class CImage;

struct GLTextureRef_t
{
	GLuint		glTexID;
	EImageType	type;
};

class CGLTexture : public CTexture
{
public:
	friend class			ShaderAPIGL;

	CGLTexture();
	~CGLTexture();

		// initializes texture from image array of images
	bool					Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	void					Release();
	void					ReleaseTextures();

	GLTextureRef_t&			GetCurrentTexture();

	// locks texture for modifications, etc
	bool					Lock(LockInOutData& data);

	// unlocks texture for modifications, etc
	void					Unlock();

	Array<GLTextureRef_t>	textures{ PP_SL };

	float					m_flLod{ 0.0f };
	GLuint					m_glTarget{ GL_NONE };
	GLuint					m_glDepthID{ GL_NONE };

	int						m_texSize{ 0 };

protected:

	GLTextureRef_t			CreateGLTexture(EImageType type, ETextureFormat format, int mipCount, int widthMip0, int heightMip0, int depthMip0) const;

	int						m_lockOffs{ 0 };
	int						m_lockSize{ 0 };
};

void SetupGLSamplerState(uint texTarget, const SamplerStateParam_t& sampler, int mipMapCount = 1);
