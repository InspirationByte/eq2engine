//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#ifndef CTEXTURE_H
#define CTEXTURE_H

#include "renderers/ITexture.h"
#include "ds/eqstring.h"

class CTexture : public ITexture
{
	friend class ShaderAPI_Base;

public:
	const char*					GetName() const { return m_szTexName.ToCString(); }
	ETextureFormat				GetFormat() const { return m_iFormat; }

	int							GetWidth() const { return m_iWidth; }
	int							GetHeight() const { return m_iHeight; }
	int							GetMipCount() const { return m_mipCount; }

	int							GetFlags() const { return m_iFlags; }
	const SamplerStateParam_t&	GetSamplerState() const {return m_samplerState;}

	int							GetAnimationFrameCount() const { return m_numAnimatedTextureFrames; }
	int							GetAnimationFrame() const { return m_nAnimatedTextureFrame; }

	void						SetFlags(int iFlags) { m_iFlags = iFlags; }
	void						SetDimensions(int width,int height) { m_iWidth = width; m_iHeight = height; }
	void						SetMipCount(int count) { m_mipCount = count; }
	void						SetFormat(ETextureFormat newformat) { m_iFormat = newformat; }
	void						SetSamplerState(const SamplerStateParam_t& newSamplerState) { m_samplerState = newSamplerState; }

	void						SetName(const char* pszNewName);
	void						SetAnimationFrame(int frame);
protected:
	EqString				m_szTexName;
	int						m_nameHash{ 0 };

	ushort					m_iFlags{ 0 };
	ushort					m_iWidth{ 0 };
	ushort					m_iHeight{ 0 };
	ushort					m_mipCount{ 1 };

	ushort					m_nAnimatedTextureFrame{ 0 };
	ushort					m_numAnimatedTextureFrames{ 1 };

	ETextureFormat			m_iFormat{ FORMAT_NONE };

	SamplerStateParam_t		m_samplerState;
};

#endif //CTEXTURE_H