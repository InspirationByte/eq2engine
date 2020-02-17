//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "CGLTexture.h"
#include "DebugInterface.h"
#include "imaging/ImageLoader.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#else
#include "glad.h"
#endif

#include "shaderapigl_def.h"

extern ShaderAPIGL g_shaderApi;

extern bool GLCheckError(const char* op);

CGLTexture::CGLTexture()
{
	m_flLod = 0.0f;
	glTarget = 0;
	glDepthID = 0;
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
	if (glTarget == GL_RENDERBUFFER)
	{
		glDeleteRenderbuffers(1, &glDepthID);
		GLCheckError("del tex renderbuffer");
	}
	else
	{
		for(int i = 0; i < textures.numElem(); i++)
		{
			glDeleteTextures(1, &textures[i].glTexID);
			GLCheckError("del tex");
		}
	}
}

GLTextureRef_t& CGLTexture::GetCurrentTexture()
{
	static GLTextureRef_t nulltex = {0, GLTEX_TYPE_ERROR};

	if(m_nAnimatedTextureFrame > textures.numElem()-1)
		return nulltex;

	return textures[m_nAnimatedTextureFrame];
}

// locks texture for modifications, etc
void CGLTexture::Lock(texlockdata_t* pLockData, Rectangle_t* pRect, bool bDiscard, bool bReadOnly, int nLevel, int nCubeFaceId)
{
	if(textures.numElem() > 1)
		ASSERT(!"Couldn't handle locking of animated texture! Please tell to programmer!");

	ASSERT( !m_bIsLocked );

	m_nLockLevel = nLevel;
	m_lockCubeFace = nCubeFaceId;
	m_bIsLocked = true;

	if( IsCompressedFormat(m_iFormat) )
	{
		pLockData->pData = NULL;
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
	m_lockPtr = (ubyte*)malloc(nLockByteCount);

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
		int targetOrCubeTarget = (glTarget == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X+m_lockCubeFace : glTarget;

        GLenum srcFormat = chanCountTypes[GetChannelCount(m_iFormat)];
        GLenum srcType = chanTypePerFormat[m_iFormat];

        glBindTexture(glTarget, textures[0].glTexID);

		glGetTexImage(targetOrCubeTarget, m_nLockLevel, srcFormat, srcType, m_lockPtr);

		GLCheckError("lock get tex image");

        glBindTexture(glTarget, 0);
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

			int targetOrCubeTarget = (glTarget == GL_TEXTURE_CUBE_MAP) ? GL_TEXTURE_CUBE_MAP_POSITIVE_X+m_lockCubeFace : glTarget;

			glBindTexture(glTarget, textures[0].glTexID);

			glTexSubImage2D(targetOrCubeTarget, m_nLockLevel, 0, 0, m_iWidth, m_iHeight, srcFormat, srcType, m_lockPtr);

			GLCheckError("unlock upload tex image");

			glBindTexture(glTarget, 0);
		}

		free(m_lockPtr);
		m_lockPtr = NULL;
	}
	else
		ASSERT(!"Texture is not locked!");

	m_bIsLocked = false;
}

bool UpdateGLTextureFromImage(GLTextureRef_t texture, CImage* image, int startMipLevel)
{
	const GLenum glTarget = glTexTargetType[texture.type];
	const ETextureFormat format = image->GetFormat();

	const GLenum srcFormat = chanCountTypes[GetChannelCount(format)];
	const GLenum srcType = chanTypePerFormat[format];

	GLint internalFormat = PickGLInternalFormat(format);

	glBindTexture(glTarget, texture.glTexID);
	GLCheckError("bind tex");

	// Upload it all
	ubyte *src;
	int mipMapLevel = startMipLevel;
	while ((src = image->GetPixels(mipMapLevel)) != NULL)
	{
		int size = image->GetMipMappedSize(mipMapLevel, 1);

		int lockBoxLevel = mipMapLevel - startMipLevel;

		if (texture.type == GLTEX_TYPE_VOLUMETEXTURE)
		{
			if (IsCompressedFormat(format))
			{
				glCompressedTexImage3D(glTarget,
					lockBoxLevel,
					internalFormat,
					image->GetWidth(mipMapLevel), image->GetHeight(mipMapLevel), image->GetDepth(mipMapLevel),
					0,
					image->GetMipMappedSize(mipMapLevel, 1),
					src);
			}
			else
			{
				glTexImage3D(glTarget,
					lockBoxLevel,
					internalFormat,
					image->GetWidth(mipMapLevel), image->GetHeight(mipMapLevel), image->GetDepth(mipMapLevel),
					0,
					srcFormat,
					srcType,
					src);
			}

			GLCheckError("tex upload 3d");
		}
		else if (texture.type == GLTEX_TYPE_CUBETEXTURE)
		{
			size /= 6;

			for (uint i = 0; i < 6; i++)
			{
				if (IsCompressedFormat(format))
				{
					glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						lockBoxLevel,
						internalFormat,
						image->GetWidth(mipMapLevel), image->GetHeight(mipMapLevel),
						0,
						size,
						src + i * size);
				}
				else
				{
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
						lockBoxLevel,
						internalFormat,
						image->GetWidth(mipMapLevel), image->GetHeight(mipMapLevel),
						0,
						srcFormat,
						srcType,
						src + i * size);
				}
				GLCheckError("tex upload cube");
			}
		}
		else if (texture.type == GLTEX_TYPE_TEXTURE)
		{
			if (IsCompressedFormat(format))
			{
				glCompressedTexImage2D(glTarget,
					lockBoxLevel,
					internalFormat,
					image->GetWidth(mipMapLevel), image->GetHeight(mipMapLevel),
					0,
					size,
					src);
			}
			else
			{
				glTexImage2D(glTarget,
					lockBoxLevel,
					internalFormat,
					image->GetWidth(mipMapLevel), image->GetHeight(mipMapLevel),
					0,
					srcFormat,
					srcType,
					src);
			}

			GLCheckError("tex upload 2d");
		}

		mipMapLevel++;
	}

	glBindTexture(glTarget, 0);
	GLCheckError("tex unbind");

	return true;
}