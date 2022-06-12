//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Base texture class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "renderers/ShaderAPI_defs.h"
#include "CTexture.h"

void CTexture::SetName(const char* pszNewName)
{
	ASSERT_MSG(*pszNewName != 0, "Empty texture names are not allowed");
	m_szTexName = pszNewName;
	m_nameHash = StringToHash(m_szTexName.ToCString(), true);
}

// sets current animated texture frames
void CTexture::SetAnimationFrame(int frame)
{
	m_nAnimatedTextureFrame = clamp(frame, 0, m_numAnimatedTextureFrames-1);
}
