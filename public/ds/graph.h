//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Dijkstra Graph Interface/wrapper class
//////////////////////////////////////////////////////////////////////////////////

#pragma once

template<typename NODE_ID>
class IGraphEdgeIterator
{
public:
	// requires constructor(IGraphIterator<EDGE_ITER, NODE_ID>* );
	virtual ~IGraphEdgeIterator() {}

	virtual void	Rewind(NODE_ID node) = 0;
	virtual void	operator++(int) = 0;

	virtual bool	IsEdgeValid() const = 0;
	virtual bool	HasMoreNodes() const = 0;
	virtual int		GetEdgeId() const = 0;
};

template<typename EDGE_ITER, typename NODE_ID>
class IGraph
{
public:
	using CheckNodeFunc = EqFunction<bool(NODE_ID node, float distance)>;

	virtual ~IGraph() {}

	NODE_ID				Djikstra(const NODE_ID* startNodes, int startNodeCount, const CheckNodeFunc& isStopNodeFunc);

protected:
	virtual void		ResetNodeStates() = 0;

	virtual bool		Node_IsProcessed(NODE_ID nodeId) const = 0;
	virtual void		Node_MarkProcessed(NODE_ID nodeId) = 0;

	virtual float		Node_GetDistance(NODE_ID nodeId) const = 0;
	virtual void		Node_SetDistance(NODE_ID nodeId, float newDist) = 0;

	virtual NODE_ID		Node_GetParent(NODE_ID nodeId) const = 0;
	virtual void		Node_SetParent(NODE_ID nodeId, NODE_ID newParent) = 0;

	virtual float		Edge_GetLength(NODE_ID nodeId, int edgeId) const = 0;
	virtual NODE_ID		Edge_GetNeighbourNode(NODE_ID nodeId, int edgeId) const = 0;
};

//--------------------------------------------------------------------

template<typename EDGE_ITER, typename NODE_ID>
inline NODE_ID IGraph<EDGE_ITER, NODE_ID>::Djikstra(const NODE_ID* startNodes, int startNodeCount, const CheckNodeFunc& isStopNodeFunc)
{
	// this should reset parent nodes ids
	ResetNodeStates();

	// make initial front which consists of node id and total distance
	Map<NODE_ID, float> openSet(PP_SL);
	for (int i = 0; i < startNodeCount; ++i)
	{
		Node_SetDistance(startNodes[i], 0.0f);
		openSet.insert(startNodes[i], 0.0f);
	}

	NODE_ID cheapestNode;
	EDGE_ITER edgeIt(this);
	while (true)
	{
		cheapestNode = -1;

		float minDist = F_INFINITY;
		auto bestNode = openSet.begin();
		for (auto it = openSet.begin(); it != openSet.end(); ++it)
		{
			if (*it < minDist)
			{
				bestNode = it;
				minDist = *it;
			}
		}

		if (bestNode == openSet.end())
			break;

		cheapestNode = bestNode.key();

		Node_MarkProcessed(cheapestNode);
		openSet.remove(bestNode);

		// walk through edges and neighbour nodes
		for (edgeIt.Rewind(cheapestNode); edgeIt.HasMoreNodes(); edgeIt++)
		{
			const int edgeId = edgeIt.GetEdgeId();
			const NODE_ID neighbourNode = Edge_GetNeighbourNode(cheapestNode, edgeId);

			if (Node_IsProcessed(neighbourNode))
				continue;

			const float edgeLen = Edge_GetLength(cheapestNode, edgeId);
			const float distance = minDist + edgeLen;
			if (distance < Node_GetDistance(neighbourNode))
			{
				Node_SetDistance(neighbourNode, distance);
				Node_SetParent(neighbourNode, cheapestNode);
				openSet.insert(neighbourNode, distance);
			}
		}

		if (isStopNodeFunc(cheapestNode, minDist))
			break;
	}

	return cheapestNode;
}