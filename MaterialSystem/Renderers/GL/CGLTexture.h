//***********Copyright (C) Two Dark Interactive Software 2007-2009************
//
// Description: Base OpenGL texture class
//
//****************************************************************************

#ifndef CGLTEXTURE_H
#define CGLTEXTURE_H

#include "../CTexture.h"
#include "utils/DkList.h"

#include <gl/gl.h>

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
	void					ReleaseSurfaces();

	eqGlTex					GetCurrentTexture();

	// locks texture for modifications, etc
	void					Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0);
	
	// unlocks texture for modifications, etc
	void					Unlock();

	bool					mipMapped;

	DkList<eqGlTex>			textures;

	float					m_flLod;
	GLuint					glTarget;
	GLuint					glDepthID;

	//bool					released;

	int						m_nLockLevel;
	bool					m_bIsLocked;

protected:
	ubyte*					m_lockPtr;
	int						m_lockOffs;
	int						m_lockSize;
	bool					m_lockDiscard;
	bool					m_lockReadOnly;
};

#endif //CGLTEXTURE_H