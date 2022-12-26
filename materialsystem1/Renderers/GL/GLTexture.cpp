//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"

#include "shaderapigl_def.h"
#include "GLTexture.h"
#include "GLWorker.h"
#include "imaging/ImageLoader.h"

static ConVar gl_skipTextures("gl_skipTextures", "0", nullptr, CV_CHEAT);

extern bool GLCheckError(const char* op, ...);

CGLTexture::CGLTexture()
{
	m_flLod = 0.0f;
	m_glTarget = GL_NONE;
	m_glDepthID = GL_NONE;
	m_texSize = 0;
	m_nLockLevel = 0;
	m_bIsLocked = false;

	m_lockPtr = 0;
	m_lockOffs = 0;
	m_lockSize = 0;
	m_lockDiscard = false;
	m_lockReadOnly = false;
}

CGLTexture::~CGLTexture()
{
	Release();
}

void CGLTexture::Release()
{
	ReleaseTextures();
}

void CGLTexture::ReleaseTextures()
{
	if (m_glTarget == GL_RENDERBUFFER)
	{
		glDeleteRenderbuffers(1, &m_glDepthID);
		GLCheckError("del tex renderbuffer");
	}
	else
	{
		for(int i = 0; i < textures.numElem(); i++)
		{
			glDeleteTextures(1, &textures[i].glTexID);
			GLCheckError("del tex");
		}

		if (m_glDepthID != GL_NONE)
		{
			glDeleteTextures(1, &m_glDepthID);
			GLCheckError("del depth");
		}
	}
}

void SetupGLSamplerState(uint texTarget, const SamplerStateParam_t& sampler, int mipMapCount)
{
	// Set requested wrapping modes
	glTexParameteri(texTarget, GL_TEXTURE_WRAP_S, addressModes[sampler.wrapS]);
	GLCheckError("smp w s");

#ifndef USE_GLES2
	if (texTarget != GL_TEXTURE_1D)
#endif // USE_GLES2
	{
		glTexParameteri(texTarget, GL_TEXTURE_WRAP_T, addressModes[sampler.wrapT]);
		GLCheckError("smp w t");
	}

	if (texTarget == GL_TEXTURE_3D)
	{
		glTexParameteri(texTarget, GL_TEXTURE_WRAP_R, addressModes[sampler.wrapR]);
		GLCheckError("smp w r");
	}

	// Set requested filter modes
	glTexParameteri(texTarget, GL_TEXTURE_MAG_FILTER, minFilters[sampler.magFilter]);
	GLCheckError("smp mag");

	glTexParameteri(texTarget, GL_TEXTURE_MIN_FILTER, minFilters[sampler.minFilter]);
	GLCheckError("smp min");

	glTexParameteri(texTarget, GL_TEXTURE_MAX_LEVEL, max(mipMapCount-1, 0));
	GLCheckError("smp mip");

	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	GLCheckError("smp cmpmode");

	glTexParameteri(texTarget, GL_TEXTURE_COMPARE_FUNC, depthConst[sampler.compareFunc]);
	GLCheckError("smp cmpfunc");

#ifndef USE_GLES2
	// Setup anisotropic filtering
	if (sampler.aniso > 1 && GLAD_GL_ARB_texture_filter_anisotropic)
	{
		glTexParameteri(texTarget, GL_TEXTURE_MAX_ANISOTROPY, sampler.aniso);
		GLCheckError("smp aniso");
	}
#endif // USE_GLES2
}

