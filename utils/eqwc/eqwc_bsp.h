//**************** Copyright (C) Parallel Prevision, L.L.C 2012 ******************
//
// Description: EQWC BSP definitions
//
//				Equilibrium Engine uses BSP only for building portals and
//				use visibility. Because we create world geometry in external editors,
//				engine needs additional information about spacial subdivision.
//
//********************************************************************************

#ifndef EQWC_BSP_H
#define EQWC_BSP_H

#include "Math/Vector.h"
#include "Math/BoundingBox.h"

#define	CLIP_EPSILON	0.1f
#define PLANE_LEAF		-1

struct brushwinding_t;
struct cwbrush_t;
struct cwbrushface_t;

extern DkList<Plane>	g_Planes; 

enum ClassifySide_e
{
	CS_FRONT = 0,
	CS_BACK,
	CS_ONPLANE,
	CS_CROSS,
};

// brush face
struct bspbrushface_t
{
	bspbrushface_t*					next;

	cwbrush_t*						brush;
	int								face_id;

	brushwinding_t*					winding;

	bool							checked;
};

// tree node
struct node_t
{
	// both leafs and nodes
	int					planenum;	// -1 = leaf node
	node_t*				parent;
	BoundingBox			bounds;		// valid after portalization

	// nodes only
	cwbrushface_t*		side;		// the side that created the node
	node_t*				children[2];
	int					nodeNumber;	// set after pruning

	// leafs only
	bool				opaque;		// view can never be inside

	cwbrush_t*			brushlist; // fragments of all brushes in this leaf
									// needed for FindSideForPortal

	int					area;		// determined by flood filling up to areaportals
	int					occupied;	// 1 or greater can reach entity

	//struct uPortal_s *	portals;	// also on nodes during construction
};

struct tree_t
{
	node_t*			headnode;
	node_t			outside_node;
	BoundingBox		bounds;
};

bspbrushface_t*		CreateBrushFaceList();
void				BuildFaceTree_r( node_t *node, bspbrushface_t *list );

ClassifySide_e		ClassifySide(brushwinding_t* pWinding, Plane &plane);
tree_t*				BuildBSPFaceTree(bspbrushface_t *list);


void				ClipFacesByTree(tree_t* pTree);

node_t*				AllocNode();
tree_t*				AllocTree();


#endif // EQWC_BSP_H