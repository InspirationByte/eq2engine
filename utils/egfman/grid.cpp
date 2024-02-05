//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Grid tools
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "grid.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "render/IDebugOverlay.h"


inline void ListLine(const Vector3D &from, const Vector3D &to, Array<Vertex3D> &verts)
{
	verts.append(Vertex3D(from, vec2_zero));
	verts.append(Vertex3D(to, vec2_zero));
}

void DrawWorldCenter(IGPURenderPassRecorder* rendPassRecorder)
{
	Array<Vertex3D> grid_vertices(PP_SL);

	ListLine(Vector3D(-F_INFINITY,0,0),Vector3D(F_INFINITY,0,0), grid_vertices);
	ListLine(Vector3D(0,-F_INFINITY,0),Vector3D(0, F_INFINITY,0), grid_vertices);
	ListLine(Vector3D(0,0,-F_INFINITY),Vector3D(0,0, F_INFINITY), grid_vertices);

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.drawColor = MColor(0.0f, 0.45f, 0.45f, 1.0f);

	g_matSystem->SetupDrawDefaultUP(PRIM_LINES, grid_vertices, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

void DrawGrid(float size, int count, const Vector3D& pos, const ColorRGBA& color, bool depthTest, IGPURenderPassRecorder* rendPassRecorder)
{
	const int grid_lines = count;
	const int numOfLines = grid_lines / size;

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;

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

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}
