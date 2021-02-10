//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#ifndef CTEXTURE_H
#define CTEXTURE_H

#include "renderers/ITexture.h"
#include "utils/eqstring.h"

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

private:
	// refcounted
	void					Ref_DeleteObject();

protected:
	EqString				m_szTexName;
	int						m_nameHash;

	ushort					m_iFlags;
	ushort					m_iWidth;
	ushort					m_iHeight;
	ushort					m_mipCount;

	ushort					m_nAnimatedTextureFrame;
	ushort					m_numAnimatedTextureFrames;

	ETextureFormat			m_iFormat;

	SamplerStateParam_t		m_samplerState;
};

#endif //CTEXTURE_H