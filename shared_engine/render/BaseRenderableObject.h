//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASERENDERABLEOBJECT_H
#define BASERENDERABLEOBJECT_H

#include "math/DkMath.h"

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
	friend class CRenderList;
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

	// renders this object with current transformations
	virtual void			Render(int nViewRenderFlags, void* userdata) = 0;

	// min bbox dimensions
	virtual void			GetBoundingBox(BoundingBox& outBox) = 0;

	// adds a render flags
	virtual void			SetRenderFlags(int nFlags);

	// returns render flags
	virtual int				GetRenderFlags();

protected:

	// view distance (for sorting)
	float					m_fViewDistance;
	int						m_nRenderFlags;
};

#endif // BASERENDERABLEOBJECT_H
