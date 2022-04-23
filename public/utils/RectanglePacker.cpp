//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rectangle packer, for atlases
//////////////////////////////////////////////////////////////////////////////////

#include "RectanglePacker.h"
#include "core/DebugInterface.h"

struct PackerNode 
{
	PackerNode(float x, float y, float w, float h, void* userdata)
	{
		left = right = NULL;

		rect = new PackerRectangle;
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
	if (rect == NULL)
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

			left  = new PackerNode(rx, ry + newRect->height, newRect->width, rect->height - newRect->height, newRect->userdata);
			right = new PackerNode(rx + newRect->width, ry, rect->width - newRect->width, rect->height, newRect->userdata);

			delete rect;
			rect = NULL;

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
	PackerRectangle* rect = new PackerRectangle;

	rect->width  = width+m_padding*2.0f;
	rect->height = height+m_padding*2.0f;

	rect->userdata = pUserData;

	return m_pRectangles.append(rect);
}

bool CRectanglePacker::AssignCoords(float& width, float& height, COMPRECTFUNC compRectFunc)
{
	// copy array and sort
	Array<PackerRectangle*> sortedRects;
	sortedRects.append(m_pRectangles);

	sortedRects.sort(compRectFunc);

	PackerNode* top = new PackerNode(0, 0, width, height, NULL);

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

void CRectanglePacker::GetRectangle(Rectangle_t& rect, void** userData, uint index) const
{
	PackerRectangle* pc = m_pRectangles[index];

	rect.vleftTop = Vector2D(pc->x + m_padding, pc->y + m_padding);
	rect.vrightBottom = rect.vleftTop + Vector2D(pc->width - m_padding*2.0f, pc->height - m_padding*2.0f);
	
	if(userData != NULL)
		*userData = pc->userdata;
}

void CRectanglePacker::Cleanup()
{
	for(int i = 0; i < m_pRectangles.numElem(); i++)
		delete m_pRectangles[i];

	m_pRectangles.clear(false);
}