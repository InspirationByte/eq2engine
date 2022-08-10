//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RenderList.h"
#include "BaseRenderableObject.h"

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
	{
		m_ObjectList[i]->Render(nViewRenderFlags, userdata);
	}
}

void CRenderList::Remove(int id)
{
	m_ObjectList.removeIndex(id);
}

void CRenderList::Clear()
{
	m_ObjectList.clear(false);
}

int CRenderList::DistanceCompare(CBaseRenderableObject* const & a, CBaseRenderableObject* const& b)
{
	return b->m_fViewDistance - a->m_fViewDistance;
}

void CRenderList::SortByDistanceFrom(const Vector3D& origin)
{
	// compute object distances
	int num = m_ObjectList.numElem();
	for(int i = 0; i < num; i++)
	{
		CBaseRenderableObject* pRenderable = m_ObjectList[i];

		// FIXME: cache?
		const BoundingBox& bbox = pRenderable->GetBoundingBox();

		// clamp point in bbox
		if(!bbox.Contains(origin))
			pRenderable->m_fViewDistance = lengthSqr(origin - bbox.ClampPoint(origin));
		else
			pRenderable->m_fViewDistance = lengthSqr(origin - bbox.GetCenter());
	}

	m_ObjectList.sort(DistanceCompare);
}