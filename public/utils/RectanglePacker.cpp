//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rectangle packer, for atlases
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "RectanglePacker.h"

struct PackerNode : PackerRectangle
{
	PackerNode(float x, float y, float w, float h)
	{
		PackerRectangle::x = x;
		PackerRectangle::y = y;
		PackerRectangle::width = w;
		PackerRectangle::height = h;
	}

	PackerNode*	left{ nullptr };
	PackerNode*	right{ nullptr };
};

using PackerNodePool = MemoryPool<PackerNode, 64>;

static bool PackerNodeAssignRectangle_r(PackerNode* node, PackerNodePool& nodePool, PackerRectangle& targetRect)
{
	if (node->left || node->right)
	{
		if (PackerNodeAssignRectangle_r(node->left, nodePool, targetRect))
			return true;

		return PackerNodeAssignRectangle_r(node->right, nodePool, targetRect);
	}

	if (targetRect.width <= node->width && targetRect.height <= node->height)
	{
		const float rx = node->x;
		const float ry = node->y;

		targetRect.x = rx;
		targetRect.y = ry;

		node->left = new(nodePool.allocate()) PackerNode(rx, ry + targetRect.height, targetRect.width, node->height - targetRect.height);
		node->right = new(nodePool.allocate()) PackerNode(rx + targetRect.width, ry, node->width - targetRect.width, node->height);
		return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------------------

CRectanglePacker::CRectanglePacker()
{
	m_padding = 0.0f;
}

CRectanglePacker::~CRectanglePacker()
{
}

int CRectanglePacker::AddRectangle(float width, float height, void* pUserData)
{
	const int rectIdx = m_rectList.numElem();

	PackerRectangle& rect = m_rectList.append();
	rect.width  = width + m_padding * 2.0f;
	rect.height = height + m_padding * 2.0f;
	m_rectUserData.append(pUserData);

	return rectIdx;
}

bool CRectanglePacker::AssignCoords(float& width, float& height, COMPRECTFUNC compRectFunc)
{
	// copy array and sort
	Array<int> sortedRects(PP_SL);
	sortedRects.reserve(m_rectList.numElem());
	for (int i = 0; i < m_rectList.numElem(); ++i)
		sortedRects.append(i);

	quickSort(sortedRects, [this, compRectFunc](const int ia, const int ib) {
		return compRectFunc(m_rectList[ia], m_rectList[ib]);
	});

	PackerNodePool nodePool(PP_SL);
	PackerNode* top = new(nodePool.allocate()) PackerNode(0, 0, width, height);

	width  = 0;
	height = 0;

	// do placement
	for (int rectIdx : sortedRects)
	{
		PackerRectangle& rect = m_rectList[rectIdx];
		if (PackerNodeAssignRectangle_r(top, nodePool, rect))
		{
			const float x = rect.x + rect.width;
			const float y = rect.y + rect.height;

			width = max(x, width);
			height = max(y, height);
		}
		else
			return false;
	}

	return true;
}

void CRectanglePacker::GetRectangle(AARectangle& rect, void** userData, int index) const
{
	const PackerRectangle& packRect = m_rectList[index];

	rect.leftTop = Vector2D(packRect.x + m_padding, packRect.y + m_padding);
	rect.rightBottom = rect.leftTop + Vector2D(packRect.width - m_padding*2.0f, packRect.height - m_padding*2.0f);
	
	if(userData)
		*userData = m_rectUserData[index];
}

void CRectanglePacker::Cleanup()
{
	m_rectList.clear(false);
	m_rectUserData.clear(false);
}