bool UpdateGLTextureFromImage(GLTextureRef_t texture, SamplerStateParam_t& sampler, const CImage* image, int startMipLevel)
{
	const GLenum glTarget = glTexTargetType[texture.type];
	const ETextureFormat format = image->GetFormat();

	const GLenum srcFormat = chanCountTypes[GetChannelCount(format)];
	const GLenum srcType = chanTypePerFormat[format];

	const GLenum internalFormat = PickGLInternalFormat(format);

	glBindTexture(glTarget, texture.glTexID);
	GLCheckError("bind tex");

	// Upload it all
	ubyte *src;
	int mipMapLevel = startMipLevel;
	while ((src = image->GetPixels(mipMapLevel)) != nullptr)
	{
		const int size = image->GetMipMappedSize(mipMapLevel, 1);
		const int lockBoxLevel = mipMapLevel - startMipLevel;
		int width = image->GetWidth(mipMapLevel);
		int height = image->GetHeight(mipMapLevel);

		if (IsCompressedFormat(format))
		{
			//FIXME: is that even valid?
			width &= ~3;
			height &= ~3;
		}

		if (texture.type == IMAGE_TYPE_3D)
		{
			if (IsCompressedFormat(format))
			{
				glCompressedTexImage3D(glTarget, lockBoxLevel, internalFormat,
					width, height, image->GetDepth(mipMapLevel), 0, size, src);
			}
			else
			{
				glTexImage3D(glTarget,lockBoxLevel, internalFormat,
					width, height, image->GetDepth(mipMapLevel), 0, srcFormat, srcType, src);
			}
			if (!GLCheckError("tex upload 3d (mip %d)", mipMapLevel))
				break;
		}
		else if (texture.type == IMAGE_TYPE_CUBE)
		{
			const int cubeFaceSize = size / 6;

			for (uint i = 0; i < 6; i++)
			{
				if (IsCompressedFormat(format))
				{
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, lockBoxLevel, internalFormat,
						width, height, 0, cubeFaceSize, src + i * cubeFaceSize);
				}
				else
				{
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, lockBoxLevel, internalFormat,
						width, height, 0, srcFormat, srcType, src + i * cubeFaceSize);
				}
				if (!GLCheckError("tex upload cube (mip %d)", mipMapLevel))
					break;
			}
		}
		else if (texture.type == IMAGE_TYPE_1D || IMAGE_TYPE_2D)
		{
			if (IsCompressedFormat(format))
			{
				glCompressedTexImage2D(glTarget, lockBoxLevel, internalFormat,
					width, height, 0, size, src);
			}
			else
			{
				glTexImage2D(glTarget, lockBoxLevel, internalFormat,
					width, height, 0, srcFormat, srcType, src);
			}

			if (!GLCheckError("tex upload 2d (mip %d)", mipMapLevel))
				break;
		}

		mipMapLevel++;
	}

	SetupGLSamplerState(glTarget, sampler, mipMapLevel);

	glBindTexture(glTarget, 0);
	GLCheckError("tex unbind");

	return true;
}

// initializes texture from image array of images
bool CGLTexture::Init(const SamplerStateParam_t& sampler, const ArrayCRef<CImage*> images, int flags)
{
	static GLTextureRef_t invalidTexture = { 0, IMAGE_TYPE_INVALID };

	// FIXME: only release if pool, flags, format and size is different
	Release();

	m_samplerState = sampler;
	m_iFlags = flags;

	HOOK_TO_CVAR(r_loadmiplevel);

	for (int i = 0; i < images.numElem(); i++)
	{
		if (images[i]->IsCube())
			m_iFlags |= TEXFLAG_CUBEMAP;
	}

	m_iFlags |= TEXFLAG_MANAGED;
	m_glTarget = glTexTargetType[images[0]->GetImageType()];

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

		GLTextureRef_t texture;
		texture.type = imgType;

		if (gl_skipTextures.GetBool())
			textures.append(invalidTexture);

		const int result = g_glWorker.WaitForExecute(__FUNCTION__, [this, &texture, img, mipStart]()
		{
			// Generate a texture
			glGenTextures(1, &texture.glTexID);

			if (!GLCheckError("gen tex"))
				return -1;

			if (!UpdateGLTextureFromImage(texture, m_samplerState, img, mipStart))
			{
				glBindTexture(m_glTarget, 0);
				GLCheckError("tex unbind");

				glDeleteTextures(1, &texture.glTexID);
				GLCheckError("del tex");

				return -1;
			}
			return 0;
		});

		if (result == -1)
			continue;

		// FIXME: check for differences?
		m_mipCount = max(m_mipCount, mipCount);
		m_iWidth = max(m_iWidth, texWidth);
		m_iHeight = max(m_iHeight, texHeight);
		m_iFormat = imgFmt;

		m_texSize += img->GetMipMappedSize(mipStart);
		textures.append(texture);
	}

	m_numAnimatedTextureFrames = textures.numElem();

	return true;
}

