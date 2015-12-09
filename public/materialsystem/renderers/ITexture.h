//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef ITEXTURE_H
#define ITEXTURE_H

#include "textureformats.h"
#include "ShaderAPI_defs.h"
#include "refcounted.h"

struct texlockdata_t
{
	ubyte*	pData;
	int		nPitch;
};

class ITexture : public RefCountedObject
{
public:
	PPMEM_MANAGED_OBJECT();

	// Name of texture
	virtual const char*				GetName() = 0;
	virtual void					SetName(const char* pszNewName) = 0;

	// The dimensions of texture
	virtual int						GetWidth() = 0;
	virtual int						GetHeight() = 0;

	// Set dimensions. Used only internally
	virtual void					SetDimensions(int width,int height) = 0;

	// Returns texture flags
	virtual int						GetFlags() = 0;

	// Set flags
	virtual void					SetFlags(int iFlags) = 0;

	// Returns the format of texture
	virtual ETextureFormat			GetFormat() = 0;

	// Set format of texture. Use only on render targets
	virtual void					SetFormat(ETextureFormat newformat) = 0;

	// returns sampler state of this texuture TODO: remove me
	virtual SamplerStateParam_t&	GetSamplerState() = 0;

	// sets texture sampler state TODO: remove me
	virtual void					SetSamplerState(const SamplerStateParam_t& newSamplerState) = 0;

	// Animated texture props (matsystem usage)
	virtual int						GetAnimatedTextureFrames() = 0;

	// current frame (matsystem usage)
	virtual int						GetCurrentAnimatedTextureFrame() = 0;

	// sets current animated texture frames (WARNING: more internal usage)
	virtual void					SetAnimatedTextureFrame(int frame) = 0;

	// locks texture for modifications, etc
	virtual void					Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0) = 0;
	
	// unlocks texture for modifications, etc
	virtual void					Unlock() = 0;
};

#endif
