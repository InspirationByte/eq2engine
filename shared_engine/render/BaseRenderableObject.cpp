//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "math/math_common.h"
#include "BaseRenderableObject.h"

/*
// returns world transformation of this object
Matrix4x4 CBaseRenderableObject::GetRenderWorldTransform()
{
	return identity4();
}*/

// adds a render flags
void CBaseRenderableObject::SetRenderFlags(int nFlags)
{
	m_nRenderFlags = nFlags;
}

// removes render flags
int CBaseRenderableObject::GetRenderFlags() const
{
	return m_nRenderFlags;
}
