//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#ifndef RENDERLIST_H
#define RENDERLIST_H

#include "BaseRenderableObject.h"
#include "ds/Array.h"

//----------------------------------------------------
// Base render list interface.
//----------------------------------------------------
class CRenderList
{
public:
	CRenderList();
	virtual ~CRenderList();

	void								AddRenderable(CBaseRenderableObject* pObject);		// adds a single object

	void								Append(CRenderList* pAnotherList);					// copies objects from another render list. Call update if it's distance-sorted.

	int									GetRenderableCount();								// returns count of renderables in this list
	CBaseRenderableObject*				GetRenderable(int id);								// returns renderable pointer

	void								Render(int nViewRenderFlags, void* userdata);		// draws render list

	void								SortByDistanceFrom(const Vector3D& origin);

	void								Remove(int id);
	void								Clear();										// clear it
protected:

	static int DistanceCompare(CBaseRenderableObject* const& a, CBaseRenderableObject* const& b);

	Array<CBaseRenderableObject*>		m_ObjectList;
};


#endif // RENDERLIST_H