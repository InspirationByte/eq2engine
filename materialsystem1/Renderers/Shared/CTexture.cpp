//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#include "CTexture.h"

#include "core/DebugInterface.h"
#include "utils/strtools.h"

CTexture::CTexture()
{
	m_iFormat = FORMAT_NONE;
	m_iFlags = 0;
	m_iWidth = 0;
	m_iHeight = 0;
	m_mipCount = 1;
	m_nameHash = 0;

	// default frame is zero
	m_nAnimatedTextureFrame = 0;

	// can be one
	m_numAnimatedTextureFrames = 1;
}

CTexture::~CTexture()
{

}

const char* CTexture::GetName() const
{
	return m_szTexName.GetData();
}

ETextureFormat CTexture::GetFormat() const
{
	return m_iFormat;
}

int CTexture::GetWidth() const
{
	return m_iWidth;
}

int CTexture::GetHeight() const
{
	return m_iHeight;
}

int	CTexture::GetMipCount() const
{
	return m_mipCount;
}

int CTexture::GetFlags() const
{
	return m_iFlags;
}

void CTexture::SetName(const char* pszNewName)
{
	m_szTexName = pszNewName;
	m_nameHash = StringToHash(m_szTexName.ToCString(), true);
}

// Animated texture props
int CTexture::GetAnimatedTextureFrames() const
{
	return m_numAnimatedTextureFrames;
}

// current frame
int CTexture::GetCurrentAnimatedTextureFrame() const
{
	return m_nAnimatedTextureFrame;
}

// sets current animated texture frames
void CTexture::SetAnimatedTextureFrame(int frame)
{
	if(frame >= m_numAnimatedTextureFrames)
		return;

	m_nAnimatedTextureFrame = frame;
}
