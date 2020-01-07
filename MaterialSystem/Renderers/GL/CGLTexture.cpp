//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "CGLTexture.h"
#include "DebugInterface.h"

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
        g_shaderApi.GL_CRITICAL();

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

            g_shaderApi.GL_CRITICAL();

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
