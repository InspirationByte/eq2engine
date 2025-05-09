//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Quadtree
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "QuadTree.h"

// Default constructor for quadtree -- build tree, too!
QuadTree::QuadTree(float* inp_data, int N)
{
    // Compute mean, width, and height of current map (boundaries of quadtree)
    float* mean_Y = PPNew float[QT_NO_DIMS];

	for(int d = 0; d < QT_NO_DIMS; d++)
		mean_Y[d] = .0;

    float*  min_Y = PPNew float[QT_NO_DIMS];

	for(int d = 0; d < QT_NO_DIMS; d++)
		min_Y[d] =  FLT_MAX;

    float*  max_Y = PPNew float[QT_NO_DIMS];

	for(int d = 0; d < QT_NO_DIMS; d++)
		max_Y[d] = -FLT_MAX;

    for(int n = 0; n < N; n++)
	{
        for(int d = 0; d < QT_NO_DIMS; d++)
		{
            mean_Y[d] += inp_data[n * QT_NO_DIMS + d];

            if(inp_data[n * QT_NO_DIMS + d] < min_Y[d])
				min_Y[d] = inp_data[n * QT_NO_DIMS + d];

            if(inp_data[n * QT_NO_DIMS + d] > max_Y[d])
				max_Y[d] = inp_data[n * QT_NO_DIMS + d];
        }
    }

    for(int d = 0; d < QT_NO_DIMS; d++)
		mean_Y[d] /= (float)N;
    
    // Construct quadtree
    Init(nullptr, inp_data, mean_Y[0], mean_Y[1], max(max_Y[0] - mean_Y[0], mean_Y[0] - min_Y[0]) + 1e-5f,
                                               max(max_Y[1] - mean_Y[1], mean_Y[1] - min_Y[1]) + 1e-5f);
    Fill(N);

    delete[] mean_Y;
	delete[] max_Y;
	delete[] min_Y;
}


// Constructor for quadtree with particular size and parent -- build the tree, too!
QuadTree::QuadTree(float* inp_data, int N, float inp_x, float inp_y, float inp_hw, float inp_hh)
{
    Init(nullptr, inp_data, inp_x, inp_y, inp_hw, inp_hh);
    Fill(N);
}

// Constructor for quadtree with particular size and parent -- build the tree, too!
QuadTree::QuadTree(QuadTree* inp_parent, float* inp_data, int N, float inp_x, float inp_y, float inp_hw, float inp_hh)
{
    Init(inp_parent, inp_data, inp_x, inp_y, inp_hw, inp_hh);
    Fill(N);
}


// Constructor for quadtree with particular size (do not fill the tree)
QuadTree::QuadTree(float* inp_data, float inp_x, float inp_y, float inp_hw, float inp_hh)
{
    Init(nullptr, inp_data, inp_x, inp_y, inp_hw, inp_hh);
}


// Constructor for quadtree with particular size and parent (do not fill the tree)
QuadTree::QuadTree(QuadTree* inp_parent, float* inp_data, float inp_x, float inp_y, float inp_hw, float inp_hh)
{
    Init(inp_parent, inp_data, inp_x, inp_y, inp_hw, inp_hh);
}


// Main initialization function
void QuadTree::Init(QuadTree* inp_parent, float* inp_data, float inp_x, float inp_y, float inp_hw, float inp_hh)
{
    parent = inp_parent;
    data = inp_data;
    is_leaf = true;
    size = 0;
    cum_size = 0;
    boundary.x  = inp_x;
    boundary.y  = inp_y;
    boundary.hw = inp_hw;
    boundary.hh = inp_hh;
    northWest = nullptr;
    northEast = nullptr;
    southWest = nullptr;
    southEast = nullptr;

    for(int i = 0; i < QT_NO_DIMS; i++)
		center_of_mass[i] = .0;
}


// Destructor for quadtree
QuadTree::~QuadTree()
{
    delete northWest;
    delete northEast;
    delete southWest;
    delete southEast;
}


// Update the data underlying this tree
void QuadTree::SetData(float* inp_data)
{
    data = inp_data;
}


// Get the parent of the current tree
QuadTree* QuadTree::GetParent()
{
    return parent;
}


