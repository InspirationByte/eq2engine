// dear imgui: Renderer Backend for Equilibrium Engine

#include "core/core_common.h"
#include "imgui_impl_matsystem.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"
#include "imaging/ImageLoader.h"

// DirectX data
struct ImGui_ImplMatSystem_Data
{
	ITexturePtr	FontTexture;

	ImGui_ImplMatSystem_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplMatSystem_Data* ImGui_ImplMatSystem_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplMatSystem_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

static void ImGui_ImplMatSystem_SetupRenderState(ImDrawData* draw_data, IGPURenderPassRecorder* rendPassRecorder, const RenderDrawCmd& drawCmd)
{
	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;

	g_matSystem->SetupMaterialPipeline(g_matSystem->GetDefaultMaterial(), nullptr, drawCmd.batchInfo.primTopology, drawCmd.instanceInfo.instFormat, RenderPassContext(rendPassRecorder, &defaultRenderPass));

	rendPassRecorder->SetViewport(AARectangle(0.0f, 0.0f, draw_data->FramebufferScale.x * draw_data->DisplaySize.x, draw_data->FramebufferScale.y * draw_data->DisplaySize.y), 0.0f, 1.0f);
	rendPassRecorder->SetVertexBufferView(0, drawCmd.instanceInfo.vertexBuffers[0]);
	rendPassRecorder->SetIndexBufferView(drawCmd.instanceInfo.indexBuffer, drawCmd.instanceInfo.indexFormat);
}

// Dynamic mesh capacity is lower than a draw list can have (MAX_DYNAMIC_VERTICES and MAX_DYNAMIC_INDICES
// in DynamicMesh.cpp), draw commands are split by triangles into chunks and chunks are packed into meshes
static constexpr int s_maxBatchVertices = 32000;
static constexpr int s_maxBatchIndices = 31998;		// whole triangles

// part of a draw command that fits into one dynamic mesh
struct ImGui_ImplMatSystem_DrawChunk
{
	const ImDrawCmd*	cmd;
	int					firstIndex;		// in the draw list index buffer
	int					numIndices;
	int					minVertex;		// referenced range of the draw list vertex buffer
	int					maxVertex;
};

static void ImGui_ImplMatSystem_DrawChunks(ImDrawData* draw_data, IGPURenderPassRecorder* rendPassRecorder, const ImDrawList* cmd_list, ArrayCRef<ImGui_ImplMatSystem_DrawChunk> chunks, int numVertices, int numIndices, RenderDrawCmd& drawCmd)
{
	if (chunks.isEmpty())
		return;

	IDynamicMeshPtr dynMesh = g_matSystem->GetDynamicMesh(max(numVertices, numIndices));
	CMeshBuilder mb(dynMesh);
	mb.Begin(PRIM_TRIANGLES);

	// vertices of each chunk go one after another
	for (const ImGui_ImplMatSystem_DrawChunk& chunk : chunks)
	{
		for (int i = chunk.minVertex; i <= chunk.maxVertex; i++)
		{
			const ImDrawVert& vtx_src = cmd_list->VtxBuffer.Data[i];
			MColor color(vtx_src.col);

			mb.Position2f(vtx_src.pos.x, vtx_src.pos.y);
			mb.TexCoord2f(vtx_src.uv.x, vtx_src.uv.y);
			mb.Color4fv(color);
			mb.AdvanceVertex();
		}
	}

	// indices are rebased to the chunk vertices
	int baseVertex = 0;
	for (const ImGui_ImplMatSystem_DrawChunk& chunk : chunks)
	{
		ImDrawIdx* idx_dst;
		void* vtx_dst;
		if (dynMesh->AllocateGeom(0, chunk.numIndices, &vtx_dst, &idx_dst, false) != -1)
		{
			const ImDrawIdx* idx_src = cmd_list->IdxBuffer.Data + chunk.firstIndex;
			const int indexOffset = chunk.cmd->VtxOffset - chunk.minVertex + baseVertex;
			for (int i = 0; i < chunk.numIndices; i++)
				idx_dst[i] = static_cast<ImDrawIdx>(idx_src[i] + indexOffset);
		}
		baseVertex += chunk.maxVertex - chunk.minVertex + 1;
	}

	if (!mb.End(drawCmd))
		return;

	const ImVec2 clip_off = draw_data->DisplayPos;
	int firstIndex = 0;
	for (const ImGui_ImplMatSystem_DrawChunk& chunk : chunks)
	{
		const ImDrawCmd* pcmd = chunk.cmd;
		const int chunkFirstIndex = firstIndex;
		firstIndex += chunk.numIndices;

		// Project scissor/clipping rectangles into framebuffer space
		ImVec2 clip_min(pcmd->ClipRect.x - clip_off.x, pcmd->ClipRect.y - clip_off.y);
		ImVec2 clip_max(pcmd->ClipRect.z - clip_off.x, pcmd->ClipRect.w - clip_off.y);
		if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
			continue;

		// Apply Scissor/clipping rectangle, Bind texture, Draw

		MatSysDefaultRenderPass defaultRenderPass;
		defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;
		defaultRenderPass.texture = ITexturePtr((ITexture*)pcmd->GetTexID());

		g_matSystem->SetupMaterialPipeline(g_matSystem->GetDefaultMaterial(), nullptr, drawCmd.batchInfo.primTopology, drawCmd.instanceInfo.instFormat, RenderPassContext(rendPassRecorder, &defaultRenderPass));

		rendPassRecorder->SetScissorRectangle(IAARectangle((int)clip_min.x, (int)clip_min.y, (int)clip_max.x, (int)clip_max.y));
		rendPassRecorder->SetVertexBufferView(0, drawCmd.instanceInfo.vertexBuffers[0]);
		rendPassRecorder->SetIndexBufferView(drawCmd.instanceInfo.indexBuffer, INDEXFMT_UINT16);
		rendPassRecorder->DrawIndexed(chunk.numIndices, chunkFirstIndex, 1);
	}
}

// Render function.
void ImGui_ImplMatSystem_RenderDrawData(ImDrawData* draw_data, IGPURenderPassRecorder* rendPassRecorder)
{
	// Avoid rendering when minimized
	if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
		return;

	Array<ImGui_ImplMatSystem_DrawChunk> chunks(PP_SL);
	RenderDrawCmd drawCmd;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];

