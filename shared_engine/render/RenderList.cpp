//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#include "core/DebugInterface.h"

#include "RenderList.h"
#include "math/BoundingBox.h"
#include "math/math_util.h"

#include <stdlib.h> // for qsort

#define MIN_OBJECT_RENDERLIST_MEMSIZE 128

void CBasicRenderList::AddRenderable(CBaseRenderableObject* pObject)
{
	m_ObjectList.append(pObject);
}

int CBasicRenderList::GetRenderableCount()
{
	return m_ObjectList.numElem();
}

CBaseRenderableObject* CBasicRenderList::GetRenderable(int id)
{
	return m_ObjectList[id];
}

void CBasicRenderList::CopyFromList(IRenderList* pAnotherList)
{
	for(int i = 0; i < pAnotherList->GetRenderableCount(); i++)
		m_ObjectList.append(pAnotherList->GetRenderable(i));
}

void CBasicRenderList::Update(int nViewRenderFlags, void* userdata)
{
	// do nothing in this context
}

void CBasicRenderList::Render(int nViewRenderFlags, void* userdata)
{
	for(int i = 0; i < m_ObjectList.numElem(); i++)
	{
		m_ObjectList[i]->Render(nViewRenderFlags);
	}
}

void CBasicRenderList::Remove(int id)
{
	m_ObjectList.removeIndex(id);
}

void CBasicRenderList::Clear()
{
	m_ObjectList.clear();
	m_ObjectList.resize(MIN_OBJECT_RENDERLIST_MEMSIZE);
}

static int DistanceCompare(const void* obj1, const void* obj2)
{
	CBaseRenderableObject* pObjectA = *(CBaseRenderableObject**)obj1;
	CBaseRenderableObject* pObjectB = *(CBaseRenderableObject**)obj2;

	return -UTIL_CompareFloats(pObjectA->m_fViewDistance, pObjectB->m_fViewDistance);
}

void CBasicRenderList::SortByDistanceFrom(const Vector3D& origin)
{
	//Msg("num objc: %d\n", m_ObjectList.numElem());

	for(int i = 0; i < m_ObjectList.numElem(); i++)
	{
		BoundingBox bbox(m_ObjectList[i]->GetBBoxMins(), m_ObjectList[i]->GetBBoxMaxs());

		Vector3D pos = origin;

		float dist_to_camera;

		// clamp point in bbox
		if(!bbox.Contains(pos))
		{
			pos = bbox.ClampPoint(pos);
			dist_to_camera = length(pos - origin);
		}
		else
		{
			dist_to_camera = length(pos - bbox.GetCenter());
		}

		m_ObjectList[i]->m_fViewDistance = dist_to_camera;
	}

	// sort list
	qsort(m_ObjectList.ptr(), m_ObjectList.numElem(), sizeof(CBaseRenderableObject*), DistanceCompare);
}