// Insert a point into the QuadTree
bool QuadTree::Insert(int new_index)
{
    // Ignore objects which do not belong in this quad tree
    float* point = data + new_index * QT_NO_DIMS;

    if(!boundary.ContainsPoint(point))
        return false;
    
    // Online update of cumulative size and center-of-mass
    cum_size++;
    float mult1 = (float) (cum_size - 1) / (float) cum_size;
    float mult2 = 1.0f / (float) cum_size;

    for(int d = 0; d < QT_NO_DIMS; d++)
		center_of_mass[d] *= mult1;

    for(int d = 0; d < QT_NO_DIMS; d++)
		center_of_mass[d] += mult2 * point[d];
    
    // If there is space in this quad tree and it is a leaf, add the object here
    if(is_leaf && size < QT_NODE_CAPACITY)
	{
        index[size] = new_index;
        size++;
        return true;
    }
    
    // Don't add duplicates for now (this is not very nice)
    bool any_duplicate = false;
    for(int n = 0; n < size; n++) {
        bool duplicate = true;
        for(int d = 0; d < QT_NO_DIMS; d++) {
            if(point[d] != data[index[n] * QT_NO_DIMS + d]) { duplicate = false; break; }
        }
        any_duplicate = any_duplicate | duplicate;
    }
    if(any_duplicate) return true;
    
    // Otherwise, we need to subdivide the current cell
    if(is_leaf)
		Subdivide();
    
    // Find out where the point can be inserted
    if(northWest->Insert(new_index))
		return true;

    if(northEast->Insert(new_index))
		return true;

    if(southWest->Insert(new_index)) 
		return true;

    if(southEast->Insert(new_index))
		return true;
    
    // Otherwise, the point cannot be inserted (this should never happen)
    return false;
}

    
// Create four children which fully divide this cell into four quads of equal area
void QuadTree::Subdivide()
{
    // Create four children
    northWest = PPNew QuadTree(this, data, boundary.x - 0.5f * boundary.hw, boundary.y - 0.5f * boundary.hh, 0.5f * boundary.hw, 0.5f * boundary.hh);
    northEast = PPNew QuadTree(this, data, boundary.x + 0.5f * boundary.hw, boundary.y - 0.5f * boundary.hh, 0.5f * boundary.hw, 0.5f * boundary.hh);
    southWest = PPNew QuadTree(this, data, boundary.x - 0.5f * boundary.hw, boundary.y + 0.5f * boundary.hh, 0.5f * boundary.hw, 0.5f * boundary.hh);
    southEast = PPNew QuadTree(this, data, boundary.x + 0.5f * boundary.hw, boundary.y + 0.5f * boundary.hh, 0.5f * boundary.hw, 0.5f * boundary.hh);
    
    // Move existing points to correct children
    for(int i = 0; i < size; i++)
	{
        bool success = false;

        if(!success)
			success = northWest->Insert(index[i]);
        if(!success)
			success = northEast->Insert(index[i]);
        if(!success)
			success = southWest->Insert(index[i]);
        if(!success)
			success = southEast->Insert(index[i]);
        index[i] = -1;
    }
    
    // Empty parent node
    size = 0;
    is_leaf = false;
}


// Build quadtree on dataset
void QuadTree::Fill(int N)
{
    for(int i = 0; i < N; i++)
		Insert(i);
}


// Checks whether the specified tree is correct
bool QuadTree::IsCorrect()
{
    for(int n = 0; n < size; n++)
	{
        float* point = data + index[n] * QT_NO_DIMS;

        if(!boundary.ContainsPoint(point))
			return false;
    }

    if(!is_leaf) 
	{
		return northWest->IsCorrect() &&
               northEast->IsCorrect() &&
               southWest->IsCorrect() &&
               southEast->IsCorrect();
	}
    else
		return true;
}


// Rebuilds a possibly incorrect tree (LAURENS: This function is not tested yet!)
void QuadTree::RebuildTree()
{
    for(int n = 0; n < size; n++) {
        
        // Check whether point is erroneous
        float* point = data + index[n] * QT_NO_DIMS;
        if(!boundary.ContainsPoint(point))
		{
            // Remove erroneous point
            int rem_index = index[n];

            for(int m = n + 1; m < size; m++)
				index[m - 1] = index[m];

            index[size - 1] = -1;
            size--;
            
            // Update center-of-mass and counter in all parents
            bool done = false;
            QuadTree* node = this;
            while(!done)
			{
                for(int d = 0; d < QT_NO_DIMS; d++)
                    node->center_of_mass[d] = ((float) node->cum_size * node->center_of_mass[d] - point[d]) / (float) (node->cum_size - 1);

                node->cum_size--;

                if(node->GetParent() == nullptr)
					done = true;
                else
					node = node->GetParent();
            }
            
            // Reinsert point in the root tree
            node->Insert(rem_index);
        }
    }

	if (!is_leaf)
	{
		// Rebuild lower parts of the tree
		northWest->RebuildTree();
		northEast->RebuildTree();
		southWest->RebuildTree();
		southEast->RebuildTree();
	}
}


