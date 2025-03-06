//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct RenderPassContext;
class IRenderableObject;

class CRenderList
{
public:
	using Renderable = IRenderableObject;

	CRenderList();
	virtual ~CRenderList() = default;

	void					Clear();

	void					AddRenderable(Renderable* renderable, void* userData = nullptr);		// adds a single object
	ArrayCRef<Renderable*>	GetRenderables() const { return m_objectList; }
	void					SortByDistanceFrom(const Vector3D& origin, bool reverse);

	void					Render(int renderFlags, const RenderPassContext& passContext, void* userdata = nullptr);// draws render list

protected:
	struct RendPair
	{
		float	distance;
		int		objIdx;
	};
	Array<Renderable*>	m_objectList;
	Array<RendPair>		m_viewDistance;
};
