//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Grid tools
//
// TODO: diagonal (45 degree) grid for fast rotational
//////////////////////////////////////////////////////////////////////////////////

#include "grid.h"

#include "utils/DkList.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/IDebugOverlay.h"
#include "math/coord.h"


inline void ListLine(const Vector3D &from, const Vector3D &to, DkList<Vertex3D_t> &verts)
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

void DrawGrid(float size, int count, const Vector3D& pos, const ColorRGBA& color, bool depthTest)
{
	int grid_lines = count;

	g_pShaderAPI->SetTexture(NULL, NULL, 0);
	materials->SetDepthStates(depthTest, false);
	materials->SetRasterizerStates(CULL_BACK, FILL_SOLID, false, false, true);
	materials->SetBlendingStates(BLENDFACTOR_SRC_ALPHA, BLENDFACTOR_ONE_MINUS_SRC_ALPHA);

	materials->BindMaterial(materials->GetDefaultMaterial());

	int numOfLines = grid_lines / size;

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
	meshBuilder.Begin(PRIM_LINES);

	for (int i = 0; i <= numOfLines; i++)
	{
		int max_grid_size = grid_lines;
		float grid_step = size * float(i);

		meshBuilder.Color4fv(color);

		meshBuilder.Line3fv(pos + Vector3D(0, 0, grid_step), pos + Vector3D(max_grid_size, 0, grid_step));
		meshBuilder.Line3fv(pos + Vector3D(grid_step, 0, 0), pos + Vector3D(grid_step, 0, max_grid_size));

		meshBuilder.Line3fv(pos + Vector3D(0, 0, -grid_step), pos + Vector3D(-max_grid_size, 0, -grid_step));
		meshBuilder.Line3fv(pos + Vector3D(-grid_step, 0, 0), pos + Vector3D(-grid_step, 0, -max_grid_size));

		// draw another part
		meshBuilder.Line3fv(pos + Vector3D(0, 0, -grid_step), pos + Vector3D(max_grid_size, 0, -grid_step));
		meshBuilder.Line3fv(pos + Vector3D(-grid_step, 0, 0), pos + Vector3D(-grid_step, 0, max_grid_size));

		meshBuilder.Line3fv(pos + Vector3D(0, 0, grid_step), pos + Vector3D(-max_grid_size, 0, grid_step));
		meshBuilder.Line3fv(pos + Vector3D(grid_step, 0, 0), pos + Vector3D(grid_step, 0, -max_grid_size));
	}

	meshBuilder.End();
}
