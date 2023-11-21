//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DakrTech scene renderer renderable
//////////////////////////////////////////////////////////////////////////////////

#pragma once
class CBaseRenderableObject;
class IGPURenderPassRecorder;

//----------------------------------------------------
// Base render list interface.
//----------------------------------------------------
class CRenderList
{
public:
	using Renderable = CBaseRenderableObject;

	CRenderList();
	virtual ~CRenderList();

	void				AddRenderable(Renderable* pObject);		// adds a single object

	void				Append(CRenderList* pAnotherList);		// copies objects from another render list. Call update if it's distance-sorted.

	int					GetRenderableCount();					// returns count of renderables in this list
	Renderable*			GetRenderable(int id);					// returns renderable pointer

	void				Render(int renderFlags, IGPURenderPassRecorder* rendPassRecorder, void* userdata);// draws render list

	void				SortByDistanceFrom(const Vector3D& origin, bool reverse);

	void				Clear();
protected:
	struct RendPair
	{
		float	distance;
		int		objIdx;
	};
	Array<Renderable*>	m_objectList;
	Array<RendPair>		m_viewDistance;
};
