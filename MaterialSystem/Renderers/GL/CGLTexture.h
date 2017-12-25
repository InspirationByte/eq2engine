//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLTEXTURE_H
#define CGLTEXTURE_H

#include "../CTexture.h"
#include "utils/DkList.h"

#include "ShaderAPIGL.h"

struct eqGlTex
{
	GLuint glTexID;
};

class CGLTexture : public CTexture
{
public:
	friend class			ShaderAPIGL;

	CGLTexture();
	~CGLTexture();

	void					Release();
	void					ReleaseTextures();

	eqGlTex&				GetCurrentTexture();

	// locks texture for modifications, etc
	void					Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);

	// unlocks texture for modifications, etc
	void					Unlock();

	DkList<eqGlTex>			textures;

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

#endif //CGLTEXTURE_H
