//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#pragma once
class CBaseRenderableObject;

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

	void								SortByDistanceFrom(const Vector3D& origin, bool reverse);

	void								Remove(int id);
	void								Clear();										// clear it
protected:
	Array<CBaseRenderableObject*>		m_ObjectList;
};
