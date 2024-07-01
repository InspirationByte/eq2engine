//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RenderList.h"
#include "RenderableObject.h"

static constexpr const int MIN_OBJECT_RENDERLIST_SIZE = 64;

CRenderList::CRenderList()
	: m_objectList(PP_SL, MIN_OBJECT_RENDERLIST_SIZE)
	, m_viewDistance(PP_SL, MIN_OBJECT_RENDERLIST_SIZE)
{

}

void CRenderList::AddRenderable(Renderable* pObject, void* userData)
{
	if (!pObject)
		return;

	pObject->OnAddedToRenderList(this, userData);

	const int idx = m_objectList.append(pObject);
	m_viewDistance.append({ 0.0f, idx });
}

void CRenderList::Render(int renderFlags, const RenderPassContext& passContext, void* userdata)
{
	RenderInfo rinfo{ passContext, userdata, 0.0f, renderFlags };
	for (const RendPair& renderable : m_viewDistance)
	{
		rinfo.distance = renderable.distance;
		m_objectList[renderable.objIdx]->Render(rinfo);
	}
}

void CRenderList::Clear()
{
	m_objectList.clear(false);
	m_viewDistance.clear(false);
}

void CRenderList::SortByDistanceFrom(const Vector3D& origin, bool reverse)
{
	m_viewDistance.setNum(m_objectList.numElem(), false);

	// pre-compute object distances
	for(int i = 0; i < m_objectList.numElem(); ++i)
	{
		const Renderable* renderable = m_objectList[i];
		const BoundingBox& bbox = renderable->GetBoundingBox();

		// clamp point in bbox
		if(!bbox.Contains(origin))
			m_viewDistance[i].distance = length(origin - bbox.ClampPoint(origin));
		else
			m_viewDistance[i].distance = length(origin - bbox.GetCenter());
	}

	if (reverse)
	{
		// furthest to closest (for transparency)
		arraySort(m_viewDistance, [this](RendPair& a, RendPair& b) {
			return sortCompare(b.distance, a.distance);
		});
	}
	else
	{
		// closest to furthest
		arraySort(m_viewDistance, [this](RendPair& a, RendPair& b) {
			return sortCompare(a.distance, b.distance);
		});
	}
}