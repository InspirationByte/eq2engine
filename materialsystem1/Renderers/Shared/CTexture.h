//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ITexture.h"
#include "renderers/ShaderAPI_defs.h"

static constexpr const int TEXTURE_TRANSFER_RATE_THRESHOLD = 512;
static constexpr const int TEXTURE_TRANSFER_MAX_TEXTURES_PER_FRAME = 15;

enum EProgressiveStatus : int
{
	PROGRESSIVE_STATUS_COMPLETED = 0,
	PROGRESSIVE_STATUS_WAIT_MORE_FRAMES,
	PROGRESSIVE_STATUS_DID_UPLOAD,
};

class CTexture : public ITexture
{
	friend class ShaderAPI_Base;

public:
	bool						InitProcedural(const SamplerStateParam_t& sampler, ETextureFormat format, int width, int height, int depth = 1, int arraySize = 1, int flags = 0);
	
	// generates a new error texture
	bool						GenerateErrorTexture(int flags = 0);

	void						SetName(const char* pszNewName);
	const char*					GetName() const { return m_szTexName.ToCString(); }
	int							GetNameHash() const { return m_nameHash; }
	ETextureFormat				GetFormat() const { return m_iFormat; }

	int							GetWidth() const { return m_iWidth; }
	int							GetHeight() const { return m_iHeight; }
	int							GetDepth() const { return m_iDepth; }

	int							GetMipCount() const { return m_mipCount; }

	void						SetFlags(int iFlags) { m_iFlags = iFlags; }
	int							GetFlags() const { return m_iFlags; }

	const SamplerStateParam_t&	GetSamplerState() const {return m_samplerState;}

	int							GetAnimationFrameCount() const { return m_numAnimatedTextureFrames; }
	int							GetAnimationFrame() const { return m_nAnimatedTextureFrame; }
	void						SetAnimationFrame(int frame);

	void						SetDimensions(int width, int height, int depth = 1) { m_iWidth = width; m_iHeight = height; m_iDepth = depth; }

	void						SetMipCount(int count) { m_mipCount = count; }
	void						SetFormat(ETextureFormat newformat) { m_iFormat = newformat; }
	void						SetSamplerState(const SamplerStateParam_t& newSamplerState) { m_samplerState = newSamplerState; }

protected:
	EqString				m_szTexName;
	int						m_nameHash{ 0 };

	struct LodState
	{
		CRefPtr<CImage> image;
		int8			idx{ 0 };
		int8			lockBoxLevel{ 0 };
		int8			mipMapLevel{ 0 };
		uint8			frameDelay{ 1 };
	};

	Array<LodState>			m_progressiveState{ PP_SL };

	LockInOutData*			m_lockData{ nullptr };

	ushort					m_iFlags{ 0 };
	ushort					m_iWidth{ 0 };
	ushort					m_iHeight{ 0 };
	ushort					m_iDepth{ 0 };
	ushort					m_mipCount{ 1 };

	ushort					m_nAnimatedTextureFrame{ 0 };
	ushort					m_numAnimatedTextureFrames{ 1 };

	ETextureFormat			m_iFormat{ FORMAT_NONE };

	SamplerStateParam_t		m_samplerState;
};