//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Dijkstra Graph Interface/wrapper class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ds/SimpleBinaryHeap.h"

template<typename NODE_ID>
class IGraphEdgeIterator
{
public:
	// requires constructor(IGraphIterator<EDGE_ITER, NODE_ID>* );
	virtual ~IGraphEdgeIterator() {}

	virtual void	operator++(int) = 0;

	virtual void	Rewind(NODE_ID nodeId) { m_nodeId = nodeId; }

	virtual bool	IsEdgeValid() const = 0;
	virtual bool	AtEnd() const = 0;
	virtual int		GetEdgeId() const = 0;
	int				GetNodeId() const { return m_nodeId; }

protected:
	NODE_ID			m_nodeId;
};

template<typename EDGE_ITER, typename NODE_ID>
class IGraph
{
public:
	using CheckNodeFunc = EqFunction<bool(NODE_ID node, float distance)>;

	virtual ~IGraph() {}

	NODE_ID				Djikstra(ArrayCRef<NODE_ID> startNodes, const CheckNodeFunc& isStopNodeFunc);

protected:
	virtual void		ResetNodeStates() = 0;

	virtual bool		Node_IsProcessed(NODE_ID nodeId) const = 0;
	virtual void		Node_MarkProcessed(NODE_ID nodeId) = 0;

	virtual float		Node_GetDistance(NODE_ID nodeId) const = 0;
	virtual void		Node_SetDistance(NODE_ID nodeId, float newDist) = 0;

	virtual NODE_ID		Node_GetParent(NODE_ID nodeId) const = 0;
	virtual void		Node_SetParent(NODE_ID nodeId, NODE_ID newParent) = 0;

	virtual float		Edge_GetLength(const EDGE_ITER& edgeIt) const = 0;
	virtual NODE_ID		Edge_GetNeighbourNode(const EDGE_ITER& edgeIt) const = 0;
};

//--------------------------------------------------------------------

template<typename NODE_ID>
struct GraphNode
{
	NODE_ID nodeId;
	float dist;

	static int Compare(const GraphNode& nodeA, const GraphNode& nodeB)
	{
		return nodeA.dist > nodeB.dist;
	}
};

template<typename EDGE_ITER, typename NODE_ID>
inline NODE_ID IGraph<EDGE_ITER, NODE_ID>::Djikstra(ArrayCRef<NODE_ID> startNodes, const CheckNodeFunc& isStopNodeFunc)
{
	// this should reset parent nodes ids
	ResetNodeStates();

	// make initial front which consists of node id and total distance
	SimpleBinaryHeap<GraphNode<NODE_ID>> openSet(PP_SL);
	for (NODE_ID nodeId: startNodes)
	{
		Node_SetDistance(nodeId, 0.0f);
		openSet.Add({ nodeId, 0.0f });
	}

	NODE_ID cheapestNode = -1;
	EDGE_ITER edgeIt(this);
	while (openSet.HasItems())
	{
		GraphNode<NODE_ID> bestNode = openSet.PopMin();

		cheapestNode = bestNode.nodeId;
		const float minDist = bestNode.dist;

		Node_MarkProcessed(cheapestNode);

		// walk through edges and neighbour nodes
		for (edgeIt.Rewind(cheapestNode); !edgeIt.AtEnd(); edgeIt++)
		{
			const NODE_ID neighbourNode = Edge_GetNeighbourNode(edgeIt);

			if (Node_IsProcessed(neighbourNode))
				continue;

			const float edgeLen = Edge_GetLength(edgeIt);
			const float distance = minDist + edgeLen;
			if (distance < Node_GetDistance(neighbourNode))
			{
				Node_SetDistance(neighbourNode, distance);
				Node_SetParent(neighbourNode, cheapestNode);
				openSet.Add({ neighbourNode, distance });
			}
		}

		if (isStopNodeFunc(cheapestNode, minDist))
			break;
	}

	return cheapestNode;
}