GLTextureRef_t& CGLTexture::GetCurrentTexture()
{
	static GLTextureRef_t nulltex = {0, IMAGE_TYPE_INVALID};

	if(!textures.inRange(m_nAnimatedTextureFrame))
		return nulltex;

	return textures[m_nAnimatedTextureFrame];
}

// locks texture for modifications, etc
void CGLTexture::Lock(LockData* pLockData, Rectangle_t* pRect, bool bDiscard, bool bReadOnly, int nLevel, int nCubeFaceId)
{
	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	ASSERT( !m_bIsLocked );

	m_nLockLevel = nLevel;
	m_lockCubeFace = nCubeFaceId;
	m_bIsLocked = true;

	if( IsCompressedFormat(m_iFormat) )
	{
		pLockData->pData = nullptr;
		pLockData->nPitch = 0;
		return;
	}

	int lockOffset = 0;
	int sizeToLock = GetWidth()*GetHeight();

	IRectangle lock_rect;
	if(pRect)
	{
		lock_rect.vleftTop = pRect->vleftTop;
		lock_rect.vrightBottom = pRect->vrightBottom;

		sizeToLock = lock_rect.GetSize().x*lock_rect.GetSize().y;
		lockOffset = lock_rect.vleftTop.x*lock_rect.vleftTop.y;
	}
	else
	{
		lock_rect.vleftTop.x = 0;
		lock_rect.vleftTop.y = 0;

		lock_rect.vrightBottom.x = GetWidth();
		lock_rect.vrightBottom.y = GetHeight();
	}

	int nLockByteCount = GetBytesPerPixel(m_iFormat) * sizeToLock;

	// allocate memory for lock data
	m_lockPtr = (ubyte*)PPAlloc(nLockByteCount);

	pLockData->pData = m_lockPtr;
	pLockData->nPitch = lock_rect.GetSize().y;

	m_lockSize = sizeToLock;
	m_lockOffs = lockOffset;
	m_lockReadOnly = bReadOnly;

#ifdef USE_GLES2
	// Always need to discard data from GLES :(
#else
    if(!bDiscard)
    {
		int targetOrCubeTarget = (m_glTarget == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X+m_lockCubeFace : m_glTarget;

        GLenum srcFormat = chanCountTypes[GetChannelCount(m_iFormat)];
        GLenum srcType = chanTypePerFormat[m_iFormat];

        glBindTexture(m_glTarget, textures[0].glTexID);

		glGetTexImage(targetOrCubeTarget, m_nLockLevel, srcFormat, srcType, m_lockPtr);

		GLCheckError("lock get tex image");

        glBindTexture(m_glTarget, 0);
    }
#endif // USE_GLES2
}

// unlocks texture for modifications, etc
void CGLTexture::Unlock()
{
	if(m_bIsLocked)
	{
		if( !m_lockReadOnly )
		{
			GLenum srcFormat = chanCountTypes[GetChannelCount(m_iFormat)];
			GLenum srcType = chanTypePerFormat[m_iFormat];
			GLenum internalFormat = PickGLInternalFormat(m_iFormat);

			int targetOrCubeTarget = (m_glTarget == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X+m_lockCubeFace : m_glTarget;

			glBindTexture(m_glTarget, textures[0].glTexID);
			GLCheckError("bind texture");

			glTexSubImage2D(targetOrCubeTarget, m_nLockLevel, 0, 0, m_iWidth, m_iHeight, srcFormat, srcType, m_lockPtr);
			GLCheckError("unlock upload tex image");

			glBindTexture(m_glTarget, 0);
		}

		PPFree(m_lockPtr);
		m_lockPtr = nullptr;
	}
	else
		ASSERT(!"Texture is not locked!");

	m_bIsLocked = false;
}
