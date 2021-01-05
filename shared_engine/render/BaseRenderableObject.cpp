//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "BaseRenderableObject.h"
/*
// basic methods and helpers
//------------------------------------------------------------
// draws transformed bbox for occlusion system
void CBaseRenderableObject::RenderOcclusionTest(int nViewRenderFlags)
{
	ASSERT(!"CBaseRenderableObject::RenderOcclusionTest(): draw bbox here, and setup status of rendering\n");
}

*/

// returns world transformation of this object
Matrix4x4 CBaseRenderableObject::GetRenderWorldTransform()
{
	return identity4();
}

// adds a render flags
void CBaseRenderableObject::AddRenderFlags(int nFlags)
{
	m_nRenderFlags |= nFlags;
}

// returns render flags
int	CBaseRenderableObject::GetRenderFlags()
{
	return m_nRenderFlags;
}

// removes render flags
void CBaseRenderableObject::RemoveRenderFlags(int nFlags)
{
	m_nRenderFlags &= ~nFlags;
}
/*
// resets visibility status
void CBaseRenderableObject::ResetVisibilityState()
{
	m_nVisibilityStatus = VISIBILITY_NOT_TESTED;
}
*/