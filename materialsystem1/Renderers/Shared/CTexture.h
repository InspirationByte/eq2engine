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

								CTexture();
								~CTexture();

	const char*					GetName() const;
	ETextureFormat				GetFormat() const;

	int							GetWidth() const;
	int							GetHeight() const;
	int							GetMipCount() const;

	int							GetFlags() const;
	const SamplerStateParam_t&	GetSamplerState() const {return m_samplerState;}
	int							GetAnimatedTextureFrames() const;
	int							GetCurrentAnimatedTextureFrame() const;

	void						SetName(const char* pszNewName);
	void						SetFlags(int iFlags) {m_iFlags = iFlags;}
	void						SetDimensions(int width,int height) {m_iWidth = width; m_iHeight = height;}
	void						SetMipCount(int count) {m_mipCount = count;}
	void						SetFormat(ETextureFormat newformat) {m_iFormat = newformat;}
	void						SetSamplerState(const SamplerStateParam_t& newSamplerState) {m_samplerState = newSamplerState;}
	void						SetAnimatedTextureFrame(int frame);		// sets current animated texture frames

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