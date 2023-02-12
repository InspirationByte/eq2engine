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
	virtual ~CBaseRenderableObject() {}

	virtual void				Render(int nViewRenderFlags, void* userdata) = 0;

	virtual const BoundingBox&	GetBoundingBox() const = 0;

	virtual void				SetRenderFlags(int nFlags);
	virtual int					GetRenderFlags() const;

protected:
	float						m_viewDistance{ 0.0f };
	int							m_renderFlags{ 0 };
};
