//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "core/DebugInterface.h"

#include "RenderList.h"
#include "math/BoundingBox.h"
#include "math/Utility.h"

#include <stdlib.h> // for qsort

#define MIN_OBJECT_RENDERLIST_MEMSIZE 48

CRenderList::CRenderList() : m_ObjectList(MIN_OBJECT_RENDERLIST_MEMSIZE)
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

// compares floats (for array sort)
inline int DistComparator(float f1, float f2)
{
	if (f1 < f2)
		return -1;
	else if (f1 > f2)
		return 1;

	return 0;
}

int CRenderList::DistanceCompare(CBaseRenderableObject* const & a, CBaseRenderableObject* const& b)
{
	return DistComparator(a->m_fViewDistance, b->m_fViewDistance);
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