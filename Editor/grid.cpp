//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Grid tools
//
// TODO: diagonal (45 degree) grid for fast rotational//////////////////////////////////////////////////////////////////////////////////

#include "EditorHeader.h"
#include "IDebugOverlay.h"
#include "Math/DkMath.h"
#include "grid.h"

inline void ListLine(Vector3D &from, Vector3D &to, DkList<Vertex3D_t> &verts)
{
	verts.append(Vertex3D_t(from, vec2_zero));
	verts.append(Vertex3D_t(to, vec2_zero));
}

void DrawWorldCenter()
{
	DkList<Vertex3D_t> grid_vertices;

	ListLine(Vector3D(-MAX_COORD_UNITS,0,0),Vector3D(MAX_COORD_UNITS,0,0), grid_vertices);
	ListLine(Vector3D(0,-MAX_COORD_UNITS,0),Vector3D(0,MAX_COORD_UNITS,0), grid_vertices);
	ListLine(Vector3D(0,0,-MAX_COORD_UNITS),Vector3D(0,0,MAX_COORD_UNITS), grid_vertices);

	DepthStencilStateParams_t depth;

	depth.depthTest = false;
	depth.depthWrite = false;
	depth.depthFunc = COMP_LEQUAL;

	RasterizerStateParams_t raster;

	raster.cullMode = CULL_BACK;
	raster.fillMode = FILL_SOLID;
	raster.multiSample = true;
	raster.scissor = false;

	materials->DrawPrimitivesFFP(PRIM_LINES, grid_vertices.ptr(), grid_vertices.numElem(), NULL, ColorRGBA(0,0.45f,0.45f,1), NULL, &depth, &raster);
}

void DrawGrid(int size, ColorRGBA &color, bool for2D)
{
	int grid_lines = 64*size;

	DkList<Vertex3D_t> grid_vertices(grid_lines / size);

	for(int i = 0; i <= grid_lines / size;i++)
	{
		int max_grid_size = grid_lines;
		int grid_step = size*i;

		ListLine(Vector3D(0,grid_step,0),Vector3D(max_grid_size,grid_step,0), grid_vertices);
		ListLine(Vector3D(grid_step,0,0),Vector3D(grid_step,max_grid_size,0), grid_vertices);

		ListLine(Vector3D(0,-grid_step,0),Vector3D(-max_grid_size,-grid_step,0), grid_vertices);
		ListLine(Vector3D(-grid_step,0,0),Vector3D(-grid_step,-max_grid_size,0), grid_vertices);

		// draw another part

		ListLine(Vector3D(0,-grid_step,0),Vector3D(max_grid_size,-grid_step,0), grid_vertices);
		ListLine(Vector3D(-grid_step,0,0),Vector3D(-grid_step,max_grid_size,0), grid_vertices);

		ListLine(Vector3D(0,grid_step,0),Vector3D(-max_grid_size,grid_step,0), grid_vertices);
		ListLine(Vector3D(grid_step,0,0),Vector3D(grid_step,-max_grid_size,0), grid_vertices);
	}

	DepthStencilStateParams_t depth;

	depth.depthTest = !for2D;
	depth.depthWrite = !for2D;
	depth.depthFunc = COMP_LEQUAL;

	RasterizerStateParams_t raster;

	raster.cullMode = CULL_BACK;
	raster.fillMode = FILL_SOLID;
	raster.multiSample = true;
	raster.scissor = false;

	materials->DrawPrimitivesFFP(PRIM_LINES, grid_vertices.ptr(), grid_vertices.numElem(), NULL, color, NULL, &depth, &raster);
}

float SnapFloat(int grid_spacing, float val)
{
	return round(val / grid_spacing) * grid_spacing;
}

Vector3D SnapVector(int grid_spacing, Vector3D &vector)
{
	return Vector3D(
		SnapFloat(grid_spacing, vector.x),
		SnapFloat(grid_spacing, vector.y),
		SnapFloat(grid_spacing, vector.z));
}