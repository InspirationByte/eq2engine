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

void DrawGrid(float size, int count, const Vector3D& pos, const ColorRGBA& color, bool depthTest, IGPURenderPassRecorder* rendPassRecorder)
{
	const int numOfLines = count / size;

	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
	defaultRenderPass.depthTest = true;

	meshBuilder.Begin(PRIM_LINES);

	for (int i = 0; i <= numOfLines; i++)
	{
		const float gridStep = size * float(i);

		meshBuilder.Color4fv(color);

		meshBuilder.Line3fv(pos + Vector3D(0, 0, gridStep), pos + Vector3D(count, 0, gridStep));
		meshBuilder.Line3fv(pos + Vector3D(gridStep, 0, 0), pos + Vector3D(gridStep, 0, count));

		meshBuilder.Line3fv(pos + Vector3D(0, 0, -gridStep), pos + Vector3D(-count, 0, -gridStep));
		meshBuilder.Line3fv(pos + Vector3D(-gridStep, 0, 0), pos + Vector3D(-gridStep, 0, -count));

		// draw another part
		meshBuilder.Line3fv(pos + Vector3D(0, 0, -gridStep), pos + Vector3D(count, 0, -gridStep));
		meshBuilder.Line3fv(pos + Vector3D(-gridStep, 0, 0), pos + Vector3D(-gridStep, 0, count));

		meshBuilder.Line3fv(pos + Vector3D(0, 0, gridStep), pos + Vector3D(-count, 0, gridStep));
		meshBuilder.Line3fv(pos + Vector3D(gridStep, 0, 0), pos + Vector3D(gridStep, 0, -count));
	}

	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}
