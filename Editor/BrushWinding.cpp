//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Level optimization
//////////////////////////////////////////////////////////////////////////////////

#include "math/Vector.h"
#include "BuildOptionsDialog.h"
#include "EqBrush.h"
#include "IDebugOverlay.h"
#include "cmd_pacifier.h"

#define BRUSH_VERTEX_SNAP 0.1f

#define TINY_EPSILON 0.5

bool WindingIsTiny(EqBrushWinding_t* pWinding)
{
	int nEdges = 0;

	for(int i = 0; i < pWinding->vertices.numElem(); i++)
	{
		int j = i == pWinding->vertices.numElem() - 1 ? 0 : i + 1;

		Vector3D delta = pWinding->vertices[i].position - pWinding->vertices[j].position;

		if(length(delta) > TINY_EPSILON)
		{
			if(++nEdges == 3)
				return false;
		}
	}

	return true;
}

void SubdivideWindings(DkList<EqBrushWinding_t> &windings, DkList<bool> &bUseTable)
{
	DkList<EqBrushWinding_t*> pOriginalFaces;

	for(int i = 0; i < windings.numElem(); i++)
	{
		pOriginalFaces.append(&windings[i]);

		IMatVar* pNodraw = windings[i].pAssignedFace->pMaterial->FindMaterialVar("nodraw");
		IMatVar* pNoCollide = windings[i].pAssignedFace->pMaterial->FindMaterialVar("nocollide");

		bool nodraw = false;
		bool nocollide = false;

		if(pNodraw && pNodraw->GetInt() > 0)
			nodraw = true;

		if(pNoCollide && pNoCollide->GetInt() > 0)
			nocollide = true;

		if(nodraw || nocollide)
		{
			bUseTable.append(false);
			continue;
		}

		bUseTable.append(true);
	}

	for(int i = 0; i < windings.numElem(); i++)
	{
		if(!bUseTable[i])
			continue;

		for(int j = 0; j < windings.numElem(); j++)
		{
			if(i == j)
				continue;

			if(!bUseTable[j])
				continue;

			EqBrushWinding_t *windingA = &windings[i];
			EqBrushWinding_t *windingB = &windings[j];

			if(windingA->pAssignedBrush == windingB->pAssignedBrush)
				continue;

			BoundingBox b1box(windingA->pAssignedBrush->GetBBoxMins(), windingA->pAssignedBrush->GetBBoxMaxs());
			BoundingBox b2box(windingB->pAssignedBrush->GetBBoxMins(), windingB->pAssignedBrush->GetBBoxMaxs());

			if(!b1box.Intersects(b2box))
				continue;


			// TODO: Get intersection points and compute their size, then compare
			// don't clip massive objects by tiny brushes
			if(length(b1box.GetSize()) > length(b2box.GetSize()))
				continue;
				

			if(!windingA->pAssignedBrush->IsWindingIntersectsBrush(windingB))
				continue;

			// if winding is completely inside brush, ignore it
			if(windingA->pAssignedBrush->IsWindingFullyInsideBrush(windingB))
			{
				bUseTable[j] = false;
				continue;
			}

			ClassifyPoly_e cpl = windingA->Classify(windingB);

			if(cpl == CPL_SPLIT)
			{
				EqBrushWinding_t front;
				EqBrushWinding_t back;

				// split winding
				windingA->Split(windingB, &front, &back);

				if(windingA->pAssignedBrush->IsWindingFullyInsideBrush(&front) || WindingIsTiny(&front))
				{
					front.vertices.clear();
				}

				if(windingA->pAssignedBrush->IsWindingFullyInsideBrush(&back) || WindingIsTiny(&back))
				{
					back.vertices.clear();
				}

				if(!front.vertices.numElem() && !back.vertices.numElem())
					continue;

				// don't use splitted polygon again
				bUseTable[j] = false;
				
				if(front.vertices.numElem())
				{
					windings.append(front);
					bUseTable.append(true);
				}
				
				if(back.vertices.numElem())
				{
					windings.append(back);
					bUseTable.append(true);
				}
				
			}
		}
	}
	
}

void SnapAndWeldVerts(EqBrushWinding_t* pWindingA, EqBrushWinding_t* pWindingB)
{
	for(int i = 0; i < pWindingA->vertices.numElem(); i++)
	{
		for(int j = 0; j < pWindingB->vertices.numElem(); j++)
		{
			if(distance(pWindingA->vertices[i].position, pWindingB->vertices[j].position) < BRUSH_VERTEX_SNAP)
			{
				// snap them to first
				pWindingB->vertices[j].position = pWindingA->vertices[i].position;
			}
		}
	}
}

bool IsFacingEquals(EqBrushWinding_t* pWindingA, EqBrushWinding_t* pWindingB)
{
	if(pWindingA->vertices.numElem() != pWindingB->vertices.numElem())
		return false;

	int nEquals = 0;

	for(int i = 0; i < pWindingA->vertices.numElem(); i++)
	{
		for(int j = 0; j < pWindingB->vertices.numElem(); j++)
		{
			if(distance(pWindingA->vertices[i].position, pWindingB->vertices[j].position) < BRUSH_VERTEX_SNAP)
				nEquals++;
		}
	}

	return nEquals >= pWindingA->vertices.numElem();
}