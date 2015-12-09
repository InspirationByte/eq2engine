//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#include "CTexture.h"
#include "utils/strtools.h"
#include "DebugInterface.h"

#include "ishaderapi.h"

CTexture::CTexture()
{
	m_iFormat = FORMAT_NONE;
	m_iFlags = 0;
	m_iWidth = 0;
	m_iHeight = 0;

	// default frame is zero
	m_nAnimatedTextureFrame = 0;

	// can be one
	m_numAnimatedTextureFrames = 1;
}

CTexture::~CTexture()
{

}

const char* CTexture::GetName()
{
	return m_szTexName.GetData();
}

ETextureFormat CTexture::GetFormat()
{
	return m_iFormat;
}

int CTexture::GetWidth()
{
	return m_iWidth;
}

int CTexture::GetHeight()
{
	return m_iHeight;
}

int CTexture::GetFlags()
{
	return m_iFlags;
}

void CTexture::SetName(const char* pszNewName)
{
	m_szTexName = pszNewName;
}

// Animated texture props
int CTexture::GetAnimatedTextureFrames()
{
	return m_numAnimatedTextureFrames;
}

// current frame
int CTexture::GetCurrentAnimatedTextureFrame()
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

void CTexture::Ref_DeleteObject()
{
	// don't use before refactoring
	//g_pShaderAPI->FreeTexture(this);
}
