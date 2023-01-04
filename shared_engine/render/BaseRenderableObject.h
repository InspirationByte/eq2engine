//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#pragma once

//------------------------------------------------
// A new inheritable renderable object
//------------------------------------------------

enum RenderableVisibilityState_e
{
	VISIBILITY_NOT_TESTED	= 0,
	VISIBILITY_VISIBLE,
	VISIBILITY_INVISIBLE,
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
		m_fViewDistance = 0.0f;
		//m_nVisibilityStatus = VISIBILITY_NOT_TESTED;
	}

	virtual ~CBaseRenderableObject() {}

	virtual void				Render(int nViewRenderFlags, void* userdata) = 0;

	virtual const BoundingBox&	GetBoundingBox() const = 0;

	virtual void				SetRenderFlags(int nFlags);
	virtual int					GetRenderFlags() const;

protected:

	// view distance (for sorting)
	float					m_fViewDistance;
	int						m_nRenderFlags;
};
