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

struct RenderInfo
{
	const RenderPassContext& passContext;
	void*	userData{ nullptr };
	float	distance{ 0.0f };		// dist from camera
	int		renderFlags{ 0 };
};

// renderable object
class IRenderableObject
{
	friend class CRenderList;
public:
	virtual ~IRenderableObject() = default;

	virtual void				Render(const RenderInfo& rinfo) = 0;
	virtual const BoundingBox&	GetBoundingBox() const = 0;

	void						SetRenderFlags(int flags);
	int							GetRenderFlags() const;

protected:
	virtual void				OnAddedToRenderList(CRenderList* list, void* userData) {}

	int							m_renderFlags{ 0 };
};
