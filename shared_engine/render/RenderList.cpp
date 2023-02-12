//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RenderList.h"
#include "BaseRenderableObject.h"
#include "ds/sort.h"

#define MIN_OBJECT_RENDERLIST_MEMSIZE 48

CRenderList::CRenderList() : m_ObjectList(PP_SL, MIN_OBJECT_RENDERLIST_MEMSIZE)
{

}

CRenderList::~CRenderList()
{
	
}

void CRenderList::AddRenderable(CBaseRenderableObject* pObject)
{
	m_ObjectList.append(pObject);
}

int CRenderList::GetRenderableCount()
{
	return m_ObjectList.numElem();
}

CBaseRenderableObject* CRenderList::GetRenderable(int id)
{
	return m_ObjectList[id];
}

void CRenderList::Append(CRenderList* pAnotherList)
{
	int num = pAnotherList->GetRenderableCount();

	for(int i = 0; i < num; i++)
		m_ObjectList.append(pAnotherList->GetRenderable(i));
}

void CRenderList::Render(int nViewRenderFlags, void* userdata)
{
	for(int i = 0; i < m_ObjectList.numElem(); i++)
		m_ObjectList[i]->Render(nViewRenderFlags, userdata);
}

void CRenderList::Remove(int id)
{
	m_ObjectList.removeIndex(id);
}

void CRenderList::Clear()
{
	m_ObjectList.clear(false);
}

void CRenderList::SortByDistanceFrom(const Vector3D& origin, bool reverse)
{
	// pre-compute object distances
	for(int i = 0; i < m_ObjectList.numElem(); ++i)
	{
		CBaseRenderableObject* renderable = m_ObjectList[i];
		const BoundingBox& bbox = renderable->GetBoundingBox();

		// clamp point in bbox
		if(!bbox.Contains(origin))
			renderable->m_viewDistance = lengthSqr(origin - bbox.ClampPoint(origin));
		else
			renderable->m_viewDistance = lengthSqr(origin - bbox.GetCenter());
	}

	if (reverse)
	{
		// furthest to closest (for transparency)
		shellSort(m_ObjectList, [](const CBaseRenderableObject* a, const CBaseRenderableObject* b) {
			return b->m_viewDistance - a->m_viewDistance;
		});
	}
	else
	{
		// closest to furthest
		shellSort(m_ObjectList, [](const CBaseRenderableObject* a, const CBaseRenderableObject* b) {
			return a->m_viewDistance - b->m_viewDistance;
		});
	}
}