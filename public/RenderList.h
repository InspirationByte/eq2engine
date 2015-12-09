//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#ifndef RENDERLIST_H
#define RENDERLIST_H

#include "BaseRenderableObject.h"
#include "utils/DkList.h"

//----------------------------------------------------
// Render list interface.
//----------------------------------------------------
class IRenderList
{
public:
	virtual void						AddRenderable(CBaseRenderableObject* pObject) = 0;	// adds a single object

	virtual int							GetRenderableCount() = 0;							// returns count of renderables in this list
	virtual CBaseRenderableObject*		GetRenderable(int id) = 0;							// returns renderable pointer

	virtual void						CopyFromList(IRenderList* pAnotherList) = 0;		// copies objects from another render list. Call update if it's distance-sorted.

	virtual void						Update(int nViewRenderFlags, void* userdata) = 0;	// updates the render list.
	virtual void						Render(int nViewRenderFlags, void* userdata) = 0;	// draws render list, with applying own effets, etc.

	virtual void						Remove(int id) = 0;
	virtual void						Clear() = 0;										// clear it
};

//----------------------------------------------------
// Base render list interface.
//----------------------------------------------------
class CBasicRenderList : public IRenderList
{
public:
	void								AddRenderable(CBaseRenderableObject* pObject);	// adds a single object

	int									GetRenderableCount();							// returns count of renderables in this list
	CBaseRenderableObject*				GetRenderable(int id);							// returns renderable pointer

	void								CopyFromList(IRenderList* pAnotherList);		// copies objects from another render list. Call update if it's distance-sorted.

	void								Update(int nViewRenderFlags, void* userdata);		// updates the render list.
	void								Render(int nViewRenderFlags, void* userdata);		// draws render list

	void								SortByDistanceFrom(Vector3D& origin);

	void								Remove(int id);
	void								Clear();										// clear it
protected:

	DkList<CBaseRenderableObject*>		m_ObjectList;
};


#endif // RENDERLIST_H