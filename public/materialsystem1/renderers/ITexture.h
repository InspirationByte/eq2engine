//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Texture class interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef ITEXTURE_H
#define ITEXTURE_H

#include "ShaderAPI_defs.h"

#include "ds/refcounted.h"
#include "math/Rectangle.h"
#include "core/ppmem.h"

struct texlockdata_t
{
	ubyte*	pData;
	int		nPitch;
};

class ITexture : public RefCountedObject
{
public:
	// Name of texture
	virtual const char*					GetName() const = 0;
	virtual void						SetName(const char* pszNewName) = 0;

	// The dimensions of texture
	virtual int							GetWidth() const = 0;
	virtual int							GetHeight() const = 0;
	virtual int							GetMipCount() const = 0;

	// Set dimensions. Used only internally
	virtual void						SetDimensions(int width,int height) = 0;

	// Returns texture flags
	virtual int							GetFlags() const = 0;

	// Set flags
	virtual void						SetFlags(int iFlags) = 0;

	// Returns the format of texture
	virtual ETextureFormat				GetFormat() const = 0;

	// Set format of texture. Use only on render targets
	virtual void						SetFormat(ETextureFormat newformat) = 0;

	// returns sampler state of this texuture TODO: remove me
	virtual const SamplerStateParam_t&	GetSamplerState() const = 0;

	// sets texture sampler state TODO: remove me
	virtual void						SetSamplerState(const SamplerStateParam_t& newSamplerState) = 0;

	// Animated texture props (matsystem usage)
	virtual int							GetAnimatedTextureFrames() const = 0;

	// current frame (matsystem usage)
	virtual int							GetCurrentAnimatedTextureFrame() const = 0;

	// sets current animated texture frames (WARNING: more internal usage)
	virtual void						SetAnimatedTextureFrame(int frame) = 0;

	// locks texture for modifications, etc
	virtual void						Lock(texlockdata_t* pLockData, Rectangle_t* pRect = NULL, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0) = 0;

	// unlocks texture for modifications, etc
	virtual void						Unlock() = 0;
};

#endif
