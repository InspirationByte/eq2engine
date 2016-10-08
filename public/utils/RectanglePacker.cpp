//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rectangle packer, for atlases
//////////////////////////////////////////////////////////////////////////////////

#include "RectanglePacker.h"
#include "DebugInterface.h"

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

	bool AssignRectangle_r(PackerRectangle *rect, float padding);

	PackerNode*		left;
	PackerNode*		right;

	PackerRectangle*	rect;
};

bool PackerNode::AssignRectangle_r(PackerRectangle *newRect, float padding)
{
	if (rect == NULL)
	{
		if (left->AssignRectangle_r(newRect, padding))
			return true;

		return right->AssignRectangle_r(newRect, padding);
	}
	else 
	{
		if (newRect->width <= rect->width+padding && newRect->height <= rect->height+padding)
		{
			float rx = rect->x;
			float ry = rect->y;

			newRect->x = rx + padding;
			newRect->y = ry + padding;

			// because we need it on every side
			float dpadding = padding*2.0f;

			left  = new PackerNode(rx, ry + newRect->height + dpadding, newRect->width + dpadding, rect->height - newRect->height + dpadding, newRect->userdata);
			right = new PackerNode(rx + newRect->width + dpadding, ry, rect->width - newRect->width + dpadding, rect->height + dpadding, newRect->userdata);

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

	rect->width  = width;
	rect->height = height;
	rect->userdata = pUserData;

	return m_pRectangles.append(rect);
}

bool CRectanglePacker::AssignCoords(float& width, float& height, COMPRECTFUNC compRectFunc)
{
	// copy array and sort
	DkList<PackerRectangle*> sortedRects;
	sortedRects.append(m_pRectangles);

	sortedRects.sort(compRectFunc);

	PackerNode* top = new PackerNode(0, 0, width, height, NULL);

	width  = 0;
	height = 0;

	// do placement
	for (int i = 0; i < sortedRects.numElem(); i++)
	{
		if (top->AssignRectangle_r( sortedRects[i], m_padding ))
		{
			float x = sortedRects[i]->x + sortedRects[i]->width + m_padding;
			float y = sortedRects[i]->y + sortedRects[i]->height + m_padding;

			if (x > width )
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

void CRectanglePacker::Cleanup()
{
	for(int i = 0; i < m_pRectangles.numElem(); i++)
		delete m_pRectangles[i];

	m_pRectangles.clear();
}