//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	bool				InitProcedural(const TextureDesc& textureDesc);
	bool				GenerateErrorTexture(int flags = 0);

	void				SetName(const char* pszNewName);
	const char*			GetName() const { return m_name.ToCString(); }
	int					GetNameHash() const { return m_nameHash; }
	ETextureFormat		GetFormat() const { return m_format; }
	TextureExtent		GetSize() const { return { m_width, m_height, m_arraySize }; }

	int					GetWidth() const { return m_width; }
	int					GetHeight() const { return m_height; }
	int					GetArraySize() const { return m_arraySize; }

	int					GetMipCount() const { return m_mipCount; }
	int					GetSampleCount() const { return m_sampleCount; }

	int					GetFlags() const { return m_flags; }

	// FIXME: remove?
	const SamplerStateParams&	GetSamplerState() const {return m_samplerState;}

	// DEPRECATED
	int					GetAnimationFrameCount() const { return m_animFrameCount; }
	int					GetAnimationFrame() const { return m_animFrame; }
	void				SetAnimationFrame(int frame);

protected:
	void				SetFlags(int flags) { m_flags = flags; }
	void				SetDimensions(int width, int height, int arraySize = 1) { m_width = width; m_height = height; m_arraySize = arraySize; }
	void				SetMipCount(int count) { m_mipCount = count; }
	void				SetSampleCount(int count) { m_sampleCount = count; }
	void				SetFormat(ETextureFormat newformat) { m_format = newformat; }
	void				SetSamplerState(const SamplerStateParams& newSamplerState) { m_samplerState = newSamplerState; }


	EqString			m_name;
	int					m_nameHash{ 0 };

	ETextureFormat		m_format{ FORMAT_NONE };
	SamplerStateParams	m_samplerState;

	struct LodState
	{
		CImagePtr	image;
		int8		idx{ 0 };
		int8		lockBoxLevel{ 0 };
		int8		mipMapLevel{ 0 };
	};
	Array<LodState>		m_progressiveState{ PP_SL };

	LockInOutData*		m_lockData{ nullptr };

	int					m_flags{ 0 };
	ushort				m_width{ 0 };
	ushort				m_height{ 0 };
	ushort				m_arraySize{ 0 };
	ushort				m_mipCount{ 1 };
	ushort				m_sampleCount{ 1 };

	ushort				m_animFrame{ 0 };
	ushort				m_animFrameCount{ 1 };

	ushort				m_progressiveFrameDelay{ 1 };
};