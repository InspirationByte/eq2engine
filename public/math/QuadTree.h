//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Quadtree
//////////////////////////////////////////////////////////////////////////////////

#ifndef QUADTREE_H
#define QUADTREE_H

#include "Vector.h"
 
#define QT_NO_DIMS			2
#define QT_NODE_CAPACITY	1

struct QCell
{
public:
    float x;
    float y;
    float hw;
    float hh;

    bool ContainsPoint(const Vector2D& point)
	{
		if(x - hw > point.x) return false;
		if(x + hw < point.x) return false;
		if(y - hh > point.y) return false;
		if(y + hh < point.y) return false;

		return true;
	}

    bool ContainsPoint(float* point)
	{
		if(x - hw > point[0]) return false;
		if(x + hw < point[0]) return false;
		if(y - hh > point[1]) return false;
		if(y + hh < point[1]) return false;

		return true;
	}
};

// The tree itself

class QuadTree
{
public:
	QuadTree(float* inp_data, int N);
	QuadTree(float* inp_data, float inp_x, float inp_y, float inp_hw, float inp_hh);
	QuadTree(float* inp_data, int N, float inp_x, float inp_y, float inp_hw, float inp_hh);
	QuadTree(QuadTree* inp_parent, float* inp_data, int N, float inp_x, float inp_y, float inp_hw, float inp_hh);
	QuadTree(QuadTree* inp_parent, float* inp_data, float inp_x, float inp_y, float inp_hw, float inp_hh);

					~QuadTree();

	void			SetData(float* inp_data);

	QuadTree*		GetParent();

	void			Construct(const QCell& boundary);
	bool			Insert(int new_index);

	void			Subdivide();

	bool			IsCorrect();
	void			RebuildTree();

	void			GetAllIndices(int* indices);

	int				GetDepth();

	void			ComputeNonEdgeForces(int point_index, float theta, float neg_f[], float* sum_Q);
	void			ComputeEdgeForces(int* row_P, int* col_P, float* val_P, int N, float* pos_f);    

private:
    void			Init(QuadTree* inp_parent, float* inp_data, float inp_x, float inp_y, float inp_hw, float inp_hh);

    void			Fill(int N);

    int				GetAllIndices(int* indices, int loc);
    bool			IsChild(int test_index, int start, int end);

protected:

	// A buffer we use when doing force computations
	float			buff[QT_NO_DIMS];
    
	// Properties of this node in the tree
	QuadTree*		parent;
	bool			is_leaf;
	int				size;
	int				cum_size;
        
	// Axis-aligned bounding box stored as a center with half-dimensions to represent the boundaries of this quad tree
	QCell			boundary;
    
	// Indices in this quad tree node, corresponding center-of-mass, and list of all children
	float*			data;
	float			center_of_mass[QT_NO_DIMS];
	int				index[QT_NODE_CAPACITY];
    
	// Children
	QuadTree*		northWest;
	QuadTree*		northEast;
	QuadTree*		southWest;
	QuadTree*		southEast;
};

#endif //QUADTREE_H