// Build a list of all indices in quadtree
void QuadTree::GetAllIndices(int* indices)
{
    GetAllIndices(indices, 0);
}


// Build a list of all indices in quadtree
int QuadTree::GetAllIndices(int* indices, int loc)
{
    
    // Gather indices in current quadrant
    for(int i = 0; i < size; i++)
		indices[loc + i] = index[i];

    loc += size;
    
    // Gather indices in children
    if(!is_leaf)
	{
        loc = northWest->GetAllIndices(indices, loc);
        loc = northEast->GetAllIndices(indices, loc);
        loc = southWest->GetAllIndices(indices, loc);
        loc = southEast->GetAllIndices(indices, loc);
    }

    return loc;
}


int QuadTree::GetDepth()
{
    if(is_leaf)
		return 1;

    return 1 + max(max(northWest->GetDepth(),
                       northEast->GetDepth()),
                   max(southWest->GetDepth(),
                       southEast->GetDepth()));
                   
}


// Compute non-edge forces using Barnes-Hut algorithm
void QuadTree::ComputeNonEdgeForces(int point_index, float theta, float neg_f[], float* sum_Q)
{
    
    // Make sure that we spend no time on empty nodes or self-interactions
    if(cum_size == 0 || (is_leaf && size == 1 && index[0] == point_index))
		return;
    
    // Compute distance between point and center-of-mass
    float D = 0.0f;
    int ind = point_index * QT_NO_DIMS;

    for(int d = 0; d < QT_NO_DIMS; d++)
		buff[d]  = data[ind + d];

    for(int d = 0; d < QT_NO_DIMS; d++)
		buff[d] -= center_of_mass[d];

    for(int d = 0; d < QT_NO_DIMS; d++)
		D += buff[d] * buff[d];
    
    // Check whether we can use this node as a "summary"
    if(is_leaf || max(boundary.hh, boundary.hw) / sqrt(D) < theta)
	{
        // Compute and add t-SNE force between point and current node
        float Q = 1.0f / (1.0f + D);
        *sum_Q += cum_size * Q;

        float mult = cum_size * Q * Q;

        for(int d = 0; d < QT_NO_DIMS; d++)
			neg_f[d] += mult * buff[d];
    }
    else
	{

        // Recursively apply Barnes-Hut to children
        northWest->ComputeNonEdgeForces(point_index, theta, neg_f, sum_Q);
        northEast->ComputeNonEdgeForces(point_index, theta, neg_f, sum_Q);
        southWest->ComputeNonEdgeForces(point_index, theta, neg_f, sum_Q);
        southEast->ComputeNonEdgeForces(point_index, theta, neg_f, sum_Q);
    }
}


// Computes edge forces
void QuadTree::ComputeEdgeForces(int* row_P, int* col_P, float* val_P, int N, float* pos_f)
{
    // Loop over all edges in the graph
    int ind1, ind2;

    float D;

    for(int n = 0; n < N; n++)
	{
        ind1 = n * QT_NO_DIMS;
        for(int i = row_P[n]; i < row_P[n + 1]; i++)
		{
            // Compute pairwise distance and Q-value
            D = .0;
            ind2 = col_P[i] * QT_NO_DIMS;

            for(int d = 0; d < QT_NO_DIMS; d++)
				buff[d]  = data[ind1 + d];

            for(int d = 0; d < QT_NO_DIMS; d++)
				buff[d] -= data[ind2 + d];

            for(int d = 0; d < QT_NO_DIMS; d++)
				D += buff[d] * buff[d];
            D = val_P[i] / (1.0f + D);
            
            // Sum positive force
            for(int d = 0; d < QT_NO_DIMS; d++)
				pos_f[ind1 + d] += D * buff[d];
        }
    }
}

/*
// Print out tree
void QuadTree::print() 
{
    if(cum_size == 0) {
        printf("Empty node\n");
        return;
    }

    if(is_leaf) {
        printf("Leaf node; data = [");
        for(int i = 0; i < size; i++) {
            double* point = data + index[i] * QT_NO_DIMS;
            for(int d = 0; d < QT_NO_DIMS; d++) printf("%f, ", point[d]);
            printf(" (index = %d)", index[i]);
            if(i < size - 1) printf("\n");
            else printf("]\n");
        }        
    }
    else {
        printf("Intersection node with center-of-mass = [");
        for(int d = 0; d < QT_NO_DIMS; d++) printf("%f, ", center_of_mass[d]);
        printf("]; children are:\n");
        northEast->print();
        northWest->print();
        southEast->print();
        southWest->print();
    }
}*/