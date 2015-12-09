//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#ifndef CTEXTURE_H
#define CTEXTURE_H

#include "ITexture.h"
#include "utils/eqstring.h"

class CTexture : public ITexture
{
public:

							CTexture();
							~CTexture();

	const char*				GetName();
	ETextureFormat			GetFormat();

	int						GetWidth();
	int						GetHeight();

	//void					PrintInfo();

	int						GetFlags();
	SamplerStateParam_t&	GetSamplerState() {return m_samplerState;}

	void					SetName(const char* pszNewName);
	void					SetFlags(int iFlags) {m_iFlags = iFlags;}
	void					SetDimensions(int width,int height) {m_iWidth = width; m_iHeight = height;}
	void					SetFormat(ETextureFormat newformat) {m_iFormat = newformat;}
	void					SetSamplerState(const SamplerStateParam_t& newSamplerState) {m_samplerState = newSamplerState;}

	// Animated texture props
	int						GetAnimatedTextureFrames();

	// current frame
	int						GetCurrentAnimatedTextureFrame();

	// sets current animated texture frames
	void					SetAnimatedTextureFrame(int frame);

private:
	// refcounted
	void					Ref_DeleteObject();

protected:
	EqString				m_szTexName;

	ETextureFormat					m_iFormat;
	uint					m_iFlags;
	int						m_iWidth;
	int						m_iHeight;
	SamplerStateParam_t		m_samplerState;

	int						m_nAnimatedTextureFrame;
	int						m_numAnimatedTextureFrames;
};

#endif //CTEXTURE_H