		// chunks waiting for a mesh
		int batchVertices = 0;
		int batchIndices = 0;
		chunks.clear();

		auto flushChunks = [&]() {
			ImGui_ImplMatSystem_DrawChunks(draw_data, rendPassRecorder, cmd_list, chunks, batchVertices, batchIndices, drawCmd);
			chunks.clear();
			batchVertices = 0;
			batchIndices = 0;
		};

		auto addChunk = [&](const ImGui_ImplMatSystem_DrawChunk& chunk) {
			const int chunkVertices = chunk.maxVertex - chunk.minVertex + 1;
			if (batchVertices + chunkVertices > s_maxBatchVertices || batchIndices + chunk.numIndices > s_maxBatchIndices)
				flushChunks();

			chunks.append(chunk);
			batchVertices += chunkVertices;
			batchIndices += chunk.numIndices;
		};

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

			if (pcmd->UserCallback != nullptr)
			{
				// commands before the callback are drawn first
				flushChunks();

				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
				{
					if (drawCmd.instanceInfo.vertexBuffers[0].buffer)
						ImGui_ImplMatSystem_SetupRenderState(draw_data, rendPassRecorder, drawCmd);
				}
				else
					pcmd->UserCallback(cmd_list, pcmd);

				continue;
			}

			// split the command by triangles into chunks that fit into one mesh
			ImGui_ImplMatSystem_DrawChunk chunk;
			chunk.cmd = pcmd;
			chunk.firstIndex = pcmd->IdxOffset;
			chunk.numIndices = 0;
			chunk.minVertex = INT_MAX;
			chunk.maxVertex = -1;

			const ImDrawIdx* indices = cmd_list->IdxBuffer.Data;
			for (int i = pcmd->IdxOffset; i + 2 < (int)(pcmd->IdxOffset + pcmd->ElemCount); i += 3)
			{
				const int triMin = min(min(indices[i], indices[i + 1]), indices[i + 2]) + pcmd->VtxOffset;
				const int triMax = max(max(indices[i], indices[i + 1]), indices[i + 2]) + pcmd->VtxOffset;

				const int minVertex = min(chunk.minVertex, triMin);
				const int maxVertex = max(chunk.maxVertex, triMax);
				if (chunk.numIndices && (maxVertex - minVertex + 1 > s_maxBatchVertices || chunk.numIndices + 3 > s_maxBatchIndices))
				{
					addChunk(chunk);

					chunk.firstIndex = i;
					chunk.numIndices = 0;
					chunk.minVertex = triMin;
					chunk.maxVertex = triMax;
				}
				else
				{
					chunk.minVertex = minVertex;
					chunk.maxVertex = maxVertex;
				}
				chunk.numIndices += 3;
			}

			if (chunk.numIndices)
				addChunk(chunk);
		}

		flushChunks();
	}
}

bool ImGui_ImplMatSystem_Init()
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	ImGui_ImplMatSystem_Data* bd = IM_NEW(ImGui_ImplMatSystem_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "eqMatSystem";
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

	return true;
}

void ImGui_ImplMatSystem_Shutdown()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplMatSystem_InvalidateDeviceObjects();
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	IM_DELETE(bd);
}

static bool ImGui_ImplMatSystem_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	unsigned char* pixels;
	int width, height, bytes_per_pixel;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

	CImagePtr image = CRefPtr_new(CImage);
	image->SetName("_imfont");

	ubyte* pImage = image->Create(FORMAT_RGBA8, width, height, 1, 1);
	memcpy(pImage, pixels, GetBytesPerPixel(FORMAT_RGBA8) * width * height);

	SamplerStateParams params(TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
	bd->FontTexture = g_renderAPI->CreateTexture(image, params);

	ASSERT(bd->FontTexture);
	if (!bd->FontTexture)
		return false;

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)bd->FontTexture);

	return true;
}

bool ImGui_ImplMatSystem_CreateDeviceObjects()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	if (!bd)
		return false;
	if (!ImGui_ImplMatSystem_CreateFontsTexture())
		return false;
	return true;
}

void ImGui_ImplMatSystem_InvalidateDeviceObjects()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	if (!bd)
		return;
	if (bd->FontTexture) 
	{ 
		bd->FontTexture = nullptr; 
		ImGui::GetIO().Fonts->SetTexID(nullptr); 
	} // We copied bd->pFontTextureView to io.Fonts->TexID so let's clear that as well.
}

void ImGui_ImplMatSystem_NewFrame()
{
	ImGui_ImplMatSystem_Data* bd = ImGui_ImplMatSystem_GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplMatSystem_Init()?");

	if (!bd->FontTexture)
		ImGui_ImplMatSystem_CreateDeviceObjects();
}
