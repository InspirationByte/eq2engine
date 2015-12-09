//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#ifndef CBASERENDERABLE_H
#define CBASERENDERABLE_H

#include "math/DkMath.h"
#include "dktypes.h"		// ubyte

//------------------------------------------------
// A new inheritable renderable object
//------------------------------------------------

enum RenderableVisibilityState_e
{
	VISIBILITY_NOT_TESTED	= 0,
	VISIBILITY_VISIBLE,
	VISIBILITY_INVISIBLE,
};

enum RenderableFlags_e
{
	RF_NO_DRAW				= (1 << 1),		// doesn't draws the object

	RF_DISABLE_SHADOWS		= (1 << 2),		// disables shadow casting, but doesn't disables shadow receiving
	RF_ONLY_SHADOWS			= (1 << 3),		// only renders this object for shadow
	RF_SKIPVISIBILITYTEST	= (1 << 4),		// skips visibility test, and means that it's visible.
};

// renderable object
class CBaseRenderableObject
{
public:
//------------------------------------------------------------
// basic methods and helpers
//------------------------------------------------------------
	CBaseRenderableObject()
	{
		m_nRenderFlags = 0;
		//m_nVisibilityStatus = VISIBILITY_NOT_TESTED;
	}

	virtual ~CBaseRenderableObject() {}

	// draws transformed bbox for occlusion system
	//virtual void			RenderOcclusionTest(int nViewRenderFlags);

	// renders this object with current transformations
	virtual void			Render(int nViewRenderFlags) = 0;

	virtual Matrix4x4		GetRenderWorldTransform();

	// adds a render flags
	virtual void			AddRenderFlags(int nFlags);

	// removes render flags
	virtual void			RemoveRenderFlags(int nFlags);

	// returns render flags
	virtual int				GetRenderFlags();

	// min bbox dimensions
	virtual Vector3D		GetBBoxMins() = 0;

	// max bbox dimensions
	virtual Vector3D		GetBBoxMaxs() = 0;

	// resets visibility status
	//virtual void			ResetVisibilityState();

	// view distance (for sorting)
	float					m_fViewDistance;

protected:

	int						m_nRenderFlags;

	//ubyte					m_nVisibilityStatus;				// object visibility status
};

#endif //CBASERENDERABLE_H
