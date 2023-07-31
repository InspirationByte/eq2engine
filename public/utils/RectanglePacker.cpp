//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rectangle packer, for atlases
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RectanglePacker.h"

struct PackerNode 
{
	PackerNode(float x, float y, float w, float h, void* userdata)
	{
		left = right = nullptr;

		rect = PPNew PackerRectangle;
		rect->x = x;
		rect->y = y;
		rect->width = w;
		rect->height = h;
		rect->userdata = userdata;
	}

	~PackerNode()
	{
		delete left;

		delete right;

		if(rect)
			delete rect;
	}

	bool AssignRectangle_r(PackerRectangle *rect);

	PackerNode*		left;
	PackerNode*		right;

	PackerRectangle*	rect;
};

bool PackerNode::AssignRectangle_r(PackerRectangle *newRect)
{
	if (rect == nullptr)
	{
		if (left->AssignRectangle_r(newRect))
			return true;

		return right->AssignRectangle_r(newRect);
	}
	else 
	{
		if (newRect->width <= rect->width && newRect->height <= rect->height)
		{
			float rx = rect->x;
			float ry = rect->y;

			newRect->x = rx;
			newRect->y = ry;

			left = PPNew PackerNode(rx, ry + newRect->height, newRect->width, rect->height - newRect->height, newRect->userdata);
			right = PPNew PackerNode(rx + newRect->width, ry, rect->width - newRect->width, rect->height, newRect->userdata);

			delete rect;
			rect = nullptr;

			return true;
		}

		return false;
	}
}

//----------------------------------------------------------------------------------------------------------------------------

CRectanglePacker::CRectanglePacker()
{
	m_padding = 0.0f;
}

CRectanglePacker::~CRectanglePacker()
{
	for (int i = 0; i < m_pRectangles.numElem(); i++)
	{
		delete m_pRectangles[i];
	}
}

int CRectanglePacker::AddRectangle(float width, float height, void* pUserData)
{
	PackerRectangle* rect = PPNew PackerRectangle;

	rect->width  = width+m_padding*2.0f;
	rect->height = height+m_padding*2.0f;

	rect->userdata = pUserData;

	return m_pRectangles.append(rect);
}

bool CRectanglePacker::AssignCoords(float& width, float& height, COMPRECTFUNC compRectFunc)
{
	// copy array and sort
	Array<PackerRectangle*> sortedRects(PP_SL);
	sortedRects.append(m_pRectangles);

	quickSort(sortedRects, compRectFunc);

	PackerNode* top = PPNew PackerNode(0, 0, width, height, nullptr);

	width  = 0;
	height = 0;

	// do placement
	for (int i = 0; i < sortedRects.numElem(); i++)
	{
		if (top->AssignRectangle_r( sortedRects[i] ))
		{
			float x = sortedRects[i]->x + sortedRects[i]->width;
			float y = sortedRects[i]->y + sortedRects[i]->height;

			if (x > width)
				width = x;

			if (y > height)
				height = y;
		}
		else
		{
			delete top;
			return false;
		}
	}

	delete top;

	return true;
}

void CRectanglePacker::GetRectangle(AARectangle& rect, void** userData, uint index) const
{
	PackerRectangle* pc = m_pRectangles[index];

	rect.vleftTop = Vector2D(pc->x + m_padding, pc->y + m_padding);
	rect.vrightBottom = rect.vleftTop + Vector2D(pc->width - m_padding*2.0f, pc->height - m_padding*2.0f);
	
	if(userData != nullptr)
		*userData = pc->userdata;
}

void CRectanglePacker::Cleanup()
{
	for(int i = 0; i < m_pRectangles.numElem(); i++)
		delete m_pRectangles[i];

	m_pRectangles.clear(false);
}