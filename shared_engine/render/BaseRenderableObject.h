//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CRenderList;
class IGPUCommandRecorder;
struct RenderPassContext;

enum RenderableVisibilityState_e
{
	VISIBILITY_NOT_TESTED	= 0,
	VISIBILITY_VISIBLE,
	VISIBILITY_INVISIBLE,
};

struct RenderInfo
{
	const RenderPassContext& passContext;
	void*	userData{ nullptr };
	float	distance{ 0.0f };		// dist from camera
	int		renderFlags{ 0 };
};

// renderable object
class CBaseRenderableObject
{
	friend class CRenderList;
public:

	virtual ~CBaseRenderableObject() = default;

	virtual void				Render(const RenderInfo& rinfo) = 0;

	virtual const BoundingBox&	GetBoundingBox() const = 0;

	virtual void				SetRenderFlags(int nFlags);
	virtual int					GetRenderFlags() const;

protected:
	virtual void				OnAddedToRenderList(CRenderList* list, void* userData) {}

	int							m_renderFlags{ 0 };
};
